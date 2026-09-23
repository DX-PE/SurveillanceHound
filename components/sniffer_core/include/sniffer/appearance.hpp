// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
namespace sniffer {
// Frozen layout used by the first V5 appearance records.
struct AppearanceV1 {
    uint32_t version{1};
    uint8_t theme{}, scene{}, speech{1}, compact{};
};
struct AppearanceV2 {
    uint32_t version{2};
    uint8_t theme{}, scene{}, speech{1}, compact{}, inverted{};
};
// V3 consumes V2's three padding bytes without increasing its 12-byte record.
// Never read those bytes as timeouts when migrating a V2 record.
struct Appearance {
    uint32_t version{3};
    uint8_t theme{}, scene{}, speech{1}, compact{}, inverted{};
    uint8_t dim_minutes{2}, saver_minutes{5}, off_minutes{15};
};
static_assert(sizeof(Appearance) == 12 && sizeof(AppearanceV2) == 12);
inline constexpr uint8_t idle_timeouts[] = {0, 1, 2, 5, 10, 15, 30, 60};
inline bool valid_timeout(uint8_t minutes) {
    for (auto value : idle_timeouts)
        if (minutes == value)
            return true;
    return false;
}
inline uint8_t next_timeout(uint8_t minutes) {
    for (unsigned i = 0; i < sizeof(idle_timeouts); ++i)
        if (idle_timeouts[i] == minutes)
            return idle_timeouts[(i + 1) % sizeof(idle_timeouts)];
    return 0;
}
inline bool valid_appearance(const Appearance &a) {
    return a.version == 3 && a.theme < 3 && a.scene < 3 && a.speech < 3 && a.compact < 2 &&
           a.inverted < 2 && valid_timeout(a.dim_minutes) && valid_timeout(a.saver_minutes) &&
           valid_timeout(a.off_minutes);
}
inline bool migrate_appearance(const AppearanceV1 &old, Appearance &out) {
    if (old.version != 1 || old.theme >= 3 || old.scene >= 3 || old.speech >= 3 || old.compact >= 2)
        return false;
    Appearance next{};
    next.theme = old.theme;
    next.scene = old.scene;
    next.speech = old.speech;
    next.compact = old.compact;
    out = next;
    return true;
}
inline bool migrate_appearance(const AppearanceV2 &old, Appearance &out) {
    if (old.version != 2 || old.inverted >= 2)
        return false;
    AppearanceV1 base{1, old.theme, old.scene, old.speech, old.compact};
    Appearance next{};
    if (!migrate_appearance(base, next))
        return false;
    next.inverted = old.inverted;
    out = next;
    return true;
}
inline constexpr const char *theme_names[] = {"MIDNIGHT", "SUNSET", "DAYLIGHT"};
inline constexpr const char *scene_names[] = {"SIGNAL GRID", "DOG PARK", "ROOFTOPS"};
inline constexpr const char *speech_names[] = {"OFF", "OCCASIONAL", "CHATTY"};
} // namespace sniffer
