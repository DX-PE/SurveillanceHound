// SPDX-License-Identifier: Apache-2.0
#include "signatures_generated.h"
#include "sniffer/core.hpp"
#include "sniffer/json.hpp"
#include "state.hpp"
#include "ui.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace sniffer;
static unsigned checks;
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(c)) {                                                                                \
            std::fprintf(stderr, "feature line %d: %s\n", __LINE__, #c);                           \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)
const Rule &rule(std::string_view id) {
    for (auto &r : production_rules)
        if (id == r.id)
            return r;
    std::abort();
}
int main() {
    Json j;
    CHECK(j.parse("{\"a\":[1,true,null],\"b\":\"hello\\nworld\"}"));
    char value[32];
    CHECK(j.string(j.member(0, "b"), value));
    CHECK(std::string_view(value) == "hello\nworld");
    for (auto bad : {"{", "[]garbage", "{\"a\":1,\"a\":2}", "{\"a\":1,\"\\u0061\":2}", "{\"a\":01}",
                     "{\"a\":1e}", "[1,]", "{\"a\":\"\\z\"}", "[[[[[[[[[[0]]]]]]]]]]"})
        CHECK(!j.parse(bad));
    std::array<uint8_t, 31> apple{30, 0xff, 0x4c, 0, 0x12, 0x19};
    Observation o{};
    CHECK(parse_ble(apple, o));
    CHECK(matches(rule("ble.apple.find_my"), o));
    apple[4] = 7;
    CHECK(parse_ble(apple, o));
    CHECK(!matches(rule("ble.apple.find_my"), o));
    apple[4] = 0x12;
    apple[0] = 29;
    CHECK(parse_ble(std::span(apple).first(30), o));
    CHECK(!matches(rule("ble.apple.find_my"), o));
    std::array<uint8_t, 18> uuid{17,   7,    0x6f, 0x2e, 0x17, 0xdf, 0x14, 0x18, 0xe5,
                                 0x9f, 0xa8, 0x46, 0x32, 0x95, 0x38, 0xbb, 0xcc, 0xe8};
    CHECK(parse_ble(uuid, o));
    CHECK(matches(rule("ble.flock.accessory"), o));
    uuid[0] = 16;
    CHECK(!parse_ble(std::span(uuid).first(17), o));
    const char *pwn =
        "{\"name\":\"TEST HOUND\",\"pwnd_tot\":7,\"version\":\"1.0\",\"extra\":{\"safe\":true}}";
    std::vector<uint8_t> beacon(36);
    beacon[0] = 0x80;
    const uint8_t signature[] = {0xde, 0xad, 0xbe, 0xef, 0xde, 0xad};
    std::copy_n(signature, 6, beacon.begin() + 10);
    beacon[16] = 0x10;
    beacon[21] = 1;
    beacon.insert(beacon.end(), {0, 0, 222, 20});
    beacon.insert(beacon.end(), pwn, pwn + 20);
    beacon.push_back(222);
    beacon.push_back(std::strlen(pwn) - 20);
    beacon.insert(beacon.end(), pwn + 20, pwn + std::strlen(pwn));
    CHECK(parse_wifi(beacon, o));
    CHECK(o.pwnagotchi);
    CHECK(matches(rule("wifi.pwnagotchi.mesh"), o));
    for (auto bad : {"{\"name\":\"evil\\u0000name\",\"version\":\"1\",\"pwnd_tot\":1}",
                     "{\"name\":\"pwnd_tot\",\"version\":\"1\"}",
                     "{\"name\":\"x\",\"version\":\"1\",\"pwnd_tot\":-1}"})
        CHECK(!parse_pwnagotchi({reinterpret_cast<const uint8_t *>(bad), std::strlen(bad)}, value));
    std::vector<uint8_t> probe(24);
    probe[0] = 0x40;
    probe[10] = 0xb4;
    probe[11] = 0x1e;
    probe[12] = 0x52;
    probe.insert(probe.end(), {0, 0});
    CHECK(parse_wifi(probe, o));
    CHECK(matches(rule("wifi.flock.vendor"), o));
    std::array<uint8_t, 29> rid{0xfa, 0xff, 0x0d, 0, 0x02, 0x12};
    rid[6] = 'X';
    RemoteId decoded;
    CHECK(parse_remote_id(rid, decoded));
    CHECK(decoded.id_type == 1 && decoded.aircraft_type == 2);
    DroneCache drones;
    o = {};
    o.radio = Radio::Ble;
    o.address = {1, 2, 3, 4, 5, 6};
    o.service = {};
    std::copy(rid.begin(), rid.end(), o.service.begin());
    o.service_len = 29;
    o.ms = 100;
    drones.ingest(o);
    CHECK(drones.find(o.address, 0, 100)->present == 1);
    rid[26] = 1;
    CHECK(!parse_remote_id(rid, decoded));
    rid[26] = 0;
    rid.fill(0);
    rid[0] = 0xfa;
    rid[1] = 0xff;
    rid[2] = 0x0d;
    rid[4] = 0x12;
    rid[9] = 1;
    CHECK(parse_remote_id(rid, decoded));
    std::copy(rid.begin(), rid.end(), o.service.begin());
    o.ms = 30000;
    drones.ingest(o);
    CHECK(drones.find(o.address, 0, 30000)->present == 3);
    CHECK(drones.find(o.address, 0, 61000)->present == 2);
    CHECK(!drones.find(o.address, 0, 90001));
    rid[4] = 0x42;
    rid[6] = 1;
    CHECK(parse_remote_id(rid, decoded));
    rid[28] = 1;
    CHECK(!parse_remote_id(rid, decoded));
    uint64_t utc{};
    CHECK(parse_utc("202609220300", utc));
    char formatted[24];
    format_utc(utc, formatted);
    CHECK(std::string_view(formatted) == "2026-09-22T03:00:00Z");
    CHECK(!parse_utc("202602290300", utc));
    CHECK(parse_utc("202402290300", utc));
    CHECK(!parse_utc("202613010000", utc));
    BatteryCalibration b;
    CHECK(!valid_battery(b));
    CHECK(battery_mv(b, 1000) == -1);
    b.adc = {1000, 1500, 2000};
    CHECK(valid_battery(b));
    CHECK(battery_mv(b, 1250) == 3500);
    CHECK(battery_percent(3300) == 0 && battery_percent(4200) == 100);
    b.adc[1] = 999;
    CHECK(!valid_battery(b));
    Detection d{};
    d.category = Category::FLIPPER;
    d.radio = Radio::Ble;
    d.score = 75;
    d.first_ms = 1;
    d.last_ms = 1000;
    d.seen_count = 1;
    d.rssi_min = d.rssi_max = d.rssi_sum = -50;
    d.rules[0] = &rule("ble.flipper.3081");
    d.rule_count = 1;
    parse_utc("202609220300", d.unix_seconds);
    std::array<char, 2048> log{}, out{};
    std::array<uint8_t, 32> key{};
    key[0] = 1;
    auto n = write_record(d, "0123456789abcdef0123456789abcdef", "ABCDEFGHIJKLMNOPQRST", true, log);
    CHECK(n);
    CHECK(valid_log_record({log.data(), n}));
    CHECK(!valid_log_record(std::string_view(log.data(), n - 3)));
    auto m = export_log_record({log.data(), n}, key, out);
    CHECK(m);
    CHECK(j.parse({out.data(), m}));
    CHECK(j.member(0, "raw_mac") == -1);
    CHECK(j.raw(j.member(0, "time_quality")) == "\"manual\"");
    CHECK(j.string(j.member(0, "identifier"), value));
    CHECK(std::string_view(value) != "ABCDEFGHIJKLMNOPQRST");
    auto prev = out;
    key[0] = 2;
    CHECK(export_log_record({log.data(), n}, key, out));
    CHECK(prev != out);
    std::string corrupt(log.data(), n);
    auto at = corrupt.find("\"score\":75");
    CHECK(at != std::string::npos);
    corrupt.replace(at, 10, "\"score\":0");
    CHECK(!valid_log_record(corrupt));
    Settings settings;
    Pet pet;
    ui::View view(settings, pet);
    settings.onboarded = true;
    for (bool portrait : {false, true}) {
        settings.portrait = portrait;
        view.screen = ui::Screen::Home;
        view.paused = false;
        view.now = 1000;
        view.tap(portrait ? 40 : 330, portrait ? 410 : 240);
        CHECK(view.paused && (view.requests & ui::Pause));
        auto count = view.recent_count;
        view.event(d);
        CHECK(view.recent_count == count);
        view.tap(portrait ? 40 : 330, portrait ? 410 : 240);
        CHECK(!view.paused);
        view.event(d);
        CHECK(view.recent_count == count + 1);
        std::vector<uint16_t> tile(ui::View::max_tile_pixels + 2, 0xabcd);
        for (int page = 0; page <= int(ui::Screen::SelfTest); ++page) {
            view.screen = ui::Screen(page);
            for (int y = 0; y < view.height(); y += view.tile_rows())
                view.render(y, {tile.data() + 1, size_t(view.width() * view.tile_rows())});
            CHECK(tile.front() == 0xabcd && tile.back() == 0xabcd);
        }
        for (int cat = 0; cat < int(category_count); ++cat) {
            d.category = Category(cat);
            d.meal = true;
            d.demo = true;
            view.event(d);
            view.screen = ui::Screen::Home;
            view.now += 2600;
            for (int y = 0; y < view.height(); y += view.tile_rows())
                view.render(y, {tile.data() + 1, size_t(view.width() * view.tile_rows())});
            CHECK(tile.front() == 0xabcd && tile.back() == 0xabcd);
        }
    }
    view.screen = ui::Screen::Log;
    view.log_page = 0;
    view.tap(20, view.height() - 70);
    CHECK(view.log_page == 1);
    view.tap(20, settings.portrait ? 90 : 70);
    CHECK(view.selected == 4 && view.screen == ui::Screen::Details);
    storage::StateV2 old{};
    old.pet.xp = 123;
    old.settings.research = true;
    old.meal_key[0] = 1;
    storage::State upgraded;
    CHECK(storage::migrate(old, upgraded));
    CHECK(upgraded.pet.xp == 123 && upgraded.settings.research && upgraded.version == 3);
    CHECK(storage::valid_current(upgraded));
    upgraded.settings.threshold = 0;
    CHECK(!storage::valid_current(upgraded));
    std::printf("%u extended feature checks passed\n", checks);
}
