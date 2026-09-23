// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "sniffer/core.hpp"
#include <algorithm>
#include <bit>
#include <limits>

namespace sniffer {
constexpr size_t outfit_count = 6;
constexpr const char *outfit_names[] = {"CLASSIC",  "TRAIL BANDANA", "BRASS COLLAR",
                                        "RAINCOAT", "DETECTIVE CAP", "SUNGLASSES"};
constexpr const char *outfit_goals[] = {"READY TO WEAR",     "EARN 25 XP",  "EARN 100 XP",
                                        "DISCOVER 5 SCENTS", "EARN 300 XP", "DISCOVER 10 SCENTS"};
struct ScentEntry {
    uint64_t first_utc{}, last_utc{};
    uint32_t observations{};
    uint8_t best_score{}, last_score{};
    int8_t strongest{-127};
};
// Byte storage avoids seven padding bytes per entry and unaligned uint64_t loads.
constexpr size_t ignore_capacity = 64;
enum class IgnoreBackupStatus : uint8_t {
    Waiting,
    Checking,
    Unavailable,
    Saved,
    NoCard,
    Ejected,
    ReadOnly,
    Failed,
    InternalError
};
struct IgnoredScent {
    std::array<uint8_t, 8> digest{};
    Category category{};
    constexpr IgnoredScent() = default;
    constexpr IgnoredScent(uint64_t value, Category type) : category(type) {
        for (unsigned i = 0; i < 8; ++i)
            digest[i] = uint8_t(value >> (8 * i));
    }
    constexpr uint64_t hash() const {
        uint64_t value = 0;
        for (unsigned i = 0; i < 8; ++i)
            value |= uint64_t(digest[i]) << (8 * i);
        return value;
    }
    bool operator==(const IgnoredScent &) const = default;
};
static_assert(sizeof(IgnoredScent) == 9 && alignof(IgnoredScent) == 1);
using IgnoreList = std::array<IgnoredScent, ignore_capacity>;
inline bool valid_ignores(const IgnoreList &list) {
    for (size_t i = 0; i < list.size(); ++i) {
        if (size_t(list[i].category) >= category_count)
            return false;
        for (size_t j = 0; list[i].hash() && j < i; ++j)
            if (list[j].hash() == list[i].hash())
                return false;
    }
    return true;
}
// Frozen raw NVS layout. Migration never changes an existing keyed identity.
struct CompanionV1 {
    uint32_t version{1};
    std::array<ScentEntry, category_count> scents{};
    struct Entry {
        uint64_t hash{};
        Category category{};
    };
    std::array<Entry, 16> ignored{};
    uint8_t equipped{}, unlocked{1};
};
static_assert(sizeof(CompanionV1) == 728);
struct Companion {
    uint32_t version{2};
    std::array<ScentEntry, category_count> scents{};
    IgnoreList ignored{};
    uint8_t equipped{}, unlocked{1};
    unsigned discoveries() const {
        unsigned n = 0;
        for (const auto &s : scents)
            n += s.observations != 0;
        return n;
    }
    void observe(const Detection &d) {
        if (size_t(d.category) >= scents.size())
            return;
        auto &s = scents[size_t(d.category)];
        if (!s.observations)
            s.first_utc = d.unix_seconds;
        if (s.observations < std::numeric_limits<uint32_t>::max())
            ++s.observations;
        s.last_utc = d.unix_seconds;
        s.last_score = d.score;
        s.best_score = std::max(s.best_score, d.score);
        s.strongest = std::max(s.strongest, d.rssi_max);
    }
    uint8_t unlock(uint32_t xp) {
        uint8_t earned = 1 | (xp >= 25 ? 2 : 0) | (xp >= 100 ? 4 : 0) |
                         (discoveries() >= 5 ? 8 : 0) | (xp >= 300 ? 16 : 0) |
                         (discoveries() >= 10 ? 32 : 0);
        uint8_t added = earned & ~unlocked;
        unlocked |= earned;
        return added;
    }
    bool is_ignored(uint64_t hash) const {
        if (!hash)
            return false;
        for (const auto &entry : ignored)
            if (entry.hash() == hash)
                return true;
        return false;
    }
    bool ignore(uint64_t hash, Category category) {
        if (!hash || size_t(category) >= category_count)
            return false;
        if (is_ignored(hash))
            return true;
        for (auto &entry : ignored)
            if (!entry.hash()) {
                entry = {hash, category};
                return true;
            }
        return false; // Never silently evict another ignored device.
    }
    unsigned ignored_count() const {
        unsigned count = 0;
        for (const auto &entry : ignored)
            count += entry.hash() != 0;
        return count;
    }
};
inline bool valid_companion(const Companion &c) {
    if (c.version != 2 || c.equipped >= outfit_count || !(c.unlocked & 1) ||
        (c.unlocked >> outfit_count) || !(c.unlocked & (1U << c.equipped)))
        return false;
    for (const auto &s : c.scents)
        if (s.best_score > 100 || s.last_score > s.best_score || s.first_utc > 4102444799ULL ||
            s.last_utc > 4102444799ULL)
            return false;
    return valid_ignores(c.ignored);
}
inline bool migrate_companion(const CompanionV1 &old, Companion &out) {
    if (old.version != 1)
        return false;
    Companion next{};
    next.scents = old.scents;
    next.equipped = old.equipped;
    next.unlocked = old.unlocked;
    for (size_t i = 0; i < old.ignored.size(); ++i)
        next.ignored[i] = {old.ignored[i].hash, old.ignored[i].category};
    if (!valid_companion(next))
        return false;
    out = next;
    return true;
}
static_assert(sizeof(Companion) == 1048);

} // namespace sniffer
