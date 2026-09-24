// SPDX-License-Identifier: Apache-2.0
#include "signatures_generated.h"
#include "sniffer/core.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using namespace sniffer;
static int checks;
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(c)) {                                                                                \
            std::cerr << __LINE__ << ": " #c "\n";                                                 \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
Rule rule(const char *id, Kind kind, const char *value, uint8_t score = 60, uint8_t cap = 79,
          bool generic = false) {
    return {id,      Category::FLOCK, kind,
            value,   score,           cap,
            generic, "Possible test", "Synthetic test evidence"};
}
std::vector<uint8_t> beacon(const char *ssid = "LAB", bool protected_network = true) {
    std::vector<uint8_t> b(36);
    b[0] = 0x80;
    b[10] = 0x10;
    b[15] = 1;
    b[34] = protected_network ? 0x10 : 0;
    b.push_back(0);
    b.push_back(uint8_t(std::strlen(ssid)));
    b.insert(b.end(), ssid, ssid + std::strlen(ssid));
    return b;
}
void recent_count_tests() {
    Rule rules[] = {
        {"test.tag", Category::SAMSUNG_TAG, Kind::Payload, "01", 85, 100, false, "", ""},
        {"test.apple", Category::AIRTAG, Kind::Payload, "02", 65, 100, false, "", ""},
        {"test.promotion", Category::AIRTAG, Kind::Payload, "03", 85, 100, false, "", ""},
        {"test.possible", Category::AIRTAG, Kind::Payload, "04", 30, 100, false, "", ""},
        {"test.overlap", Category::FLOCK, Kind::Payload, "01", 50, 79, false, "", ""},
    };
    Engine engine(rules);
    Settings s;
    uint64_t now = 1000;
    auto sighting = [&](uint8_t id, uint8_t clue, uint8_t type = 0, Radio radio = Radio::Ble) {
        Observation o{};
        o.ms = now;
        o.radio = radio;
        o.address = {2, 0, 0, 0, 0, id};
        o.address_type = type;
        o.manufacturer[0] = o.vendor[0] = clue;
        o.manufacturer_len = o.vendor_len = 1;
        std::array<Detection, 4> out{};
        engine.ingest(o, s, out);
        now += 1000;
    };
    CHECK((engine.recent_counts(now, s) == std::array<uint32_t, 3>{0, 0, 0}));
    // Two tags and one Apple-family signal stay three through repeated emissions.
    for (int i = 0; i < 100; ++i) {
        sighting(1, 1);
        sighting(2, 1);
        sighting(3, 2);
        CHECK((engine.recent_counts(now, s) == std::array<uint32_t, 3>{2, 1, 0}));
    }
    s.alert_categories = 0; // Muting does not hide a signal that is still present.
    s.sound = false;
    CHECK((engine.recent_counts(now, s) == std::array<uint32_t, 3>{2, 1, 0}));
    s.enabled_categories &= ~(1U << unsigned(Category::SAMSUNG_TAG));
    CHECK((engine.recent_counts(now, s) == std::array<uint32_t, 3>{0, 3, 0}));
    s.enabled_categories = (1U << category_count) - 1U;
    sighting(3, 3);
    CHECK((engine.recent_counts(now, s) == std::array<uint32_t, 3>{3, 0, 0}));
    now += 31000;
    sighting(3,
             4); // Latest weaker evidence moves buckets, rather than adding one.
    CHECK((engine.recent_counts(now, s) == std::array<uint32_t, 3>{2, 0, 1}));
    sighting(3, 3, 1);
    sighting(3, 3, 0, Radio::Wifi);
    CHECK((engine.recent_counts(now, s) == std::array<uint32_t, 3>{4, 0, 1}));
    CHECK((engine.recent_counts(now - 1000 + 90000, s) == std::array<uint32_t, 3>{1, 0, 0}));
    CHECK((engine.recent_counts(now + 90000, s) == std::array<uint32_t, 3>{0, 0, 0}));
    CHECK((engine.recent_counts(0, s) == std::array<uint32_t, 3>{0, 0, 0}));
    // Capacity is bounded by the existing cache; no heap allocation or total
    // overflow.
    for (int i = 0; i < 200; ++i) {
        sighting(uint8_t(i), 3);
        now -= 990; // Fit the cache-pressure case inside the recent window.
    }
    auto counts = engine.recent_counts(now + 1000, s);
    CHECK(counts[0] == 128 && counts[1] == 0 && counts[2] == 0);
}
void needs_tests() {
    Pet p;
    p.tick(Pet::needs_interval_ms - 1);
    CHECK(p.fullness == 70 && p.mood == 70);
    p.tick(Pet::needs_interval_ms);
    CHECK(p.fullness == 67 && p.mood == 68);
    p.tick(Pet::needs_interval_ms);
    CHECK(p.fullness == 67 && p.mood == 68);
    p.tick(Pet::needs_interval_ms * 15);
    CHECK(p.fullness == 25 && p.mood == 40);
    p.tick(Pet::needs_interval_ms * 40);
    CHECK(p.fullness == 0 && p.mood == 0);
    p.tick(100); // Rebase after clock reset without decay or underflow.
    CHECK(p.decay_ms == 100 && p.fullness == 0 && p.mood == 0);
    Detection d{};
    d.category = Category::SAMSUNG_TAG;
    d.score = 60;
    d.meal = true;
    d.last_ms = 200;
    p.feed(d);
    CHECK(p.fullness == 35 && p.mood == 40 && p.decay_ms == 200);
    CHECK(p.meals == 1);
    p.tick(200 + Pet::needs_interval_ms - 1);
    CHECK(p.fullness == 35 && p.mood == 40);
    p.tick(200 + Pet::needs_interval_ms);
    CHECK(p.fullness == 32 && p.mood == 38);
    auto saved = p;
    d.meal = false;
    p.feed(d);
    d.meal = true;
    d.demo = true;
    p.feed(d);
    CHECK(p.fullness == saved.fullness && p.mood == saved.mood && p.meals == saved.meals);
    d.demo = false;
    d.score = 20;
    p.feed(d);
    CHECK(p.meals == saved.meals);
    p.feed(d, true);
    CHECK(p.meals == saved.meals + 1 && p.fullness == 38 && p.mood == 44);
    p.fullness = p.mood = 99;
    d.score = 85;
    p.feed(d);
    CHECK(p.fullness == 100 && p.mood == 100);
    p.tick(UINT64_MAX);
    CHECK(p.fullness == 0 && p.mood == 0);
    p.stroke();
    CHECK(p.fullness == 0 && p.mood == 2);
}
void wifi_inference_tests() {
    Settings settings;
    std::array<Detection, 4> out{};
    const Rule exact = rule("test.ssid", Kind::SsidExact, "Pineapple_LAB");
    const Rule prefix = rule("test.prefix", Kind::SsidPrefix, "Pineapple_");
    // Parse actual frame layouts, including the shorter probe-request header.
    for (uint8_t subtype : {8, 5, 4}) {
        auto bytes = beacon("Pineapple_LAB");
        bytes[0] = subtype << 4;
        if (subtype == 4)
            bytes.erase(bytes.begin() + 24, bytes.begin() + 36);
        Observation o{};
        o.ms = 1000;
        CHECK(parse_wifi(bytes, o));
        CHECK(o.subtype == subtype && o.ssid_len == 13);
        const bool hosted = subtype != 4;
        CHECK(matches(exact, o) == hosted);
        CHECK(matches(prefix, o) == hosted);
        Engine engine(production_rules);
        CHECK(engine.ingest(o, settings, out) == size_t(hosted));
        CHECK(engine.recent_counts(o.ms, settings)[1] == unsigned(hosted));
        if (hosted)
            CHECK(out[0].category == Category::PINEAPPLE);
    }
    // The frame-role guard must preserve literal byte matching, including the
    // 32-byte boundary, rather than matching the sanitized display name.
    for (uint8_t subtype : {8, 5}) {
        for (const auto &ssid : {std::string{}, std::string{"pineapple_LAB"},
                                 std::string{"xPineapple_LAB"}, std::string{"Pineapple\0_LAB", 14},
                                 std::string{"Pineapple_"} + std::string(22, 'X')}) {
            auto bytes = beacon("");
            bytes[0] = subtype << 4;
            bytes[37] = uint8_t(ssid.size());
            bytes.insert(bytes.end(), ssid.begin(), ssid.end());
            Observation sample{};
            CHECK(parse_wifi(bytes, sample));
            CHECK(sample.ssid_len == ssid.size());
            CHECK(!matches(exact, sample));
            CHECK(matches(prefix, sample) == (ssid.size() == 32));
            auto malformed = bytes;
            malformed.insert(malformed.end(), {0, 1, 'X'});
            CHECK(!parse_wifi(malformed, sample)); // Duplicate SSID IE.
            bytes.pop_back();
            CHECK(!parse_wifi(bytes, sample)); // Truncated length or SSID.
        }
    }
    // A locally administered source loses only the MAC manufacturer clue.
    // A vendor identifier carried in an IE is not a transmitter address.
    const Rule clues[] = {rule("test.local", Kind::Oui, "122030"), prefix,
                          rule("test.payload", Kind::Payload, "6A5C35")};
    auto bytes = beacon("Pineapple_LAB");
    bytes[10] = 0x12;
    bytes[11] = 0x20;
    bytes[12] = 0x30;
    bytes.insert(bytes.end(), {221, 3, 0x6a, 0x5c, 0x35});
    Observation o{};
    o.ms = 1000;
    CHECK(parse_wifi(bytes, o));
    CHECK(!matches(clues[0], o) && matches(clues[1], o) && matches(clues[2], o));
    Engine local(clues);
    CHECK(local.ingest(o, settings, out) == 1);
    CHECK(out[0].rule_count == 2 && out[0].score == 75);
    CHECK((local.recent_counts(o.ms, settings) == std::array<uint32_t, 3>{0, 1, 0}));

    // Test matching prefixes for all four U/L + I/G combinations: no masking
    // a local/group address back into a globally assigned manufacturer prefix.
    for (auto [first, value] :
         {std::pair{0x10, "102030"}, {0x11, "112030"}, {0x12, "122030"}, {0x13, "132030"}}) {
        const auto vendor = rule("test.vendor", Kind::Oui, value);
        o = {};
        o.address = {uint8_t(first), 0x20, 0x30, 0, 0, 1};
        o.ms = 1000;
        Engine engine(std::span(&vendor, 1));
        CHECK(matches(vendor, o) == (first == 0x10));
        CHECK(engine.ingest(o, settings, out) == size_t(first == 0x10));
        CHECK(engine.recent_counts(o.ms, settings)[1] == unsigned(first == 0x10));
        o.radio = Radio::Ble;
        CHECK(!matches(vendor, o));
    }
    // BLE's address type and payload evidence are independent of Wi-Fi OUI inference.
    o = {};
    o.radio = Radio::Ble;
    o.address = {0x12, 0x20, 0x30, 0, 0, 1};
    o.address_type = 1;
    const std::array<uint8_t, 4> company{3, 0xff, 0x4c, 0};
    CHECK(parse_ble(company, o));
    CHECK(matches(rule("test.company", Kind::Company, "004C"), o));
}
int main() {
    wifi_inference_tests();
    needs_tests();
    recent_count_tests();
    Observation o{};
    auto b = beacon();
    CHECK(parse_wifi(b, o));
    CHECK(o.ssid_len == 3 && o.protected_network);
    for (size_t n = 0; n < 36; ++n)
        CHECK(!parse_wifi(std::span(b).first(n), o));
    auto malformed = b;
    malformed.push_back(1);
    CHECK(!parse_wifi(malformed, o));
    malformed = b;
    malformed[37] = 33;
    CHECK(!parse_wifi(malformed, o));
    std::array<uint8_t, 5> ble{4, 0x16, 0x2c, 0xfe, 1};
    CHECK(parse_ble(ble, o));
    std::array<uint8_t, 4> badble{5, 0xff, 0, 0};
    CHECK(!parse_ble(badble, o));
    std::array<uint8_t, 3> odd_uuid{2, 3, 1};
    CHECK(!parse_ble(odd_uuid, o));
    std::array<uint8_t, 6> fastpair{5, 0x16, 0x2c, 0xfe, 1, 2};
    CHECK(parse_ble(fastpair, o));
    const Rule *fp = nullptr;
    const Rule *ib = nullptr;
    const Rule *fh = nullptr;
    for (const auto &r : production_rules) {
        if (std::string_view(r.value) == "fast_pair_model")
            fp = &r;
        if (std::string_view(r.value) == "ibeacon")
            ib = &r;
        if (std::string_view(r.value) == "find_hub")
            fh = &r;
    }
    CHECK(fp && ib && fh);
    CHECK(!matches(*fp, o));
    std::array<uint8_t, 7> validfp{6, 0x16, 0x2c, 0xfe, 1, 2, 3};
    CHECK(parse_ble(validfp, o));
    CHECK(matches(*fp, o));
    std::array<uint8_t, 4> apple{3, 0xff, 0x4c, 0};
    CHECK(parse_ble(apple, o));
    CHECK(!matches(*ib, o));
    std::array<uint8_t, 27> ibeacon{26, 0xff, 0x4c, 0, 2, 0x15};
    ibeacon[26] = 0xc5;
    CHECK(parse_ble(ibeacon, o));
    CHECK(matches(*ib, o));
    std::array<uint8_t, 26> hub{25, 0x16, 0xaa, 0xfe, 0x40};
    CHECK(parse_ble(hub, o));
    CHECK(matches(*fh, o));
    hub[4] = 0x10;
    CHECK(parse_ble(hub, o));
    CHECK(!matches(*fh, o));
    std::array<char, 8> safe{};
    const uint8_t dirty[] = {0, 'A', 0x80, 10, 0xff};
    sanitize(dirty, safe);
    CHECK(std::string_view(safe.data()) == "?A???");
    auto oui = rule("test.oui", Kind::Oui, "102030", 90, 100);
    o = {};
    o.address = {0x10, 0x20, 0x30, 0, 0, 1};
    o.ms = 1000;
    CHECK(matches(oui, o));
    auto generic = oui;
    generic.generic = true;
    Settings settings;
    std::array<Detection, 4> out{};
    Engine g(std::span(&generic, 1));
    CHECK(g.ingest(o, settings, out) == 1);
    CHECK(out[0].score == 49);
    auto local = rule("test.local", Kind::Oui, "122030", 95, 100);
    o.address[0] = 0x12;
    Engine l(std::span(&local, 1));
    CHECK(!matches(local, o));
    CHECK(l.ingest(o, settings, out) == 0);
    CHECK((l.recent_counts(o.ms, settings) == std::array<uint32_t, 3>{0, 0, 0}));
    // A same-kind exact/prefix pair must not count as independent corroboration.
    Rule names[] = {rule("test.name", Kind::NameExact, "LAB"),
                    rule("test.prefix", Kind::NamePrefix, "LA")};
    o = {};
    o.radio = Radio::Ble;
    o.ms = 1000;
    std::strcpy(o.name.data(), "LAB");
    Engine n(names);
    CHECK(n.ingest(o, settings, out) == 1);
    CHECK(out[0].score == 60);
    for (int i = 0; i < 100; ++i) {
        o.ms += 100;
        n.ingest(o, settings, out);
    }
    o.ms += 30000;
    CHECK(n.ingest(o, settings, out) == 1);
    CHECK(out[0].score == 60 && out[0].seen_count == 102);
    Rule fused[] = {rule("test.company", Kind::Company, "1234", 60, 100),
                    rule("test.name", Kind::NameExact, "LAB", 60, 79)};
    o.manufacturer_len = 2;
    o.manufacturer[0] = 0x34;
    o.manufacturer[1] = 0x12;
    Engine f(fused);
    CHECK(f.ingest(o, settings, out) == 1);
    CHECK(out[0].score == 75);
    o.ms += 31000;
    o.manufacturer_len = 0;
    CHECK(f.ingest(o, settings, out) == 1);
    CHECK(out[0].score == 60);
    o.ms += 31000;
    o.contradiction = true;
    CHECK(f.ingest(o, settings, out) == 1);
    CHECK(out[0].score == 40);
    // Dense cache storage must preserve independence between every coexisting
    // evidence kind, while exact/prefix matches remain one clue. Start at time
    // zero so occupied slots cannot be confused with zero-initialized entries.
    auto check_groups = [&](std::span<const Rule> rules, Observation sample) {
        for (size_t i = 0; i < rules.size(); ++i)
            for (size_t j = i + 1; j < rules.size(); ++j) {
                std::array<Rule, 2> pair{rules[i], rules[j]};
                Engine cache(pair);
                const bool shared =
                    (pair[0].kind == Kind::NameExact && pair[1].kind == Kind::NamePrefix) ||
                    (pair[0].kind == Kind::SsidExact && pair[1].kind == Kind::SsidPrefix);
                sample.ms = 0;
                CHECK(cache.ingest(sample, settings, out) == 1);
                CHECK(out[0].score == (shared ? 50 : 62));
                CHECK(out[0].rule_count == (shared ? 1 : 2));
                sample.ms = 30100;
                CHECK(cache.ingest(sample, settings, out) == 1);
                CHECK(out[0].seen_count == 2 && out[0].score == (shared ? 50 : 62));
                CHECK(cache.recent_counts(sample.ms, settings)[1] == 1);
            }
    };
    Rule ble_groups[] = {
        rule("group.name", Kind::NameExact, "LAB", 50, 100),
        rule("group.prefix", Kind::NamePrefix, "LA", 50, 100),
        rule("group.uuid16", Kind::Uuid16, "FD5A", 50, 100),
        rule("group.uuid128", Kind::Uuid128, "00000000000000000000000000000000", 50, 100),
        rule("group.company", Kind::Company, "004C", 50, 100),
        rule("group.payload", Kind::Payload, "4C001219", 50, 100),
        rule("group.protocol", Kind::Protocol, "apple_find_my", 50, 100)};
    Observation all_ble{};
    all_ble.radio = Radio::Ble;
    std::strcpy(all_ble.name.data(), "LAB");
    all_ble.uuids[0] = 0xFD5A;
    all_ble.uuid_count = all_ble.uuid128_count = 1;
    all_ble.manufacturer = {0x4c, 0, 0x12, 0x19};
    all_ble.manufacturer_len = 29;
    check_groups(ble_groups, all_ble);
    Rule wifi_groups[] = {rule("group.oui", Kind::Oui, "102030", 50, 100),
                          rule("group.ssid", Kind::SsidExact, "LAB", 50, 100),
                          rule("group.ssid_prefix", Kind::SsidPrefix, "LA", 50, 100),
                          rule("group.vendor", Kind::Payload, "01", 50, 100),
                          rule("group.pwn", Kind::Protocol, "pwnagotchi", 50, 100)};
    Observation all_wifi{};
    all_wifi.subtype = 8;
    all_wifi.address = {0x10, 0x20, 0x30, 0, 0, 1};
    all_wifi.ssid = {'L', 'A', 'B'};
    all_wifi.ssid_len = 3;
    all_wifi.vendor[0] = 1;
    all_wifi.vendor_len = 1;
    all_wifi.pwnagotchi = true;
    check_groups(wifi_groups, all_wifi);
    // No OUI, Apple company ID, or Meta company ID is shipped as a product rule.
    Engine production(production_rules);
    for (Address a : {Address{0xb8, 0xe9, 0x37, 0, 0, 1}, Address{0x24, 0x6f, 0x28, 0, 0, 1}}) {
        o = {};
        o.address = a;
        o.ms = 1000;
        CHECK(production.ingest(o, settings, out) == 0);
    }
    CHECK(parse_ble(apple, o));
    CHECK(production.ingest(o, settings, out) == 0);
    o.manufacturer[0] = 0xab;
    o.manufacturer[1] = 5;
    CHECK(production.ingest(o, settings, out) == 0);
    // Security differences, not merely BSSID count.
    Engine behaviors(production_rules);
    b = beacon();
    o = {};
    o.ms = 1000;
    CHECK(parse_wifi(b, o));
    CHECK(behaviors.ingest(o, settings, out) == 0);
    b[15] = 2;
    o.ms = 2000;
    CHECK(parse_wifi(b, o));
    CHECK(behaviors.ingest(o, settings, out) == 0);
    b[34] = 0;
    b[15] = 3;
    o.ms = 3000;
    CHECK(parse_wifi(b, o));
    CHECK(behaviors.ingest(o, settings, out) == 1);
    CHECK(out[0].category == Category::EVIL_TWIN);
    o = {};
    o.subtype = 12;
    o.channel = 6;
    o.address[0] = 0x10;
    Engine bursts(production_rules);
    for (int i = 0; i < 19; ++i) {
        o.ms = 1000 + i * 100;
        CHECK(bursts.ingest(o, settings, out) == 0);
    }
    o.ms = 3000;
    CHECK(bursts.ingest(o, settings, out) == 1);
    CHECK(out[0].category == Category::DEAUTH);
    Engine slow(production_rules);
    for (int i = 0; i < 30; ++i) {
        o.ms = 1000 + i * 1000;
        CHECK(slow.ingest(o, settings, out) == 0);
    }
    std::array<uint8_t, 29> rid{0xfa, 0xff, 0x0d, 0, 0x12};
    RemoteId decoded{};
    rid[9] = 0x80;
    rid[10] = 0x96;
    rid[11] = 0x98;
    rid[12] = 0; // Latitude 1 degree.
    CHECK(parse_remote_id(rid, decoded));
    CHECK(decoded.latitude_e7 == 10000000 && decoded.location_valid);
    CHECK(!parse_remote_id(std::span(rid).first(28), decoded));
    rid[4] = 0xf2;
    CHECK(!parse_remote_id(rid, decoded));
    MealGuard guard;
    CHECK(guard.award(123, 1000, Category::FLOCK, 80, false));
    CHECK(!guard.award(123, 9000, Category::FLOCK, 80, false));
    auto saved = guard.snapshot(9000);
    MealGuard restored;
    restored.restore(saved);
    CHECK(!restored.award(123, 1, Category::FLOCK, 80, false));
    CHECK(restored.award(123, 1800001, Category::FLOCK, 80, false));
    CHECK(!guard.award(234, 31000, Category::AXON, 35, false));
    Detection d{};
    d.category = Category::FLOCK;
    d.score = 80;
    d.meal = true;
    d.rules[0] = &oui;
    d.rule_count = 1;
    d.seen_count = 1;
    Pet pet;
    pet.feed(d);
    CHECK(pet.xp == 13 && pet.fullness == 90 && pet.meals == 1);
    d.demo = true;
    pet.feed(d);
    CHECK(pet.xp == 13);
    d.demo = false;
    pet.tick(3600000 * 100ULL);
    CHECK(pet.fullness == 0 && pet.mood == 0);
    pet.stroke();
    CHECK(pet.mood == 2);
    std::array<uint8_t, 32> key{}, digest{};
    std::array<char, 21> token{}, token2{};
    CHECK(private_token(key, d, token));
    key[0] = 1;
    CHECK(private_token(key, d, token2));
    CHECK(token != token2);
    // RFC 4231 test case 1; independently defined standard vector.
    std::array<uint8_t, 20> hkey;
    hkey.fill(0x0b);
    const uint8_t msg[] = {'H', 'i', ' ', 'T', 'h', 'e', 'r', 'e'};
    CHECK(hmac_sha256(hkey, msg, digest));
    const uint8_t expected[] = {0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53, 0x5c, 0xa8, 0xaf,
                                0xce, 0xaf, 0x0b, 0xf1, 0x2b, 0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83,
                                0x3d, 0xa7, 0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7};
    CHECK(std::equal(digest.begin(), digest.end(), expected));
    std::array<char, 2048> json{};
    CHECK(json_escape("a\"\\\n", json) > 0);
    CHECK(std::string_view(json.data()) == "\"a\\\"\\\\\\u000a\"");
    CHECK(write_record(d, "session", token.data(), false, json) > 0);
    CHECK(std::strstr(json.data(), "raw_mac") == nullptr);
    CHECK(write_record(d, "session", token.data(), true, json) > 0);
    CHECK(std::strstr(json.data(), "raw_mac") != nullptr);
    std::array<char, 8> tiny{};
    CHECK(write_record(d, "s", "t", false, tiny) == 0);
    d.demo = true;
    CHECK(write_record(d, "s", "t", false, json) == 0);
    // Every category has a host-testable synthetic path, independent of production verification.
    for (size_t c = 0; c < category_count; ++c) {
        Rule family = rule("test.family", Kind::NameExact, "SYNTHETIC", 60, 79);
        family.category = static_cast<Category>(c);
        Engine family_engine(std::span(&family, 1));
        Observation sample{};
        sample.radio = Radio::Ble;
        sample.ms = 1000;
        std::strcpy(sample.name.data(), "SYNTHETIC");
        CHECK(family_engine.ingest(sample, settings, out) == 1);
        CHECK(out[0].category == family.category);
    }
    CHECK(std::string_view(foods[size_t(Category::FLIPPER)]) == "dolphin");
    CHECK(std::string_view(foods[size_t(Category::PINEAPPLE)]) == "pineapple");
    // A BLE service UUID list is matched in network order after little-endian decoding.
    auto uuid_rule = rule("test.uuid", Kind::Uuid16, "1234");
    const std::array<uint8_t, 4> uuid_packet{3, 3, 0x34, 0x12};
    CHECK(parse_ble(uuid_packet, o));
    CHECK(matches(uuid_rule, o));
    auto payload_rule = rule("test.payload", Kind::Payload, "4C000215");
    CHECK(parse_ble(ibeacon, o));
    CHECK(matches(payload_rule, o));
    CHECK(write_export_record(d, "s", "t", false, json) == 0); // Demo rejection.
    std::cout << checks << " core checks passed\n";
}
