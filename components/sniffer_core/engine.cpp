// SPDX-License-Identifier: Apache-2.0
#include "sniffer/core.hpp"
#include <algorithm>
#include <cstring>
#include <limits>

namespace sniffer {
namespace {
int hex(char c) {
    return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}
bool hex_prefix(std::string_view value, std::span<const uint8_t> data) {
    if (value.empty() || value.size() % 2 || value.size() / 2 > data.size())
        return false;
    for (size_t i = 0; i < value.size() / 2; ++i) {
        int a = hex(value[2 * i]), b = hex(value[2 * i + 1]);
        if (a < 0 || b < 0 || data[i] != (a * 16 + b))
            return false;
    }
    return true;
}
size_t group(Kind k) {
    if (k == Kind::SsidPrefix)
        k = Kind::SsidExact;
    if (k == Kind::NamePrefix)
        k = Kind::NameExact;
    return static_cast<size_t>(k);
}
uint8_t cap(const Rule &rule, const Observation &o) {
    uint8_t limit = rule.cap;
    if (rule.generic || (rule.kind == Kind::Oui && (o.address[0] & 2)))
        limit = std::min<uint8_t>(limit, 49);
    if (rule.kind == Kind::NameExact || rule.kind == Kind::NamePrefix ||
        rule.kind == Kind::SsidExact || rule.kind == Kind::SsidPrefix)
        limit = std::min<uint8_t>(limit, 79);
    return limit;
}
} // namespace
bool matches(const Rule &r, const Observation &o) {
    std::string_view v(r.value);
    std::string_view name(o.name.data());
    std::string_view ssid(reinterpret_cast<const char *>(o.ssid.data()), o.ssid_len);
    switch (r.kind) {
    case Kind::Oui:
        return o.radio == Radio::Wifi && hex_prefix(v, o.address);
    case Kind::SsidExact:
        return o.radio == Radio::Wifi && ssid == v;
    case Kind::SsidPrefix:
        return o.radio == Radio::Wifi && ssid.starts_with(v);
    case Kind::NameExact:
        return o.radio == Radio::Ble && name == v;
    case Kind::NamePrefix:
        return o.radio == Radio::Ble && name.starts_with(v);
    case Kind::Uuid16: {
        if (o.radio != Radio::Ble || v.size() != 4)
            return false;
        for (size_t i = 0; i < o.uuid_count; ++i) {
            std::array<uint8_t, 2> big{uint8_t(o.uuids[i] >> 8), uint8_t(o.uuids[i])};
            if (hex_prefix(v, big))
                return true;
        }
        return false;
    }
    case Kind::Uuid128:
        return o.radio == Radio::Ble && v.size() == 32 && o.uuid128_count &&
               hex_prefix(v, o.uuids128[0]);
    case Kind::Company: {
        if (o.radio != Radio::Ble || o.manufacturer_len < 2)
            return false;
        std::array<uint8_t, 2> big{o.manufacturer[1], o.manufacturer[0]};
        return hex_prefix(v, big);
    }
    case Kind::Payload:
        return o.radio == Radio::Wifi
                   ? hex_prefix(v, std::span(o.vendor).first(o.vendor_len))
                   : hex_prefix(v, std::span(o.manufacturer).first(o.manufacturer_len));
    case Kind::Protocol:
        if (v == "pwnagotchi")
            return o.radio == Radio::Wifi && o.pwnagotchi;
        if (o.radio != Radio::Ble)
            return false;
        if (v == "apple_find_my")
            return o.manufacturer_len == 29 && hex_prefix("4C001219", o.manufacturer);
        if (v == "ibeacon")
            return o.manufacturer_len == 25 && hex_prefix("4C000215", o.manufacturer) &&
                   int8_t(o.manufacturer[24]) >= -126 && int8_t(o.manufacturer[24]) <= 20;
        if (v == "find_hub")
            return (o.service_len == 23 || o.service_len == 24) && hex_prefix("AAFE", o.service) &&
                   (o.service[2] == 0x40 || o.service[2] == 0x41);
        if (v == "fast_pair_model")
            return o.service_len == 5 && hex_prefix("2CFE", o.service);
        if (v == "remote_id") {
            RemoteId rid{};
            return parse_remote_id(std::span(o.service).first(o.service_len), rid);
        }
        return false;
    default:
        return false;
    }
}
bool Engine::behavior(const Rule &r, const Observation &o) {
    if (o.radio != Radio::Wifi)
        return false;
    if (std::string_view(r.value) == "deauth_burst") {
        if (o.subtype != 10 && o.subtype != 12)
            return false;
        Burst *b = nullptr;
        for (auto &x : bursts_)
            if (x.used && x.address == o.address && x.channel == o.channel) {
                b = &x;
                break;
            }
        if (!b) {
            b = &*std::min_element(bursts_.begin(), bursts_.end(),
                                   [](auto &a, auto &z) { return a.last < z.last; });
            for (auto &x : bursts_)
                if (!x.used) {
                    b = &x;
                    break;
                }
            *b = {};
            b->used = true;
            b->address = o.address;
            b->channel = o.channel;
        }
        b->times[b->next] = o.ms;
        b->next = (b->next + 1) % b->times.size();
        b->count = std::min<int>(20, b->count + 1);
        b->last = o.ms;
        return b->count == 20 && o.ms >= b->times[b->next] && o.ms - b->times[b->next] <= 10000;
    }
    if (std::string_view(r.value) == "security_downgrade" ||
        std::string_view(r.value) == "security_corroborated") {
        if ((o.subtype != 8 && o.subtype != 5) || !o.ssid_len)
            return false;
        for (auto &a : aps_) {
            if (!a.used || o.ms < a.obs.ms || o.ms - a.obs.ms > 30000 || a.obs.address == o.address)
                continue;
            if (o.ssid_len == a.obs.ssid_len &&
                std::equal(o.ssid.begin(), o.ssid.begin() + o.ssid_len, a.obs.ssid.begin()) &&
                o.protected_network != a.obs.protected_network) {
                if (std::string_view(r.value) == "security_downgrade")
                    return true;
                else {
                    unsigned clues =
                        (a.obs.address[0] != o.address[0] || a.obs.address[1] != o.address[1] ||
                         a.obs.address[2] != o.address[2]);
                    clues += (a.obs.rsn_caps & 0xc0) != (o.rsn_caps & 0xc0);
                    clues += a.obs.vendor_fingerprint && o.vendor_fingerprint &&
                             a.obs.vendor_fingerprint != o.vendor_fingerprint;
                    clues += a.obs.channel != o.channel && std::abs(int(a.obs.rssi) - o.rssi) >= 20;
                    if (clues >= 2)
                        return true;
                }
            }
        }
    }
    return false;
}
void Engine::remember_ap(const Observation &o) {
    if (o.radio != Radio::Wifi || (o.subtype != 8 && o.subtype != 5) || !o.ssid_len || o.pwnagotchi)
        return;
    AccessPoint *dest = nullptr;
    for (auto &a : aps_)
        if (a.used && a.obs.address == o.address) {
            dest = &a;
            break;
        }
    if (!dest) {
        dest = &*std::min_element(aps_.begin(), aps_.end(),
                                  [](auto &a, auto &b) { return a.obs.ms < b.obs.ms; });
        for (auto &a : aps_)
            if (!a.used) {
                dest = &a;
                break;
            }
    }
    dest->used = true;
    dest->obs = {o.ms,       o.address, o.ssid,    o.ssid_len,          o.protected_network,
                 o.rsn_caps, o.rssi,    o.channel, o.vendor_fingerprint};
}
size_t Engine::ingest(const Observation &o, const Settings &settings, std::span<Detection> out) {
    std::array<Entry *, category_count> touched{};
    for (const auto &rule : rules_) {
        const size_t c = static_cast<size_t>(rule.category);
        if (!(settings.enabled_categories & (1U << c)))
            continue;
        if (!(rule.kind == Kind::Behavior ? behavior(rule, o) : matches(rule, o)))
            continue;
        Entry *e = touched[c];
        if (!e) {
            for (auto &candidate : cache_) {
                const auto &d = candidate.detection;
                if (candidate.used && d.address == o.address && d.category == rule.category &&
                    d.radio == o.radio && d.address_type == o.address_type) {
                    e = &candidate;
                    break;
                }
            }
            if (!e) {
                e = &*std::min_element(cache_.begin(), cache_.end(), [](auto &a, auto &b) {
                    return a.detection.last_ms < b.detection.last_ms;
                });
                for (auto &candidate : cache_)
                    if (!candidate.used) {
                        e = &candidate;
                        break;
                    }
                *e = {};
                e->used = true;
                auto &d = e->detection;
                d.category = rule.category;
                d.radio = o.radio;
                d.address = o.address;
                d.address_type = o.address_type;
                d.first_ms = o.ms;
                d.rssi_min = o.rssi;
                d.rssi_max = o.rssi;
            }
            auto &d = e->detection;
            if (o.ms < d.last_ms || o.ms - d.last_ms > 30000)
                e->evidence.fill({});
            d.last_ms = o.ms;
            d.channel = o.channel;
            d.rssi_min = std::min(d.rssi_min, o.rssi);
            d.rssi_max = std::max(d.rssi_max, o.rssi);
            if (d.seen_count != std::numeric_limits<uint32_t>::max()) {
                ++d.seen_count;
                d.rssi_sum += o.rssi;
            }
            touched[c] = e;
        }
        auto &ev = e->evidence[group(rule.kind)];
        if (!ev.rule || uint32_t(o.ms) - ev.at > 30000 || rule.score >= ev.rule->score)
            ev = {&rule, uint32_t(o.ms)};
    }
    remember_ap(o);
    size_t n = 0;
    for (auto *e : touched) {
        if (!e)
            continue;
        auto &d = e->detection;
        uint8_t old_score = d.score;
        std::array<const Rule *, kind_count> active{};
        size_t count = 0;
        for (auto &ev : e->evidence)
            if (ev.rule && uint32_t(o.ms) - ev.at <= 30000)
                active[count++] = ev.rule;
        std::sort(active.begin(), active.begin() + count, [&](auto *a, auto *b) {
            return std::min(a->score, cap(*a, o)) > std::min(b->score, cap(*b, o));
        });
        d.rule_count = static_cast<uint8_t>(std::min(count, d.rules.size()));
        d.rules.fill(nullptr);
        unsigned score = 0, ceiling = 0;
        bool strongest_specific = false;
        for (size_t i = 0; i < count; ++i) {
            const auto &r = *active[i];
            ceiling = std::max<unsigned>(ceiling, cap(r, o));
            unsigned s = std::min(r.score, cap(r, o));
            if (i == 0) {
                score = s;
                strongest_specific = !r.generic;
            }
            // An additional generic clue is not independent product evidence.
            else if (i == 1 && strongest_specific && !r.generic)
                score += s / 4;
            if (i < d.rules.size())
                d.rules[i] = &r;
        }
        score = std::min(score, ceiling);
        if (o.contradiction)
            score = score > 20 ? score - 20 : 1;
        d.score = static_cast<uint8_t>(score);
        const bool due = !e->emitted || o.ms - e->emitted >= 30000 ||
                         (d.score >= 80   ? 3
                          : d.score >= 50 ? 2
                          : d.score >= 20 ? 1
                                          : 0) > (old_score >= 80   ? 3
                                                  : old_score >= 50 ? 2
                                                  : old_score >= 20 ? 1
                                                                    : 0);
        if (due && o.ms >= next_emit_ && n < out.size()) {
            out[n++] = d;
            e->emitted = std::max<uint64_t>(1, o.ms);
            next_emit_ = o.ms + 100;
        }
    }
    return n;
}
std::array<uint32_t, 3> Engine::recent_counts(uint64_t now, const Settings &settings) const {
    std::array<uint32_t, 3> counts{};
    auto active = [&](const Entry &e) {
        const auto &d = e.detection;
        return e.used && d.score >= 20 && now >= d.last_ms && now - d.last_ms <= 90000 &&
               (settings.enabled_categories & (1U << unsigned(d.category)));
    };
    for (size_t i = 0; i < cache_.size(); ++i) {
        if (!active(cache_[i]))
            continue;
        const auto &d = cache_[i].detection;
        bool already_counted = false;
        for (size_t j = 0; j < cache_.size(); ++j) {
            if (i == j || !active(cache_[j]))
                continue;
            const auto &other = cache_[j].detection;
            if (d.address == other.address && d.radio == other.radio &&
                d.address_type == other.address_type &&
                (other.score > d.score || (other.score == d.score && j < i))) {
                already_counted = true;
                break;
            }
        }
        if (!already_counted)
            ++counts[d.score >= 80 ? 0 : d.score >= 50 ? 1 : 2];
    }
    return counts;
}
void Pet::tick(uint64_t now) {
    if (now < decay_ms)
        decay_ms = now;
    uint64_t hours = (now - decay_ms) / 3600000;
    if (!hours)
        return;
    fullness = static_cast<uint8_t>(fullness > hours ? fullness - hours : 0);
    mood = static_cast<uint8_t>(mood > hours + 20 ? mood - hours : 20);
    decay_ms += hours * 3600000;
}
void Pet::feed(const Detection &d, bool low) {
    if (d.demo || !d.meal || (d.score < 50 && !low))
        return;
    const auto c = static_cast<size_t>(d.category);
    uint32_t bonus = (category_mask & (1U << c)) ? 0 : 3;
    category_mask |= 1U << c;
    xp += (d.score >= 80 ? 10 : d.score >= 50 ? 5 : 1) + bonus;
    fullness = std::min<int>(100, fullness + (d.score >= 80 ? 12 : d.score >= 50 ? 7 : 0));
    mood = std::min<int>(100, mood + 3);
    ++meals;
    ++lifetime_meals[c];
}
void Pet::stroke() {
    mood = std::min<int>(100, mood + 2);
}
bool MealGuard::award(uint64_t hash, uint64_t now, Category category, uint8_t score, bool low) {
    if (!hash || score < (low ? 20 : 50))
        return false;
    size_t c = static_cast<size_t>(category);
    if (now < global_until_ || now < category_until_[c])
        return false;
    for (auto &s : stamps)
        if (s.hash == hash && now < s.remaining_ms)
            return false;
    auto *dest = &*std::min_element(stamps.begin(), stamps.end(), [](auto &a, auto &b) {
        return a.remaining_ms < b.remaining_ms;
    });
    for (auto &s : stamps)
        if (s.hash == hash) {
            dest = &s;
            break;
        }
    *dest = {hash, now + (category == Category::DEAUTH ? 600000U : 1800000U)};
    global_until_ = now + 5000;
    category_until_[c] = now + 30000;
    return true;
}
void MealGuard::restore(std::span<const MealStamp> saved) {
    stamps.fill({});
    size_t n = 0;
    for (auto s : saved)
        if (s.hash && s.remaining_ms && n < stamps.size()) {
            // No trusted wall clock: retain a conservative full cooldown after each boot.
            stamps[n++] = {s.hash, 1800000};
        }
}
std::array<MealStamp, 32> MealGuard::snapshot(uint64_t now) const {
    auto sorted = stamps;
    std::sort(sorted.begin(), sorted.end(),
              [](auto &a, auto &b) { return a.remaining_ms > b.remaining_ms; });
    std::array<MealStamp, 32> result{};
    for (size_t i = 0; i < result.size(); ++i)
        if (sorted[i].remaining_ms > now)
            result[i] = {sorted[i].hash, sorted[i].remaining_ms - now};
    return result;
}
} // namespace sniffer
