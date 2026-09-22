// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "sniffer/core.hpp"
#include <algorithm>
namespace sniffer {
enum class ScentSignal { None, Paused, Waiting, Fresh, Stale, Lost };
// One volatile identity, sampled at most once per second. No address correlation or RF changes.
struct FollowScent {
    struct Sample {
        uint64_t ms{};
        int8_t rssi{};
    };
    Address address{};
    Radio radio{};
    uint8_t address_type{};
    Category category{};
    bool active{}, demo{}, received{};
    uint64_t started{}, last_ms{}, accept_after{};
    int8_t rssi{};
    std::array<Sample, 30> history{};
    size_t count{};
    void select(const Detection &d, uint64_t now) {
        *this = {};
        if (size_t(d.category) >= category_count)
            return;
        active = true;
        demo = d.demo;
        address = d.address;
        radio = d.radio;
        address_type = d.address_type;
        category = d.category;
        started = accept_after = now;
    }
    void restart(uint64_t now) {
        received = false;
        count = 0;
        started = accept_after = now;
    }
    bool observe(const Observation &o, bool synthetic) {
        if (!active || synthetic != demo || o.address != address || o.radio != radio ||
            o.address_type != address_type || o.ms <= accept_after ||
            (received && o.ms < last_ms) || o.rssi > 0 || o.rssi < -126)
            return false;
        received = true;
        last_ms = o.ms;
        rssi = o.rssi;
        if (count && history[count - 1].ms / 1000 == o.ms / 1000)
            history[count - 1] = {o.ms, o.rssi};
        else {
            if (count == history.size()) {
                std::move(history.begin() + 1, history.end(), history.begin());
                --count;
            }
            history[count++] = {o.ms, o.rssi};
        }
        return true;
    }
    ScentSignal state(uint64_t now, bool listening) const {
        if (!active)
            return ScentSignal::None;
        if (!listening)
            return ScentSignal::Paused;
        if (!received)
            return now >= started && now - started >= 15000 ? ScentSignal::Lost
                                                            : ScentSignal::Waiting;
        if (now < last_ms || now - last_ms >= 15000)
            return ScentSignal::Lost;
        return now - last_ms <= 5000 ? ScentSignal::Fresh : ScentSignal::Stale;
    }
    // Compare recent one-second bins only; gaps do not imply a rising/falling signal.
    int trend(uint64_t now) const {
        if (count < 2 || now < last_ms || now - last_ms > 5000)
            return 0;
        for (size_t i = count - 1; i > 0; --i) {
            if (history[i].ms - history[i - 1].ms > 5000)
                break;
            auto age = last_ms - history[i - 1].ms;
            if (age >= 2000 && age <= 8000)
                return int(rssi) - history[i - 1].rssi;
        }
        return 0;
    }
};
} // namespace sniffer
