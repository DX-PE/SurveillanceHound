// SPDX-License-Identifier: Apache-2.0
#include "sniffer/core.hpp"
#include <algorithm>
#include <cstdio>
#include <ctime>
namespace sniffer {
bool valid_battery(const BatteryCalibration &c) {
    return c.adc[0] >= 50 && c.adc[2] <= 4095 && c.adc[0] + 20 < c.adc[1] &&
           c.adc[1] + 20 < c.adc[2] && c.millivolts[0] >= 2500 && c.millivolts[2] <= 4500 &&
           c.millivolts[0] + 100 < c.millivolts[1] && c.millivolts[1] + 100 < c.millivolts[2];
}
int battery_mv(const BatteryCalibration &c, int adc) {
    if (!valid_battery(c) || adc < 0 || adc > 4095)
        return -1;
    int i = adc < c.adc[1] ? 0 : 1;
    return std::clamp(int(c.millivolts[i]) + (adc - c.adc[i]) *
                                                 (c.millivolts[i + 1] - c.millivolts[i]) /
                                                 (c.adc[i + 1] - c.adc[i]),
                      0, 5000);
}
int battery_percent(int mv) {
    if (mv < 0)
        return -1;
    // Approximate single-cell Li-ion resting-voltage curve, never a capacity measurement.
    constexpr int voltage[] = {3300, 3500, 3650, 3700, 3750, 3800, 3900, 4000, 4100, 4200};
    constexpr int percent[] = {0, 5, 15, 25, 40, 55, 70, 80, 90, 100};
    for (int i = 1; i < 10; ++i)
        if (mv < voltage[i])
            return std::max(0, percent[i - 1] + (mv - voltage[i - 1]) *
                                                    (percent[i] - percent[i - 1]) /
                                                    (voltage[i] - voltage[i - 1]));
    return 100;
}
bool parse_utc(std::string_view s, uint64_t &seconds) {
    if (s.size() != 12)
        return false;
    for (char c : s)
        if (c < '0' || c > '9')
            return false;
    auto n = [&](int i, int size) {
        int v = 0;
        while (size--)
            v = v * 10 + s[i++] - '0';
        return v;
    };
    int year = n(0, 4), month = n(4, 2), day = n(6, 2), hour = n(8, 2), minute = n(10, 2);
    if (year < 2020 || year > 2099 || month < 1 || month > 12 || hour > 23 || minute > 59)
        return false;
    constexpr int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (day < 1 || day > days[month - 1] + (month == 2 && year % 4 == 0))
        return false;
    uint64_t d = 0;
    for (int y = 1970; y < year; ++y)
        d += 365 + (y % 4 == 0);
    for (int m = 1; m < month; ++m)
        d += days[m - 1] + (m == 2 && year % 4 == 0);
    seconds = (d + day - 1) * 86400 + hour * 3600 + minute * 60;
    return true;
}
void format_utc(uint64_t seconds, std::span<char> out) {
    if (out.empty())
        return;
    out[0] = 0;
    if (seconds < 1577836800 || seconds > 4102444799ULL)
        return;
    time_t time = seconds;
    tm value{};
    if (!gmtime_r(&time, &value))
        return;
    std::strftime(out.data(), out.size(), "%Y-%m-%dT%H:%M:%SZ", &value);
}
void DroneCache::ingest(const Observation &o) {
    if (o.radio != Radio::Ble)
        return;
    RemoteId r{};
    if (!parse_remote_id(std::span(o.service).first(o.service_len), r))
        return;
    auto *t = &tracks_[0];
    for (auto &x : tracks_) {
        if (x.present && x.address == o.address && x.address_type == o.address_type) {
            t = &x;
            break;
        }
        if (*std::max_element(x.updated.begin(), x.updated.end()) <
            *std::max_element(t->updated.begin(), t->updated.end()))
            t = &x;
    }
    if (t->address != o.address || t->address_type != o.address_type ||
        o.ms < *std::max_element(t->updated.begin(), t->updated.end()) ||
        o.ms - *std::max_element(t->updated.begin(), t->updated.end()) > 60000)
        *t = {};
    t->address = o.address;
    t->address_type = o.address_type;
    unsigned slot = r.type == 0 ? 0 : r.type == 1 ? 1 : 2;
    for (unsigned i = 0; i < 3; ++i)
        if (t->present & (1U << i))
            if (o.ms < t->updated[i] || o.ms - t->updated[i] > 60000)
                t->present &= ~(1U << i);
    (slot == 0 ? t->basic : slot == 1 ? t->location : t->system) = r;
    t->updated[slot] = o.ms;
    t->present |= 1U << slot;
}
const DroneTrack *DroneCache::find(const Address &a, uint8_t type, uint64_t now) {
    for (auto &x : tracks_)
        if (x.present && x.address == a && x.address_type == type) {
            for (unsigned i = 0; i < 3; ++i)
                if ((x.present & (1U << i)) && (now < x.updated[i] || now - x.updated[i] > 60000))
                    x.present &= ~(1U << i);
            return x.present ? &x : nullptr;
        }
    return nullptr;
}
} // namespace sniffer
