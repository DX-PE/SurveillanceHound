// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "board.hpp"
#include "sniffer/appearance.hpp"
#include "sniffer/companion.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace sniffer::storage {
struct State {
    uint32_t version{5};
    Settings settings{};
    Pet pet{};
    board::Calibration calibration{};
    std::array<MealStamp, 32> recent{};
    std::array<uint8_t, 32> meal_key{};
    uint32_t boots{}, reset_reason{};
    BatteryCalibration battery{};
    Companion companion{};
    Appearance appearance{};
};
// Frozen v2 layout for pre-completion Hound saves.
struct StateV2 {
    uint32_t version{2};
    Settings settings{};
    Pet pet{};
    board::Calibration calibration{};
    std::array<MealStamp, 32> recent{};
    std::array<uint8_t, 32> meal_key{};
    uint32_t boots{}, reset_reason{};
};
// Frozen v1 layout for reading pre-Hound saves. Never write new data using this layout.
struct SettingsV1 {
    uint32_t version{1};
    uint32_t enabled_categories{(1U << category_count) - 1U};
    uint8_t threshold{50}, region{0}, brightness{70}, character{0};
    bool sound{}, low_feeding{}, reduced_animation{}, research{}, raw_ssid{}, onboarded{};
    bool ibeacon_alerts{};
    std::array<char, 13> name{'S', 'C', 'O', 'U', 'T', 0};
};
struct StateV1 {
    uint32_t version{1};
    SettingsV1 settings{};
    Pet pet{};
    board::Calibration calibration{};
    std::array<MealStamp, 32> recent{};
    std::array<uint8_t, 32> meal_key{};
    uint32_t boots{}, reset_reason{};
};
template <class S> inline bool valid_state(const S &s, uint32_t version) {
    const auto &v = s.settings;
    for (const bool *p : {&v.sound, &v.low_feeding, &v.reduced_animation, &v.research, &v.raw_ssid,
                          &v.onboarded, &v.ibeacon_alerts})
        if (*reinterpret_cast<const uint8_t *>(p) > 1)
            return false;
    if (*reinterpret_cast<const uint8_t *>(&s.calibration.valid) > 1)
        return false;
    if (s.calibration.valid)
        for (float f : {s.calibration.ax, s.calibration.bx, s.calibration.cx, s.calibration.ay,
                        s.calibration.by, s.calibration.cy})
            if (!std::isfinite(f) || std::abs(f) > 100000)
                return false;
    if (s.version != version || v.version != (version == 1 ? 1U : 2U) || v.character >= 6 ||
        v.region >= 2 || v.brightness > 100 || v.brightness < 1 || v.name.back() != 0 ||
        s.pet.fullness > 100 || s.pet.mood > 100)
        return false;
    if (v.threshold != 20 && v.threshold != 50 && v.threshold != 80)
        return false;
    if (v.enabled_categories >> category_count)
        return false;
    for (char c : v.name)
        if (c && (c < 32 || c > 126))
            return false;
    return true;
}
inline bool valid_current(const State &s) {
    if (!valid_state(s, 5) || !valid_companion(s.companion) || !valid_appearance(s.appearance))
        return false;
    for (const bool *p : {&s.settings.portrait, &s.settings.rotation_locked, &s.battery.enabled})
        if (*reinterpret_cast<const uint8_t *>(p) > 1)
            return false;
    if (s.settings.alert_categories >> category_count)
        return false;
    if (s.battery.enabled && !valid_battery(s.battery))
        return false;
    for (auto stamp : s.recent)
        if (stamp.remaining_ms > 1800000)
            return false;
    return std::any_of(s.meal_key.begin(), s.meal_key.end(), [](auto b) { return b != 0; });
}
inline bool migrate(const StateV2 &old, State &out) {
    if (!valid_state(old, 2))
        return false;
    if (*reinterpret_cast<const uint8_t *>(&old.settings.portrait) > 1 ||
        *reinterpret_cast<const uint8_t *>(&old.settings.rotation_locked) > 1 ||
        (old.settings.alert_categories >> category_count))
        return false;
    State next{};
    next.settings = old.settings;
    next.pet = old.pet;
    next.calibration = old.calibration;
    next.recent = old.recent;
    next.meal_key = old.meal_key;
    next.boots = old.boots;
    next.reset_reason = old.reset_reason;
    out = next;
    return true;
}
inline bool migrate(const StateV1 &old, State &result) {
    if (!valid_state(old, 1))
        return false;
    State next{};
    next.settings.enabled_categories = old.settings.enabled_categories;
    next.settings.threshold = old.settings.threshold;
    next.settings.region = old.settings.region;
    next.settings.brightness = old.settings.brightness;
    next.settings.character = old.settings.character;
    next.settings.sound = old.settings.sound;
    next.settings.low_feeding = old.settings.low_feeding;
    next.settings.reduced_animation = old.settings.reduced_animation;
    next.settings.research = old.settings.research;
    next.settings.raw_ssid = old.settings.raw_ssid;
    next.settings.onboarded = old.settings.onboarded;
    next.settings.ibeacon_alerts = old.settings.ibeacon_alerts;
    if (old.settings.ibeacon_alerts)
        next.settings.alert_categories |= 1U << unsigned(Category::IBEACON);
    next.settings.name = old.settings.name;
    next.pet = old.pet;
    next.calibration = old.calibration;
    next.recent = old.recent;
    next.meal_key = old.meal_key;
    next.boots = old.boots;
    next.reset_reason = old.reset_reason;
    result = next;
    return true;
}
} // namespace sniffer::storage
