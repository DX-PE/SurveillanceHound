// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
namespace sniffer {
// Frozen layout used by the first V5 appearance records.
struct AppearanceV1 {
    uint32_t version{1};
    uint8_t theme{}, scene{}, speech{1}, compact{};
};
struct Appearance {
    uint32_t version{2};
    uint8_t theme{}, scene{}, speech{1}, compact{}, inverted{};
};
inline bool valid_appearance(const Appearance &a) {
    return a.version == 2 && a.theme < 3 && a.scene < 3 && a.speech < 3 && a.compact < 2 &&
           a.inverted < 2;
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
inline constexpr const char *theme_names[] = {"MIDNIGHT", "SUNSET", "DAYLIGHT"};
inline constexpr const char *scene_names[] = {"SIGNAL GRID", "DOG PARK", "ROOFTOPS"};
inline constexpr const char *speech_names[] = {"OFF", "OCCASIONAL", "CHATTY"};
} // namespace sniffer
