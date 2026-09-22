// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "sniffer/core.hpp"
#include <algorithm>

namespace sniffer {
// Presence heuristic only: no location, motion inference or rotating-address correlation.
// RAM-only, keyed identities. Packet floods cannot accelerate the time requirement.
struct TagWatch {
    static constexpr uint64_t duration = 600000, gap = 120000;
    static constexpr unsigned required_minutes = 8;
    struct Entry {
        uint64_t hash{}, first{}, last{};
        uint16_t minutes{};
        Category category{};
        uint8_t score{};
        bool acknowledged{};
        bool fresh(uint64_t now) const {
            return hash && now >= last && now - last <= gap;
        }
        bool ready(uint64_t now) const {
            return fresh(now) && last >= first && last - first >= duration &&
                   minutes >= required_minutes;
        }
    };
    std::array<Entry, 16> entries{};
    uint64_t started{};
    uint32_t gap_resets{}, filter_resets{}, clock_skips{};
    bool armed{}, demo{}, flash{true};
    static bool tag(Category c) {
        return c == Category::AIRTAG || c == Category::SAMSUNG_TAG || c == Category::TILE ||
               c == Category::GOOGLE_TAG;
    }
    void start(uint64_t now, bool synthetic) {
        entries = {};
        started = now;
        demo = synthetic;
        armed = true;
        gap_resets = filter_resets = clock_skips = 0;
    }
    void clear() {
        entries = {};
        armed = false;
    }
    Entry *find(uint64_t hash) {
        for (auto &e : entries)
            if (hash && e.hash == hash)
                return &e;
        return nullptr;
    }
    void drop(Entry &e, bool gap_expired) {
        if (!e.hash)
            return;
        auto &counter = gap_expired ? gap_resets : filter_resets;
        if (counter != UINT32_MAX)
            ++counter;
        e = {};
    }
    void observe(uint64_t hash, const Detection &d, uint64_t now) {
        if (!armed || !hash || !tag(d.category) || d.radio != Radio::Ble || d.demo != demo ||
            d.score < 50 || d.score > 100 || d.last_ms < started)
            return;
        if (d.last_ms > now) {
            if (clock_skips != UINT32_MAX)
                ++clock_skips;
            return;
        }
        if (now - d.last_ms > gap)
            return;
        auto *e = find(hash);
        if (e && d.last_ms <= e->last)
            return;
        if (!e) {
            // Do not evict a still-present identity just because a crowd fills the table.
            for (auto &candidate : entries)
                if (!candidate.fresh(now)) {
                    e = &candidate;
                    break;
                }
            if (!e)
                return;
            *e = {};
        }
        if (!e->hash || d.last_ms - e->last > gap) {
            if (e->hash)
                drop(*e, true);
            *e = {hash, d.last_ms, d.last_ms, 1, d.category, d.score, false};
            return;
        }
        // Count occupied minute bins, not packets or the engine's cumulative seen_count.
        if ((d.last_ms - e->first) / 60000 != (e->last - e->first) / 60000)
            e->minutes = std::min<unsigned>(65535, unsigned(e->minutes) + 1);
        e->last = d.last_ms;
        e->score = d.score;
    }
    const Entry *warning(uint64_t now) const {
        if (armed)
            for (const auto &e : entries)
                if (e.ready(now) && !e.acknowledged)
                    return &e;
        return nullptr;
    }
};
static_assert(sizeof(TagWatch) <= 544, "Tag Watch must stay within its small RAM budget");
} // namespace sniffer
