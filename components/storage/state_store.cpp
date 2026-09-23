// SPDX-License-Identifier: Apache-2.0
#include "state_store.hpp"
#include "esp_random.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "storage.hpp"
#include <cstring>
namespace sniffer::storage {
namespace {
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
// V3 has a smaller boot record. V4 requires companion data; V5 also requires appearance.
struct BootRecordV3 {
    uint32_t boots, reason;
};
struct BootRecord {
    uint32_t boots, reason, version{5};
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
// Keep read/migration temporaries off the stack while persist() calls into NVS.
// Without this boundary the old and new companion envelopes can remain reserved
// throughout the boot write, crowding the 8 KiB main task stack.
[[gnu::noinline]] bool read_generation(uint32_t gen, State &loaded) {
    ConfigRecord c{};
    PetRecord p{};
    BootRecord b{};
    if (!read_part("hound_settings", gen, c) || !read_part("hound_pet", gen, p))
        return false;
    if (read_part("hound_system", gen, b)) {
        if (b.version != 4 && b.version != 5)
            return false;
        if (!read_part("hound_companion", gen, loaded.companion)) {
            CompanionV1 old{};
            if (!read_part("hound_companion", gen, old) ||
                !migrate_companion(old, loaded.companion))
                return false;
        }
        if (b.version == 5 && (!read_part("hound_look", gen, loaded.appearance) ||
                               !valid_appearance(loaded.appearance))) {
            AppearanceV2 v2{};
            AppearanceV1 v1{};
            if (!(read_part("hound_look", gen, v2) && migrate_appearance(v2, loaded.appearance)) &&
                !(read_part("hound_look", gen, v1) && migrate_appearance(v1, loaded.appearance)))
                return false;
        }
    } else {
        BootRecordV3 legacy{};
        if (!read_part("hound_system", gen, legacy))
            return false;
        b.boots = legacy.boots;
        b.reason = legacy.reason;
        loaded.companion.unlock(p.pet.xp);
    }
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
    return true;
}
template <class Legacy>
[[gnu::noinline]] int read_legacy(nvs_handle_t handle, const char *key, State &loaded) {
    Legacy old{};
    size_t size = sizeof(old);
    auto rc = nvs_get_blob(handle, key, &old, &size);
    if (rc == ESP_OK && (size != sizeof(old) || !migrate(old, loaded)))
        return ESP_FAIL;
    return rc;
}
uint32_t generation{};
} // namespace
bool detail::persist(const State &s) {
    if (!valid_current(s))
        return false;
    uint32_t next = generation + 1;
    if (!next)
        return false;
    ConfigRecord c{s.settings, s.calibration, s.battery};
    PetRecord p{s.pet, s.recent, s.meal_key};
    BootRecord b{s.boots, s.reset_reason};
    if (!write_part("hound_settings", next, c) || !write_part("hound_pet", next, p) ||
        !write_part("hound_system", next, b) || !write_part("hound_companion", next, s.companion) ||
        !write_part("hound_look", next, s.appearance))
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
        if (!read_generation(generation, loaded))
            return false;
        state = loaded;
        state.pet.decay_ms = 0;
        ++state.boots;
        state.reset_reason = esp_reset_reason();
        return detail::persist(state);
    }
    if (rc != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return false;
    }
    rc = read_legacy<StateV2>(handle, "state_v2", loaded);
    if (rc == ESP_ERR_NVS_NOT_FOUND)
        rc = read_legacy<StateV1>(handle, "state_v1", loaded);
    nvs_close(handle);
    if (rc == ESP_OK && valid_current(loaded)) {
        state = loaded;
        state.pet.decay_ms = 0;

    } else if (rc == ESP_ERR_NVS_NOT_FOUND) {
        esp_fill_random(state.meal_key.data(), state.meal_key.size());
    } else
        return false;
    ++state.boots;
    state.reset_reason = esp_reset_reason();
    return detail::persist(state);
}
} // namespace sniffer::storage
