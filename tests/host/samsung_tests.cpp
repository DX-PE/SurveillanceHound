// SPDX-License-Identifier: Apache-2.0
#include "signatures_generated.h"
#include "ui.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
using namespace sniffer;
static unsigned checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::fprintf(stderr, "samsung:%d: %s\n", __LINE__, #x);                                \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)

// Entirely synthetic ID, counter, flags and authentication bytes; no real tag captures.
static std::array<uint8_t, 22> service(uint8_t state = 2, uint8_t id = 1) {
    std::array<uint8_t, 22> s{0x5a, 0xfd, uint8_t(0x10 | state), 0x21, 0x43, 0x01};
    for (unsigned i = 0; i < 8; ++i)
        s[6 + i] = id + i;
    s[14] = 0xc3;
    return s;
}
static Observation observation(uint64_t ms, uint8_t mac = 1, uint8_t id = 1, uint8_t state = 2) {
    auto s = service(state, id);
    std::vector<uint8_t> ad{2, 1, 6, 23, 0x16};
    ad.insert(ad.end(), s.begin(), s.end());
    Observation o{};
    o.ms = ms;
    o.address = {0xc2, 1, 2, 3, 4, mac};
    o.address_type = 1;
    o.rssi = -45;
    CHECK(parse_ble(ad, o));
    return o;
}
static Detection detection(Engine &engine, const Settings &settings, const Observation &o) {
    std::array<Detection, 4> out{};
    CHECK(engine.ingest(o, settings, out) == 1);
    CHECK(out[0].category == Category::SAMSUNG_TAG && out[0].score == 60);
    return out[0];
}
static void parser_tests() {
    for (uint8_t state = 0; state < 8; ++state) {
        SamsungTag tag{};
        CHECK(parse_samsung_tag(service(state), tag));
        CHECK(tag.valid() && tag.id.front() == 1 && tag.id.back() == 8);
        CHECK((tag.state == SamsungState::Connected) == (state == 5));
        if (state == 0 || state == 4 || state == 6 || state == 7)
            CHECK(tag.state == SamsungState::Unknown);
    }
    auto good = service(5);
    for (unsigned n = 0; n <= 32; ++n) {
        std::vector<uint8_t> s(good.begin(), good.end());
        s.resize(n);
        SamsungTag tag{};
        CHECK(parse_samsung_tag(s, tag) == (n == 22));
        CHECK(tag.valid() == (n == 22));
    }
    for (unsigned header = 0; header < 256; ++header) {
        auto s = good;
        s[2] = header;
        SamsungTag tag{};
        CHECK(parse_samsung_tag(s, tag) == ((header & 0xf8) == 0x10));
    }
    for (auto fill : {0, 255}) {
        auto s = good;
        std::fill_n(s.begin() + 6, 8, fill);
        SamsungTag tag{};
        CHECK(!parse_samsung_tag(s, tag) && !tag.valid());
    }
    SamsungTag previous{};
    CHECK(parse_samsung_tag(good, previous));
    auto bad = good;
    bad[0] = 0;
    CHECK(!parse_samsung_tag(bad, previous));
    CHECK(previous.state == SamsungState::Connected && previous.id.front() == 1);
    auto o = observation(1000, 1, 1, 5);
    constexpr uint8_t generic[] = {3, 3, 0x5a, 0xfd};
    CHECK(parse_ble(generic, o) && !o.samsung.valid()); // no stale connection state
    Settings settings;
    Engine engine(production_rules);
    CHECK(should_alert(settings, detection(engine, settings, o)));
    constexpr uint8_t partial[] = {5, 0x16, 0x5a, 0xfd, 0x15, 0};
    o.ms += 31000;
    CHECK(parse_ble(partial, o) && !o.samsung.valid());
    CHECK(should_alert(settings, detection(engine, settings, o)));
}
static void identity_tests() {
    Settings settings;
    Engine engine(production_rules);
    std::array<uint8_t, 32> key{};
    key[0] = 17;
    auto a = detection(engine, settings, observation(1000));
    auto moved = observation(32000, 2);
    moved.address_type = 0;
    auto b = detection(engine, settings, moved);
    CHECK(b.seen_count == 2 && b.first_ms == a.first_ms && b.address == moved.address);
    CHECK(engine.recent_counts(32000, settings)[1] == 1);
    CHECK(meal_hash(key, a) == meal_hash(key, b));
    CHECK(address_hash(key, a) != address_hash(key, b));
    std::array<char, 21> ta{}, tb{};
    CHECK(private_token(key, a, ta) && private_token(key, b, tb) && ta == tb);
    auto c = detection(engine, settings, observation(32200, 2, 11));
    CHECK(c.seen_count == 1 && meal_hash(key, c) != meal_hash(key, b));
    CHECK(engine.recent_counts(32200, settings)[1] == 2); // different IDs even with shared MAC
    auto new_key = key;
    new_key[1] = 9;
    CHECK(meal_hash(new_key, a) != meal_hash(key, a));
    CHECK(private_token(new_key, a, tb) && ta != tb);
    auto legacy = a;
    legacy.samsung = {};
    CHECK(meal_hash(key, legacy) == address_hash(key, legacy));
    auto other_category = a;
    other_category.category = Category::AIRTAG;
    CHECK(meal_hash(key, other_category) == address_hash(key, other_category));
    // Log only the pseudonym, never the eight-byte advertised identifier.
    std::array<char, 2048> record{};
    auto n = write_record(a, "0123456789abcdef0123456789abcdef", ta.data(), false, record);
    CHECK(n && valid_log_record(std::string_view(record.data(), n)));
    CHECK(std::string(record.data(), n).find("0102030405060708") == std::string::npos);
    MealGuard meals;
    CHECK(meals.award(meal_hash(key, a), 1000, a.category, a.score, false));
    CHECK(!meals.award(meal_hash(key, b), 32000, b.category, b.score, false));
    FollowScent follow;
    follow.select(a, 1000);
    CHECK(follow.observe(moved, false) && follow.address == moved.address);
    CHECK(!follow.observe(observation(33000, 2, 11), false));
    auto missing = moved;
    missing.ms = 34000;
    missing.samsung = {};
    CHECK(!follow.observe(missing, false));
}
static void state_tests() {
    Settings s;
    s.onboarded = true;
    Pet pet;
    ui::View v(s, pet);
    v.identity_key[0] = 17;
    v.scanning = true;
    v.tag_watch.start(1000, false);
    Engine engine(production_rules);
    auto deliver = [&](Observation o) {
        v.now = o.ms;
        auto d = detection(engine, s, o);
        v.event(d);
        return d;
    };
    auto offline = deliver(observation(1000));
    CHECK(v.alert_until > v.now && v.tag_watch.find(v.identity(offline)));
    // State transitions bypass the 30-second repeat cadence, but retain the global rate cap.
    std::array<Detection, 4> out{};
    CHECK(!engine.ingest(observation(1050, 2, 1, 5), s, out));
    auto connected = deliver(observation(1100, 2, 1, 5));
    CHECK(!v.alert_until && !v.alert_allowed(connected));
    CHECK(!v.tag_watch.find(v.identity(connected)));
    CHECK(engine.recent_counts(v.now, s)[1] == 1);
    CHECK(v.companion.scents[size_t(Category::SAMSUNG_TAG)].observations == 2);
    offline = deliver(observation(1200, 3));
    CHECK(v.alert_allowed(offline) && v.alert_until > v.now);
    CHECK(v.tag_watch.find(v.identity(offline)));
    // Suppressing one tag must not dismiss a pending card for another device.
    auto unrelated = offline;
    unrelated.category = Category::AIRTAG;
    v.event(unrelated);
    connected = deliver(observation(1300, 3, 1, 5));
    CHECK(v.alert_until > v.now && v.alert_detection.category == Category::AIRTAG);
    auto unknown = deliver(observation(1400, 3, 1, 7));
    CHECK(v.alert_allowed(unknown));
    // Unknown/partial packets cannot inherit the prior suppression.
    connected = deliver(observation(1500, 3, 1, 5));
    auto partial = observation(1600, 3);
    partial.samsung = {};
    auto generic = deliver(partial);
    CHECK(v.alert_allowed(generic) && !generic.samsung.valid());
    // The core watch has the same guard, even without the UI eligibility check.
    TagWatch watch;
    watch.start(1000, false);
    watch.observe(1, offline, 1200);
    CHECK(watch.find(1));
    watch.observe(1, connected, 1500);
    CHECK(!watch.find(1));
}
static void service_to_payload_tests() {
    Settings s;
    s.onboarded = true;
    Pet pet;
    ui::View v(s, pet);
    v.identity_key[0] = 17;
    v.scanning = true;
    v.tag_watch.start(1000, false);
    Engine engine(production_rules);
    auto generic = observation(1000);
    generic.samsung = {};
    auto old = detection(engine, s, generic);
    v.now = 1000;
    v.event(old);
    CHECK(v.alert_until > v.now && v.tag_watch.find(v.identity(old)));
    auto connected = detection(engine, s, observation(1200, 1, 1, 5));
    v.now = 1200;
    v.event(connected);
    CHECK(!v.alert_until && !v.watch_count());
    // Even a shared MAC must not clear a card for a different known broadcast ID.
    auto other = detection(engine, s, observation(1400, 1, 11));
    v.now = 1400;
    v.event(other);
    connected.last_ms = v.now = 1600;
    v.event(connected);
    CHECK(v.alert_until > v.now && v.identity(v.alert_detection) == v.identity(other));
}

static void ignore_watch_tests(bool portrait) {
    Settings s;
    s.onboarded = true;
    s.portrait = portrait;
    Pet pet;
    ui::View v(s, pet);
    v.identity_key[0] = 17;
    v.scanning = true;
    Engine engine(production_rules);
    auto a = detection(engine, s, observation(1000));
    v.now = 1000;
    v.event(a);
    v.open_actions(a);
    v.tap(30, (portrait ? 80 : 66) + 3 * (portrait ? 55 : 34) + 12);
    CHECK(v.is_ignored(a) && !v.alert_until && v.companion.ignored_count() == 1);
    // The existing persisted companion representation carries the new keyed ID unchanged.
    ui::View reloaded(s, pet);
    reloaded.companion = v.companion;
    reloaded.identity_key = v.identity_key;
    reloaded.scanning = true;
    for (unsigned hour = 1; hour <= 24; ++hour) {
        reloaded.now = uint64_t(hour) * 3600000;
        auto d = detection(engine, s, observation(reloaded.now, uint8_t(hour + 1)));
        reloaded.event(d);
        CHECK(reloaded.is_ignored(d) && !reloaded.alert_until);
        CHECK(reloaded.companion.ignored_count() == 1);
    }
    auto b = detection(engine, s, observation(reloaded.now + 200, 26, 11));
    reloaded.now += 200;
    reloaded.event(b);
    CHECK(reloaded.alert_until > reloaded.now && !reloaded.is_ignored(b));
    auto demo = a;
    demo.demo = true;
    CHECK(!reloaded.is_ignored(demo));
    // A pre-upgrade MAC ignore still works; it does not silently ignore unrelated new IDs.
    ui::View legacy(s, pet);
    legacy.identity_key = v.identity_key;
    CHECK(legacy.companion.ignore(address_hash(v.identity_key, a), a.category));
    CHECK(legacy.is_ignored(a));
    CHECK(!legacy.is_ignored(b));
    // An unignored broadcast ID can accrue Travel Watch time through MAC changes.
    ui::View travelling(s, pet);
    travelling.identity_key = v.identity_key;
    travelling.scanning = true;
    travelling.tag_watch.start(1000, false);
    Engine travelling_engine(production_rules);
    for (unsigned minute = 0; minute <= 10; ++minute) {
        travelling.now = 1000 + minute * 60000;
        auto d = detection(travelling_engine, s, observation(travelling.now, minute + 1));
        travelling.event(d);
    }
    CHECK(travelling.watch_warning());
    auto d = detection(travelling_engine, s, observation(travelling.now + 200, 12, 1, 5));
    travelling.now += 200;
    travelling.event(d);
    CHECK(!travelling.watch_warning() && !travelling.watch_count());
}
static void render(const char *dir) {
    for (bool portrait : {false, true}) {
        Settings s;
        s.onboarded = true;
        s.portrait = portrait;
        Pet pet;
        ui::View v(s, pet);
        Engine engine(production_rules);
        v.detail_detection = detection(engine, s, observation(1000, 1, 1, 5));
        v.screen = ui::Screen::Details;
        v.detail_valid = true;
        v.detail_page = 1;
        const int w = portrait ? 320 : 480, h = portrait ? 480 : 320;
        std::ofstream out(std::string(dir) +
                              (portrait ? "/samsung-portrait.ppm" : "/samsung-landscape.ppm"),
                          std::ios::binary);
        out << "P6\n" << w << " " << h << "\n255\n";
        std::vector<uint16_t> tile(w * v.tile_rows());
        for (int y = 0; y < h; y += v.tile_rows()) {
            v.render(y, tile);
            for (auto p : tile) {
                char rgb[] = {char(((p >> 11) & 31) * 255 / 31), char(((p >> 5) & 63) * 255 / 63),
                              char((p & 31) * 255 / 31)};
                out.write(rgb, 3);
            }
        }
        CHECK(bool(out));
    }
}
int main(int argc, char **argv) {
    parser_tests();
    identity_tests();
    state_tests();
    service_to_payload_tests();
    ignore_watch_tests(false);
    ignore_watch_tests(true);
    if (argc == 2)
        render(argv[1]);
    std::printf(
        "Samsung: %u checks passed; host sizes observation=%zu detection=%zu engine=%zu view=%zu\n",
        checks, sizeof(Observation), sizeof(Detection), sizeof(Engine), sizeof(ui::View));
}
