// SPDX-License-Identifier: Apache-2.0
#include "storage.hpp"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "ff.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "psa/crypto.h"
#include "signatures_generated.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
namespace sniffer::storage {
Health health;
namespace {
enum class Op : uint8_t { Record, Save, Export, Eject, Diagnostics, Sound, Clear, Sleep };
struct Message {
    Op op{};
    Detection detection{};
    State state{};
    char token[21]{};
    bool research{};
    uint8_t region{};
    char text[256]{};
};
StaticQueue_t control;
uint8_t memory[8 * sizeof(Message)];
QueueHandle_t queue;
char session_id[33]{};
char filename[160]{};
unsigned rotation{};
FILE *file{};
size_t size{};
unsigned pending{};
uint64_t last_flush{};
char line[2048];
bool mkdir_ok(const char *path) {
    return mkdir(path, 0755) == 0 || errno == EEXIST;
}
bool write_all(FILE *f, const char *text, size_t len) {
    return f && fwrite(text, 1, len, f) == len;
}
bool flush() {
    if (!file)
        return true;
    bool ok = fflush(file) == 0 && fsync(fileno(file)) == 0;
    if (!ok) {
        health.failed = true;
        ++health.errors;
    }
    pending = 0;
    last_flush = esp_timer_get_time() / 1000;
    return ok;
}
bool open_file() {
    std::snprintf(filename, sizeof(filename), "/sd/SURVSNIFF/LOGS/S-%s-%03u.JSONL", session_id,
                  rotation);
    file = fopen(filename, "wx"); // Never overwrite an existing session.
    if (!file) {
        health.failed = true;
        ++health.errors;
        return false;
    }
    setvbuf(file, nullptr, _IOFBF, 2048);
    size = 0;
    return true;
}
void space_status() {
    FATFS *fs{};
    DWORD free{};
    if (f_getfree("0:", &free, &fs) == FR_OK) {
        health.free_bytes = uint64_t(free) * fs->csize * 512;
        health.total_bytes = uint64_t(fs->n_fatent - 2) * fs->csize * 512;
    }
}
void recover_tails() {
    DIR *dir = opendir("/sd/SURVSNIFF/LOGS");
    if (!dir) {
        health.read_only = true;
        return;
    }
    unsigned files = 0;
    while (auto *e = readdir(dir)) {
        std::string_view name(e->d_name);
        if (!name.ends_with(".JSONL") || name.find('/') != name.npos)
            continue;
        if (++files > 4096) {
            health.read_only = true;
            break;
        }
        char path[300];
        std::snprintf(path, sizeof(path), "/sd/SURVSNIFF/LOGS/%s", e->d_name);
        FILE *f = fopen(path, "rb");
        if (!f) {
            health.read_only = true;
            break;
        }
        long good = 0;
        bool corrupt = false, partial = false;
        while (fgets(line, sizeof(line), f)) {
            esp_task_wdt_reset();
            size_t n = std::strlen(line);
            if (!n || line[n - 1] != '\n') {
                partial = feof(f);
                corrupt = !partial;
                break;
            }
            if (!valid_log_record({line, n})) {
                corrupt = true;
                break;
            }
            good = ftell(f);
            vTaskDelay(1);
        }
        if (ferror(f))
            corrupt = true;
        fclose(f);
        if (corrupt) {
            health.read_only = true;
            ++health.errors;
            continue;
        }
        if (partial) {
            f = fopen(path, "rb+");
            if (!f || ftruncate(fileno(f), good) != 0 || fsync(fileno(f)) != 0) {
                health.read_only = true;
                ++health.errors;
            }
            if (f)
                fclose(f);
        }
    }
    closedir(dir);
}
uint32_t checksum(std::span<const uint8_t> bytes) {
    uint32_t crc = ~0U;
    for (auto b : bytes) {
        crc ^= b;
        for (int i = 0; i < 8; ++i)
            crc = (crc >> 1) ^ (0xedb88320U & -(crc & 1U));
    }
    return ~crc;
}
struct ConfigRecord {
    Settings settings;
    board::Calibration calibration;
    BatteryCalibration battery;
};
struct PetRecord {
    Pet pet;
    std::array<MealStamp, 32> recent;
    std::array<uint8_t, 32> key;
};
struct BootRecord {
    uint32_t boots, reason;
};
template <class T> struct Envelope {
    uint32_t version{1}, generation{}, crc{};
    T value{};
};
template <class T> bool write_part(const char *space, uint32_t gen, const T &data) {
    Envelope<T> e{};
    e.generation = gen;
    e.value = data;
    e.crc = checksum({reinterpret_cast<const uint8_t *>(&e.value), sizeof(T)});
    nvs_handle_t h{};
    if (nvs_open(space, NVS_READWRITE, &h) != ESP_OK)
        return false;
    bool ok = nvs_set_blob(h, gen % 2 ? "slot1" : "slot0", &e, sizeof(e)) == ESP_OK &&
              nvs_commit(h) == ESP_OK;
    nvs_close(h);
    return ok;
}
template <class T> bool read_part(const char *space, uint32_t gen, T &data) {
    Envelope<T> e{};
    size_t size = sizeof(e);
    nvs_handle_t h{};
    if (nvs_open(space, NVS_READONLY, &h) != ESP_OK)
        return false;
    bool ok = nvs_get_blob(h, gen % 2 ? "slot1" : "slot0", &e, &size) == ESP_OK &&
              size == sizeof(e) && e.version == 1 && e.generation == gen &&
              e.crc == checksum({reinterpret_cast<const uint8_t *>(&e.value), sizeof(T)});
    nvs_close(h);
    if (ok)
        std::memcpy(&data, &e.value, sizeof(data));
    return ok;
}
uint32_t generation{};
bool persist(const State &s) {
    if (!valid_current(s))
        return false;
    uint32_t next = generation + 1;
    if (!next)
        return false;
    ConfigRecord c{s.settings, s.calibration, s.battery};
    PetRecord p{s.pet, s.recent, s.meal_key};
    BootRecord b{s.boots, s.reset_reason};
    if (!write_part("hound_settings", next, c) || !write_part("hound_pet", next, p) ||
        !write_part("hound_system", next, b))
        return false;
    nvs_handle_t h{};
    if (nvs_open("system", NVS_READWRITE, &h) != ESP_OK)
        return false;
    bool ok = nvs_set_u32(h, "generation", next) == ESP_OK && nvs_commit(h) == ESP_OK;
    nvs_close(h);
    if (ok)
        generation = next;
    return ok;
}
bool hash_file(const char *path, char output[65]) {
    FILE *input = fopen(path, "rb");
    if (!input)
        return false;
    psa_hash_operation_t op = PSA_HASH_OPERATION_INIT;
    bool ok =
        psa_crypto_init() == PSA_SUCCESS && psa_hash_setup(&op, PSA_ALG_SHA_256) == PSA_SUCCESS;
    std::array<uint8_t, 1024> buf{};
    size_t n;
    while (ok && (n = fread(buf.data(), 1, buf.size(), input))) {
        ok = psa_hash_update(&op, buf.data(), n) == PSA_SUCCESS;
        esp_task_wdt_reset();
    }
    if (ferror(input))
        ok = false;
    fclose(input);
    std::array<uint8_t, 32> digest{};
    size_t len = 0;
    if (ok)
        ok = psa_hash_finish(&op, digest.data(), digest.size(), &len) == PSA_SUCCESS && len == 32;
    psa_hash_abort(&op);
    if (ok)
        for (size_t i = 0; i < 32; ++i)
            std::snprintf(output + i * 2, 3, "%02x", digest[i]);
    return ok;
}
void export_rows() {
    if (!health.mounted || health.ejected || health.read_only)
        return;
    if (!flush())
        return;
    char dir[160];
    std::snprintf(dir, sizeof(dir), "/sd/SURVSNIFF/EXPORT/E-%08lx",
                  static_cast<unsigned long>(esp_random()));
    if (mkdir(dir, 0755) != 0) {
        ++health.errors;
        return;
    }
    char path[220];
    std::snprintf(path, sizeof(path), "%s/SIGHTINGS.JSONL", dir);
    FILE *out = fopen(path, "wx");
    bool ok = out;
    uint32_t rows = 0;
    std::array<uint8_t, 32> export_key{};
    esp_fill_random(export_key.data(), export_key.size());
    DIR *logs = opendir("/sd/SURVSNIFF/LOGS");
    if (!logs)
        ok = false;
    char exported[2048];
    unsigned files = 0;
    if (logs)
        while (ok) {
            auto *e = readdir(logs);
            if (!e)
                break;
            std::string_view name(e->d_name);
            if (!name.ends_with(".JSONL") || name.find('/') != name.npos)
                continue;
            if (++files > 4096) {
                ok = false;
                break;
            }
            char source[300];
            std::snprintf(source, sizeof(source), "/sd/SURVSNIFF/LOGS/%s", e->d_name);
            FILE *input = fopen(source, "rb");
            if (!input) {
                ok = false;
                break;
            }
            while (ok && fgets(line, sizeof(line), input)) {
                esp_task_wdt_reset();
                size_t len = std::strlen(line);
                if (!len || line[len - 1] != '\n') {
                    ok = false;
                    break;
                }
                auto n = export_log_record({line, len}, export_key, exported);
                ok = n && write_all(out, exported, n);
                if (ok)
                    ++rows;
                vTaskDelay(1);
            }
            if (ferror(input))
                ok = false;
            fclose(input);
        }
    if (logs)
        closedir(logs);
    if (out) {
        ok = (fflush(out) == 0) && ok;
        ok = (fsync(fileno(out)) == 0) && ok;
        ok = (fclose(out) == 0) && ok;
    }
    const char *names[] = {"SIGHTINGS.JSONL", "SUMMARY.JSON", "README.TXT"};
    const char *readme =
        "Surveillance Hound complete historical event export. Repeated updates are retained.\n"
        "No location is collected; not map-ready. No raw MAC, SSID, payload, or drone serial.\n"
        "Identifiers are rekeyed per export. Host export tool can aggregate events.\n"
        "Radio clues may be spoofed. Confidence is evidence strength, not proof of surveillance.\n";
    std::snprintf(path, sizeof(path), "%s/README.TXT", dir);
    out = fopen(path, "wx");
    ok = write_all(out, readme, std::strlen(readme)) && ok;
    if (out)
        ok = (fclose(out) == 0) && ok;
    std::snprintf(path, sizeof(path), "%s/SUMMARY.JSON", dir);
    out = fopen(path, "wx");
    int n = std::snprintf(
        line, sizeof(line),
        "{\"schema_version\":1,\"scope\":\"full_history_events\",\"file_limit\":4096,\"rows\":%lu,"
        "\"firmware_version\":\"%s\",\"hardware\":\"E32R40T\",\"signature_pack\":\"%s\"}\n",
        static_cast<unsigned long>(rows), firmware_version, signature_pack_version);
    ok = write_all(out, line, n) && ok;
    if (out)
        ok = (fclose(out) == 0) && ok;
    std::snprintf(path, sizeof(path), "%s/MANIFEST.SHA256", dir);
    FILE *manifest = ok ? fopen(path, "wx") : nullptr;
    ok = bool(manifest) && ok;
    for (auto name : names) {
        std::snprintf(path, sizeof(path), "%s/%s", dir, name);
        char digest[65]{};
        if (!hash_file(path, digest)) {
            ok = false;
            break;
        }
        if (manifest && std::fprintf(manifest, "%s  %s\n", digest, name) < 0)
            ok = false;
    }
    if (manifest)
        ok = (fclose(manifest) == 0) && ok;
    if (ok) {
        ++health.exports;
        health.export_rows = rows;
    } else {
        std::snprintf(path, sizeof(path), "%s/MANIFEST.SHA256", dir);
        unlink(path); // Incomplete exports must never carry a completion manifest.
        ++health.errors;
        health.failed = true;
    }
}
void task(void *) {
    esp_task_wdt_add(nullptr);
    health.mounted = board::mount_sd();
    if (health.mounted) {
        bool dirs = mkdir_ok("/sd/SURVSNIFF") && mkdir_ok("/sd/SURVSNIFF/LOGS") &&
                    mkdir_ok("/sd/SURVSNIFF/EXPORT") && mkdir_ok("/sd/SURVSNIFF/CRASH");
        if (dirs) {
            recover_tails();
            if (!health.failed && !health.read_only)
                open_file();
            space_status();
        } else
            health.failed = true;
    }
    for (;;) {
        esp_task_wdt_reset();
        health.stack_free = uxTaskGetStackHighWaterMark(nullptr);
        Message m{};
        if (xQueueReceive(queue, &m, pdMS_TO_TICKS(100)) == pdTRUE) {
            switch (m.op) {
            case Op::Save:
                if (!persist(m.state)) {
                    ++health.save_errors;
                    health.failed = true;
                }
                break;
            case Op::Record: {
                if (m.detection.demo)
                    break;

                if (!file || health.failed || health.ejected || health.read_only)
                    break;
                size_t n =
                    write_record(m.detection, session_id, m.token, m.research, line, m.region);
                if (!n)
                    break;
                if (size + n > 4 * 1024 * 1024) {
                    flush();
                    fclose(file);
                    file = nullptr;
                    ++rotation;
                    if (!open_file())
                        break;
                }
                if (!write_all(file, line, n)) {
                    health.failed = true;
                    ++health.errors;
                    break;
                }
                size += n;
                ++pending;
                ++health.writes;
                if (pending >= 10)
                    flush();
                break;
            }
            case Op::Sound:
                board::sound(m.region, true);
                break;
            case Op::Export:
                export_rows();
                space_status();
                break;
            case Op::Clear: {
                if (!health.mounted || health.ejected || health.read_only)
                    break;
                flush();
                if (file) {
                    fclose(file);
                    file = nullptr;
                }
                DIR *dir = opendir("/sd/SURVSNIFF/LOGS");
                bool ok = dir;
                if (dir) {
                    while (auto *e = readdir(dir)) {
                        std::string_view name(e->d_name);
                        if (!name.ends_with(".JSONL") || name.find('/') != name.npos)
                            continue;
                        char path[300];
                        std::snprintf(path, sizeof(path), "/sd/SURVSNIFF/LOGS/%s", e->d_name);
                        if (unlink(path) != 0)
                            ok = false;
                        esp_task_wdt_reset();
                        vTaskDelay(1);
                    }
                    closedir(dir);
                }
                if (ok) {

                    rotation = 0;
                    open_file();
                    ++health.clears;
                } else {
                    health.failed = true;
                    ++health.errors;
                }
                space_status();
                break;
            }
            case Op::Sleep:
                if (persist(m.state) && flush()) {
                    if (file) {
                        fclose(file);
                        file = nullptr;
                    }
                    board::sleep();
                } else
                    health.sleep_failed = true;
                break;
            case Op::Eject:
                flush();
                if (file) {
                    fclose(file);
                    file = nullptr;
                }
                health.ejected = true;
                break;
            case Op::Diagnostics:
                if (health.mounted && !health.ejected && !health.read_only) {
                    FILE *f = fopen("/sd/SURVSNIFF/CRASH/LAST-RESET.JSON", "w");
                    if (f) {
                        write_all(f, m.text, std::strlen(m.text));
                        fclose(f);
                    } else
                        ++health.errors;
                }
                break;
            }
        }
        if (pending && uint64_t(esp_timer_get_time() / 1000) - last_flush >= 5000) {
            flush();
            space_status();
        }
    }
}
bool send(const Message &m) {
    if (!queue || xQueueSend(queue, &m, 0) != pdTRUE) {
        ++health.dropped;
        return false;
    }
    return true;
}
} // namespace
bool load(State &state) {
    auto rc = nvs_flash_init();
    if (rc != ESP_OK)
        return false; // Never erase NVS automatically.
    nvs_handle_t handle{};
    if (nvs_open("system", NVS_READWRITE, &handle) != ESP_OK)
        return false;
    State loaded{};
    rc = nvs_get_u32(handle, "generation", &generation);
    if (rc == ESP_OK) {
        nvs_close(handle);
        ConfigRecord c{};
        PetRecord p{};
        BootRecord b{};
        if (!read_part("hound_settings", generation, c) || !read_part("hound_pet", generation, p) ||
            !read_part("hound_system", generation, b))
            return false;
        loaded.settings = c.settings;
        loaded.calibration = c.calibration;
        loaded.battery = c.battery;
        loaded.pet = p.pet;
        loaded.recent = p.recent;
        loaded.meal_key = p.key;
        loaded.boots = b.boots;
        loaded.reset_reason = b.reason;
        if (!valid_current(loaded))
            return false;
        state = loaded;
        state.pet.decay_ms = 0;
        ++state.boots;
        state.reset_reason = esp_reset_reason();
        return persist(state);
    }
    if (rc != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return false;
    }
    StateV2 old{};
    size_t n = sizeof(old);
    rc = nvs_get_blob(handle, "state_v2", &old, &n);
    if (rc == ESP_OK) {
        if (n != sizeof(old) || !migrate(old, loaded)) {
            nvs_close(handle);
            return false;
        }
        n = sizeof(loaded);
    }
    if (rc == ESP_ERR_NVS_NOT_FOUND) {
        StateV1 legacy{};
        size_t old_size = sizeof(legacy);
        rc = nvs_get_blob(handle, "state_v1", &legacy, &old_size);
        if (rc == ESP_OK) {
            if (old_size != sizeof(legacy) || !migrate(legacy, loaded)) {
                nvs_close(handle);
                return false;
            }
            n = sizeof(loaded);
        }
    }
    nvs_close(handle);
    if (rc == ESP_OK && n == sizeof(loaded) && valid_current(loaded)) {
        state = loaded;
        state.pet.decay_ms = 0;

    } else if (rc == ESP_ERR_NVS_NOT_FOUND) {
        esp_fill_random(state.meal_key.data(), state.meal_key.size());
    } else
        return false;
    ++state.boots;
    state.reset_reason = esp_reset_reason();
    return persist(state);
}
bool start(const char *session) {
    std::snprintf(session_id, sizeof(session_id), "%s", session);
    queue = xQueueCreateStatic(8, sizeof(Message), memory, &control);
    return queue &&
           xTaskCreatePinnedToCore(task, "storage", 10240, nullptr, 2, nullptr, 1) == pdPASS;
}
bool append(const Detection &d, const char *token, bool research, uint8_t region) {
    Message m{};
    m.op = Op::Record;
    m.detection = d;
    m.research = research;
    m.region = region;
    std::snprintf(m.token, sizeof(m.token), "%s", token);
    return send(m);
}
bool save(const State &state) {
    Message m{};
    m.op = Op::Save;
    m.state = state;
    return send(m);
}
bool export_history() {
    Message m{};
    m.op = Op::Export;
    return send(m);
}
bool clear_logs() {
    Message m{};
    m.op = Op::Clear;
    return send(m);
}
bool sleep(const State &state) {
    if (!valid_current(state))
        return false;
    Message m{};
    m.op = Op::Sleep;
    m.state = state;
    return send(m);
}
void sound(unsigned effect) {
    Message m{};
    m.op = Op::Sound;
    m.region = effect;
    send(m);
}
bool eject() {
    Message m{};
    m.op = Op::Eject;
    return send(m);
}
void diagnostics(const char *text) {
    Message m{};
    m.op = Op::Diagnostics;
    std::snprintf(m.text, sizeof(m.text), "%s", text);
    send(m);
}
} // namespace sniffer::storage
