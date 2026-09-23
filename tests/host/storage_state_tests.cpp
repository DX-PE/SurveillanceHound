// SPDX-License-Identifier: Apache-2.0
#include "nvs.h"
#include "state_store.hpp"
#include "storage.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using namespace sniffer;
namespace {
unsigned checks{}, step{}, cut_after{}, fail_before{};
struct PowerCut {};
using Bytes = std::vector<uint8_t>;
using Values = std::map<std::string, Bytes>;
std::map<std::string, Values> flash;
struct Handle {
    std::string space;
    Values pending;
};
std::map<unsigned, Handle> handles;
unsigned next_handle{};
bool buffered{};
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::cerr << "line " << __LINE__ << ": " #x "\n";                                      \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)
bool start_write() {
    return ++step != fail_before;
}
void end_write() {
    if (step == cut_after)
        throw PowerCut{};
}
void restart() {
    handles.clear();
    step = cut_after = fail_before = 0;
}
void same_saved(const storage::State &a, const storage::State &b) {
    CHECK(a.settings.name == b.settings.name);
    CHECK(a.settings.brightness == b.settings.brightness);
    CHECK(a.settings.portrait == b.settings.portrait);
    CHECK(a.pet.xp == b.pet.xp && a.pet.fullness == b.pet.fullness && a.pet.mood == b.pet.mood);
    CHECK(a.pet.meals == b.pet.meals && a.pet.decay_ms == 0);
    CHECK(a.companion.equipped == b.companion.equipped);
    CHECK(a.companion.unlocked == b.companion.unlocked);
    CHECK(a.companion.ignored == b.companion.ignored);
    CHECK(a.companion.ignored[0].category == b.companion.ignored[0].category);
    CHECK(a.meal_key == b.meal_key);
    CHECK(a.calibration.ax == b.calibration.ax && a.calibration.valid == b.calibration.valid);
    CHECK(a.appearance.inverted == b.appearance.inverted);
    CHECK(a.appearance.dim_minutes == b.appearance.dim_minutes);
    CHECK(a.appearance.saver_minutes == b.appearance.saver_minutes);
    CHECK(a.appearance.off_minutes == b.appearance.off_minutes);
}
} // namespace
int nvs_flash_init() {
    return ESP_OK;
}
unsigned esp_reset_reason() {
    return 1;
}
void esp_fill_random(void *p, size_t n) {
    std::memset(p, 0x5a, n);
}
int nvs_open(const char *space, int mode, nvs_handle_t *out) {
    if (mode == NVS_READONLY && !flash.contains(space))
        return ESP_ERR_NVS_NOT_FOUND;
    *out = ++next_handle;
    handles[*out] = {space, {}};
    return ESP_OK;
}
void nvs_close(nvs_handle_t h) {
    handles.erase(h);
}
int nvs_set_blob(nvs_handle_t h, const char *key, const void *data, size_t n) {
    if (!start_write())
        return ESP_FAIL;
    const auto *bytes = static_cast<const uint8_t *>(data);
    auto &handle = handles.at(h);
    (buffered ? handle.pending : flash[handle.space])[key] = Bytes(bytes, bytes + n);
    end_write();
    return ESP_OK;
}
int nvs_get_blob(nvs_handle_t h, const char *key, void *data, size_t *n) {
    const auto &values = flash[handles.at(h).space];
    auto it = values.find(key);
    if (it == values.end())
        return ESP_ERR_NVS_NOT_FOUND;
    auto capacity = *n;
    *n = it->second.size();
    if (capacity < *n)
        return ESP_ERR_NVS_INVALID_LENGTH;
    std::memcpy(data, it->second.data(), *n);
    return ESP_OK;
}
int nvs_set_u32(nvs_handle_t h, const char *key, uint32_t value) {
    return nvs_set_blob(h, key, &value, sizeof(value));
}
int nvs_get_u32(nvs_handle_t h, const char *key, uint32_t *value) {
    size_t n = sizeof(*value);
    return nvs_get_blob(h, key, value, &n);
}
int nvs_commit(nvs_handle_t h) {
    if (!start_write())
        return ESP_FAIL;
    auto &handle = handles.at(h);
    for (const auto &[key, value] : handle.pending)
        flash[handle.space][key] = value;
    handle.pending.clear();
    end_write();
    return ESP_OK;
}
int main() {
    storage::State original;
    CHECK(storage::load(original));
    original.settings.onboarded = true;
    original.settings.name = {'R', 'O', 'V', 'E', 'R', 0};
    original.settings.brightness = 60;
    original.pet.fullness = 44;
    original.pet.mood = 55;
    original.pet.xp = 100;
    original.pet.meals = 7;
    original.companion.unlock(original.pet.xp);
    original.companion.equipped = 1;
    CHECK(original.companion.ignore(1234, Category::FLIPPER));
    original.calibration.valid = true;
    original.calibration.ax = 0.123f;
    CHECK(storage::detail::persist(original));
    const auto baseline = flash;
    auto changed = original;
    changed.settings.brightness = 80;
    changed.settings.portrait = true;
    changed.settings.name = {'S', 'C', 'O', 'U', 'T', 0};
    changed.pet.fullness = 15;
    changed.pet.mood = 21;
    changed.pet.xp = 350;
    changed.pet.meals = 17;
    changed.pet.decay_ms = 999999;
    changed.companion.unlock(changed.pet.xp);
    changed.companion.equipped = 4;
    changed.companion.ignored[0] = {5678, Category::FLIPPER};
    CHECK(changed.companion.ignore(9999, Category::AIRTAG));
    changed.appearance.inverted = true;
    changed.appearance.dim_minutes = 1;
    changed.appearance.saver_minutes = 10;
    changed.appearance.off_minutes = 0;
    CHECK(storage::valid_current(changed));
    // The pinned SDK writes in nvs_set_*; nvs_commit is currently a no-op.
    // Also model future buffered commits. Both must recover one whole generation.
    for (bool mode : {false, true}) {
        buffered = mode;
        for (unsigned boundary = 1; boundary <= 12; ++boundary) {
            flash = baseline;
            restart();
            storage::State before;
            CHECK(storage::load(before));
            step = 0;
            cut_after = boundary;
            bool interrupted = false;
            try {
                storage::detail::persist(changed);
            } catch (const PowerCut &) {
                interrupted = true;
            }
            CHECK(interrupted);
            restart();
            storage::State recovered;
            CHECK(storage::load(recovered));
            same_saved(recovered, boundary >= (buffered ? 12U : 11U) ? changed : original);
        }
        // A failure before publication retains the old generation; a failed commit
        // after immediate publication can already have saved the complete new one.
        for (unsigned failure = 1; failure <= 12; ++failure) {
            flash = baseline;
            restart();
            storage::State before;
            CHECK(storage::load(before));
            step = 0;
            fail_before = failure;
            CHECK(!storage::detail::persist(changed));
            restart();
            storage::State recovered;
            CHECK(storage::load(recovered));
            same_saved(recovered, !buffered && failure == 12 ? changed : original);
            CHECK(storage::detail::persist(changed));
            restart();
            CHECK(storage::load(recovered));
            same_saved(recovered, changed);
        }
    }
    // Corruption in every required namespace must fail closed and leave the
    // existing flash untouched instead of resetting user progress/preferences.
    for (auto space :
         {"hound_settings", "hound_pet", "hound_system", "hound_companion", "hound_look"}) {
        flash = baseline;
        restart();
        uint32_t generation{};
        const auto &g = flash.at("system").at("generation");
        std::memcpy(&generation, g.data(), sizeof(generation));
        auto &blob = flash.at(space).at(generation % 2 ? "slot1" : "slot0");
        blob.back() ^= 0x80;
        const auto corrupt = flash;
        storage::State rejected;
        CHECK(!storage::load(rejected));
        CHECK(flash == corrupt);
    }
    // Real V1/V2 envelope sizes, CRCs and dirty legacy padding. New timeouts must
    // come from defaults, never the unused bytes of an earlier appearance save.
    buffered = false;
    for (uint32_t version : {1U, 2U, 3U, 99U}) {
        flash = baseline;
        restart();
        uint32_t gen{};
        const auto &g = flash.at("system").at("generation");
        std::memcpy(&gen, g.data(), sizeof(gen));
        auto &blob = flash.at("hound_look").at(gen % 2 ? "slot1" : "slot0");
        blob.resize(version == 1 ? 20 : 24);
        std::memcpy(blob.data() + 12, &version, 4);
        blob[16] = 2; // theme
        blob[17] = 1; // dog park
        blob[18] = 2; // speech
        blob[19] = 1; // compact
        if (version != 1) {
            blob[20] = 1; // inversion
            blob[21] = 0xa7;
            blob[22] = 0xff;
            blob[23] = 0x34;
        }
        uint32_t crc = ~0U;
        for (size_t i = 12; i < blob.size(); ++i) {
            crc ^= blob[i];
            for (int bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ (0xedb88320U & -(crc & 1U));
        }
        crc = ~crc;
        std::memcpy(blob.data() + 8, &crc, 4);
        const auto old_flash = flash;
        storage::State migrated;
        if (version > 2) {
            CHECK(!storage::load(migrated)); // Valid CRC cannot excuse invalid timeouts/version.
            CHECK(flash == old_flash);
            continue;
        }
        CHECK(storage::load(migrated));
        auto expected = original;
        expected.appearance.inverted = version == 2;
        same_saved(migrated, expected);
        CHECK(migrated.appearance.version == 3 && migrated.appearance.theme == 2 &&
              migrated.appearance.scene == 1 && migrated.appearance.speech == 2 &&
              migrated.appearance.compact == 1);
        restart();
        storage::State reloaded;
        CHECK(storage::load(reloaded));
        same_saved(reloaded, migrated);
    }
    // Frozen 728-byte V1 companion records migrate all 16 ignores, preserving
    // hashes/categories/outfit/scents. A power cut at every migration write must
    // leave the old or new complete generation loadable.
    flash = baseline;
    restart();
    uint32_t legacy_gen{};
    std::memcpy(&legacy_gen, flash.at("system").at("generation").data(), 4);
    auto expected = original;
    CompanionV1 old{};
    old.scents = original.companion.scents;
    old.scents[0].observations = 33;
    old.equipped = original.companion.equipped;
    old.unlocked = original.companion.unlocked;
    for (size_t i = 0; i < old.ignored.size(); ++i)
        old.ignored[i] = {0x123456789abc0000ULL + i, Category(i % category_count)};
    CHECK(migrate_companion(old, expected.companion));
    struct OldEnvelope {
        uint32_t version{1}, generation{}, crc{};
        CompanionV1 value{};
    } env{};
    static_assert(sizeof(OldEnvelope) == 744);
    auto install_old = [&](const CompanionV1 &value) {
        env.generation = legacy_gen;
        env.value = value;
        uint32_t crc = ~0U;
        for (auto byte :
             std::span(reinterpret_cast<const uint8_t *>(&env.value), sizeof(env.value))) {
            crc ^= byte;
            for (unsigned bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ (0xedb88320U & -(crc & 1U));
        }
        env.crc = ~crc;
        const auto *bytes = reinterpret_cast<const uint8_t *>(&env);
        flash["hound_companion"][legacy_gen % 2 ? "slot1" : "slot0"] =
            Bytes(bytes, bytes + sizeof(env));
    };
    install_old(old);
    const auto legacy_flash = flash;
    for (bool mode : {false, true}) {
        buffered = mode;
        for (unsigned boundary = 1; boundary <= 12; ++boundary) {
            flash = legacy_flash;
            restart();
            cut_after = boundary;
            storage::State interrupted;
            try {
                storage::load(interrupted);
            } catch (const PowerCut &) {
            }
            restart();
            storage::State recovered;
            CHECK(storage::load(recovered));
            same_saved(recovered, expected);
            CHECK(recovered.companion.version == 2 && recovered.companion.ignored_count() == 16);
            CHECK(recovered.companion.scents[0].observations == 33);
            for (unsigned i = 16; i < ignore_capacity; ++i)
                CHECK(recovered.companion.ignore(1000 + i, Category::AIRTAG));
            CHECK(recovered.companion.ignored_count() == ignore_capacity);
            CHECK(storage::detail::persist(recovered));
            restart();
            storage::State full;
            CHECK(storage::load(full));
            same_saved(full, recovered);
        }
    }
    for (unsigned invalid = 0; invalid < 4; ++invalid) {
        flash = legacy_flash;
        restart();
        auto broken = old;
        if (invalid == 0)
            broken.version = 2;
        if (invalid == 1)
            broken.ignored[1] = broken.ignored[0];
        if (invalid == 2)
            broken.ignored[3].category = Category(255);
        if (invalid == 3)
            broken.equipped = 255;
        install_old(broken);
        const auto before = flash;
        storage::State rejected;
        CHECK(!storage::load(rejected));
        CHECK(flash == before);
    }
    // Old monolithic records use separate bounded-stack readers as well.
    auto legacy_round_trip = [&](const auto &record, const char *key) {
        flash.clear();
        restart();
        const auto *bytes = reinterpret_cast<const uint8_t *>(&record);
        flash["system"][key] = Bytes(bytes, bytes + sizeof(record));
        storage::State recovered;
        CHECK(storage::load(recovered));
        CHECK(recovered.meal_key == record.meal_key && recovered.pet.xp == record.pet.xp);
        CHECK(recovered.settings.name == record.settings.name);
        CHECK(recovered.companion.version == 2 && !recovered.companion.ignored_count());
        restart();
        storage::State reloaded;
        CHECK(storage::load(reloaded));
        same_saved(reloaded, recovered);
        flash.clear();
        restart();
        flash["system"][key] = Bytes(bytes, bytes + sizeof(record) - 1);
        const auto before = flash;
        CHECK(!storage::load(reloaded));
        CHECK(flash == before);
    };
    storage::StateV2 old_state2;
    old_state2.meal_key.fill(0x72);
    old_state2.pet.xp = 350;
    old_state2.settings.name = {'R', 'O', 'V', 'E', 'R', 0};
    legacy_round_trip(old_state2, "state_v2");
    storage::StateV1 old_state1;
    old_state1.meal_key.fill(0x36);
    old_state1.pet.xp = 400;
    legacy_round_trip(old_state1, "state_v1");
    CHECK(handles.empty());
    std::cout << checks << " production persistence and interrupted-save checks passed\n";
}
