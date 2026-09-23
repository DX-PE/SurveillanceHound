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
#include "history_export.hpp"
#include "history_recovery.hpp"
#include "ignore_backup.hpp"
#include "psa/crypto.h"
#include "signatures_generated.h"
#include "state_store.hpp"
#include "storage_message.hpp"
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
using detail::Message;
using detail::Op;
using detail::Record;
detail::Snapshots snapshots;
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
    // A sector-sized stdio buffer is sufficient for the low-rate JSONL stream;
    // FatFs also retains its sector cache. Keep the existing flush/fsync cadence.
    setvbuf(file, nullptr, _IOFBF, 512);
    size = 0;
    return true;
}
void space_status() {
    if (!health.mounted || health.ejected)
        return; // An export queued after eject must not query the removed card.
    FATFS *fs{};
    DWORD free{};
    if (f_getfree("0:", &free, &fs) == FR_OK) {
        health.free_bytes = uint64_t(free) * fs->csize * 512;
        health.total_bytes = uint64_t(fs->n_fatent - 2) * fs->csize * 512;
    }
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
    if (!health.mounted || health.ejected || health.read_only || health.failed)
        return;
    if (!flush())
        return;
    char dir[160];
    std::snprintf(dir, sizeof(dir), "/sd/SURVSNIFF/EXPORT/E-%08lx",
                  static_cast<unsigned long>(esp_random()));
    std::array<uint8_t, 32> export_key{};
    esp_fill_random(export_key.data(), export_key.size());
    uint32_t rows = 0;
    bool ok = detail::export_history_files(file, filename, "/sd/SURVSNIFF/LOGS", dir, export_key,
                                           line, rows, hash_file, [] {
                                               esp_task_wdt_reset();
                                               vTaskDelay(1);
                                           });
    if (ok) {
        health.export_rows = rows;
        ++health.exports; // Publish completion only after its row count is available.
    } else {
        ++health.errors;
        health.failed = true;
    }
}
// Only the storage worker touches this cache or the card. No extra copy of the list.
std::array<uint8_t, 32> backed_up_digest{};
bool has_ignore_backup{};
void backup_ignores(const State &state, bool force = false) {
    if (!health.mounted) {
        health.ignore_backup = IgnoreBackupStatus::NoCard;
        return;
    }
    if (health.ejected) {
        health.ignore_backup = IgnoreBackupStatus::Ejected;
        return;
    }
    if (health.read_only) {
        health.ignore_backup = IgnoreBackupStatus::ReadOnly;
        return;
    }
    if (health.failed) {
        health.ignore_backup = IgnoreBackupStatus::Failed;
        return;
    }
    const auto &list = state.companion.ignored;
    std::array<uint8_t, 32> digest{};
    if (!sha256({reinterpret_cast<const uint8_t *>(list.data()), sizeof(list)}, digest)) {
        health.ignore_backup = IgnoreBackupStatus::Failed;
        return;
    }
    if (!force && has_ignore_backup && health.ignore_backup == IgnoreBackupStatus::Saved &&
        digest == backed_up_digest) {
        health.ignore_backup = IgnoreBackupStatus::Saved;
        return;
    }
    std::array<char, 17> id{};
    if (!detail::ignore_backup_id(state.meal_key, id)) {
        health.ignore_backup = IgnoreBackupStatus::Failed;
        return;
    }
    char base[80];
    std::snprintf(base, sizeof(base), "/sd/SURVSNIFF/IGNORES/%s", id.data());
    const auto result = detail::backup_ignores(base, state.meal_key, list);
    if (result == detail::BackupResult::Failed) {
        health.ignore_backup = IgnoreBackupStatus::Failed;
        return; // Internal saves and observation logging can continue.
    }
    backed_up_digest = digest;
    has_ignore_backup = true;
    health.ignore_backup_count = state.companion.ignored_count();
    health.ignore_backup = IgnoreBackupStatus::Saved;
}
void task(void *) {
    esp_task_wdt_add(nullptr);
    detail::HistoryRecovery recovery;
    detail::DeferredBackup deferred;
    health.mounted = board::mount_sd();
    if (health.mounted) {
        bool dirs = mkdir_ok("/sd/SURVSNIFF") && mkdir_ok("/sd/SURVSNIFF/LOGS") &&
                    mkdir_ok("/sd/SURVSNIFF/EXPORT") && mkdir_ok("/sd/SURVSNIFF/CRASH") &&
                    mkdir_ok("/sd/SURVSNIFF/IGNORES");
        if (dirs) {
            recovery.begin("/sd/SURVSNIFF/LOGS");
        } else
            health.failed = true;
    }
    bool initialized = false;
    uint32_t recovery_errors = 0;
    auto finish_backup = [&](const State &state, bool manual) {
        backup_ignores(state, manual);
        if (manual)
            ++health.manual_backups;
    };
    for (;;) {
        esp_task_wdt_reset();
        if (recovery.active())
            recovery.step(line);
        health.checked_rows = recovery.rows();
        health.checked_files = recovery.files();
        health.read_only = recovery.read_only();
        health.errors += recovery.errors() - recovery_errors;
        recovery_errors = recovery.errors();
        if (!initialized && !recovery.active()) {
            initialized = true;
            health.checking = false;
            if (health.mounted && !health.failed && !health.read_only && !health.ejected)
                open_file();
            if (deferred.slot >= 0) {
                finish_backup(snapshots.get(size_t(deferred.slot)), deferred.manual);
                deferred.release(snapshots);
            }
        }
        health.stack_free = uxTaskGetStackHighWaterMark(nullptr);
        Message m{};
        if (xQueueReceive(queue, &m, recovery.active() ? 0 : pdMS_TO_TICKS(100)) == pdTRUE) {
            switch (m.op) {
            case Op::Save:
            case Op::BackupIgnores: {
                const auto &state = snapshots.get(m.value);
                bool ok = detail::persist(state);
                if (ok) {
                    ++health.saves;
                    if (recovery.active()) {
                        deferred.retain(snapshots, m.value, m.op == Op::BackupIgnores);
                        health.ignore_backup = IgnoreBackupStatus::Checking;
                    } else
                        finish_backup(state, m.op == Op::BackupIgnores);
                } else {
                    ++health.save_errors;
                    health.ignore_backup = IgnoreBackupStatus::InternalError;
                    health.failed = true;
                    if (m.op == Op::BackupIgnores)
                        ++health.manual_backups;
                }
                if (!ok || !recovery.active())
                    snapshots.release(m.value);
                break;
            }
            case Op::Record: {
                const auto &record = std::get<Record>(m.payload);
                if (record.detection.demo)
                    break;

                if (!file || health.failed || health.ejected || health.read_only)
                    break;
                size_t n = write_record(record.detection, session_id, record.token.data(),
                                        record.research, line, record.region);
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
                board::sound(m.value, true);
                break;
            case Op::Export:
                export_rows();
                space_status();
                break;
            case Op::Clear: {
                if (recovery.active())
                    break;
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
            case Op::Sleep: {
                const bool was_checking = recovery.active();
                bool closed = recovery.cancel();
                initialized = true;
                health.checking = false;
                deferred.release(snapshots);
                bool ok = detail::persist(snapshots.get(m.value)) && closed;
                if (ok && !was_checking)
                    backup_ignores(snapshots.get(m.value));
                snapshots.release(m.value);
                if (ok && flush()) {
                    if (file) {
                        fclose(file);
                        file = nullptr;
                    }
                    board::sleep();
                } else
                    health.sleep_failed = true;
                break;
            }
            case Op::Eject: {
                // Serialize behind exports/records and stop all future card I/O even
                // on failure. Never turn a failed close into success on a second tap.
                if (health.ejected)
                    break;
                // Close the startup reader as well, before publishing SAFE TO REMOVE.
                bool ok = recovery.cancel();
                ok = detail::close_log(file) && ok;
                initialized = true;
                health.checking = false;
                const bool cancelled_backup = deferred.manual;
                deferred.release(snapshots);
                pending = 0;
                if (!ok) {
                    health.failed = true;
                    ++health.errors;
                }
                health.eject_failed = !ok;
                health.ignore_backup = IgnoreBackupStatus::Ejected;
                health.ejected = true; // Publish only after the result and closed writer.
                if (cancelled_backup)
                    ++health.manual_backups;
                break;
            }
            case Op::Diagnostics:
                if (!recovery.active() && health.mounted && !health.ejected && !health.read_only) {
                    FILE *f = fopen("/sd/SURVSNIFF/CRASH/LAST-RESET.JSON", "w");
                    if (f) {
                        const auto &text = std::get<std::array<char, 256>>(m.payload);
                        write_all(f, text.data(), std::strlen(text.data()));
                        fclose(f);
                    } else
                        ++health.errors;
                }
                break;
            }
        }
        if (recovery.active())
            vTaskDelay(1); // Yield per batch, not once per historical row.
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
bool send_snapshot(Op op, const State &state) {
    int slot = snapshots.claim(state);
    if (slot < 0) {
        ++health.dropped;
        return false;
    }
    Message m{};
    m.op = op;
    m.value = uint8_t(slot);
    if (send(m))
        return true;
    snapshots.release(slot);
    return false;
}
} // namespace
bool start(const char *session) {
    std::snprintf(session_id, sizeof(session_id), "%s", session);
    queue = xQueueCreateStatic(8, sizeof(Message), memory, &control);
    health.checking = true;
    health.ignore_backup = IgnoreBackupStatus::Checking;
    if (queue && xTaskCreatePinnedToCore(task, "storage", 10240, nullptr, 2, nullptr, 1) == pdPASS)
        return true;
    queue = nullptr;
    health.checking = false;
    health.failed = true;
    health.ignore_backup = IgnoreBackupStatus::Unavailable;
    return false;
}
bool append(const Detection &d, const char *token, bool research, uint8_t region) {
    if (health.checking) {
        ++health.dropped; // Startup sightings cannot displace Save/Eject requests.
        return false;
    }
    Message m{};
    m.op = Op::Record;
    auto &record = m.payload.emplace<Record>();
    record.detection = d;
    record.research = research;
    record.region = region;
    std::snprintf(record.token.data(), record.token.size(), "%s", token);
    return send(m);
}
bool save(const State &state, bool backup) {
    return send_snapshot(backup ? Op::BackupIgnores : Op::Save, state);
}
bool export_history() {
    if (health.checking)
        return false;
    Message m{};
    m.op = Op::Export;
    return send(m);
}
bool clear_logs() {
    if (health.checking)
        return false;
    Message m{};
    m.op = Op::Clear;
    return send(m);
}
bool sleep(const State &state) {
    if (!valid_current(state))
        return false;
    return send_snapshot(Op::Sleep, state);
}
void sound(unsigned effect) {
    Message m{};
    m.op = Op::Sound;
    m.value = effect;
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
    auto &buffer = m.payload.emplace<std::array<char, 256>>();
    std::snprintf(buffer.data(), buffer.size(), "%s", text);
    send(m);
}
} // namespace sniffer::storage
