// SPDX-License-Identifier: Apache-2.0
#include "ui.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace sniffer;
static unsigned checks;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::fprintf(stderr, "follow:%d: %s\n", __LINE__, #x);                                 \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)
int main() {
    Detection d{};
    d.category = Category::SAMSUNG_TAG;
    d.address = {2, 3, 4, 5, 6, 7};
    d.radio = Radio::Ble;
    d.rssi_max = -20;
    d.rssi_min = -100;
    d.last_ms = 900; // Historical extrema must not seed live readings.
    FollowScent f;
    f.select(d, 1000);
    CHECK(!f.received && f.state(1001, true) == ScentSignal::Waiting);
    CHECK(f.state(16000, true) == ScentSignal::Lost);
    CHECK(f.state(16000, false) == ScentSignal::Paused);
    Observation o{};
    o.address = d.address;
    o.radio = d.radio;
    o.ms = 1000;
    o.rssi = -80;
    CHECK(!f.observe(o, false));
    o.ms = 1001;
    CHECK(f.observe(o, false));
    CHECK(f.count == 1 && f.rssi == -80);
    auto other = o;
    other.address[5]++;
    CHECK(!f.observe(other, false));
    other = o;
    other.address_type++;
    CHECK(!f.observe(other, false));
    other = o;
    other.radio = Radio::Wifi;
    CHECK(!f.observe(other, false));
    CHECK(!f.observe(o, true));
    other = o;
    other.ms = 999;
    CHECK(!f.observe(other, false));
    for (int bad : {-127, 1, 127}) {
        other = o;
        other.rssi = bad;
        CHECK(!f.observe(other, false));
    }
    o.ms = 1999;
    o.rssi = -78;
    CHECK(f.observe(o, false));
    CHECK(f.count == 1);
    o.ms = 4000;
    o.rssi = -60;
    CHECK(f.observe(o, false));
    CHECK(f.trend(4000) == 18);
    CHECK(f.state(9000, true) == ScentSignal::Fresh);
    CHECK(f.state(9001, true) == ScentSignal::Stale && f.trend(9001) == 0);
    CHECK(f.state(19000, true) == ScentSignal::Lost);
    o.ms = 20000;
    o.rssi = -40;
    CHECK(f.observe(o, false));
    CHECK(f.trend(20000) == 0);
    for (unsigned i = 1; i <= 100; ++i) {
        o.ms = 20000 + i * 1000;
        CHECK(f.observe(o, false));
    }
    CHECK(f.count == 30 && f.history[0].ms == 91000 && f.last_ms == 120000);
    f.restart(120000);
    CHECK(!f.received && !f.count && f.active);
    CHECK(!f.observe(o, false));
    for (bool portrait : {false, true}) {
        Settings settings;
        Pet pet;
        ui::View v(settings, pet);
        settings.onboarded = true;
        settings.portrait = portrait;
        v.scanning = true;
        v.now = 10000;
        v.screen = ui::Screen::Details;
        v.detail_detection = d;
        v.detail_valid = true;
        v.tap(80, v.height() - 24);
        CHECK(v.screen == ui::Screen::Follow && v.follow.address == d.address);
        o.ms = v.now + 1;
        v.observe(o);
        CHECK(!v.follow.received); // Future timestamps rejected.
        ++v.now;
        v.observe(o);
        CHECK(v.signal() == ScentSignal::Fresh);
        v.now += 6000;
        CHECK(v.signal() == ScentSignal::Stale);
        v.tap(30, v.height() - 65);
        CHECK(v.screen == ui::Screen::Home && v.follow.active);
        v.tap(portrait ? 160 : 380, portrait ? 415 : 241);
        CHECK(v.paused && v.signal() == ScentSignal::Paused);
        o.ms = ++v.now;
        v.observe(o);
        CHECK(!v.follow.received);
        v.tap(portrait ? 160 : 380, portrait ? 415 : 241);
        CHECK(!v.paused && v.signal() == ScentSignal::Waiting);
        o.ms = ++v.now;
        v.observe(o);
        CHECK(v.signal() == ScentSignal::Fresh);
        auto observations = v.recent_count;
        CHECK(observations == 0 && pet.xp == 0); // Follow samples do not feed/log by themselves.
        v.screen = ui::Screen::Follow;
        std::vector<uint16_t> guarded(ui::View::max_tile_pixels + 2, 0xabcd);
        for (bool reduced : {false, true}) {
            settings.reduced_animation = reduced;
            for (auto time : {v.now, v.now + 6000, v.now + 16000, v.now + 50000}) {
                v.now = time;
                for (int y = 0; y < v.height(); y += v.tile_rows())
                    v.render(y, {guarded.data() + 1, size_t(v.width() * v.tile_rows())});
                CHECK(guarded.front() == 0xabcd && guarded.back() == 0xabcd);
            }
        }
        v.tap(v.width() - 30, v.height() - 65);
        CHECK(!v.follow.active && v.screen == ui::Screen::Home);
        v.follow_scent(d);
        v.screen = ui::Screen::Settings;
        v.settings_page = 3;
        v.tap(30, (portrait ? 80 : 66) + 3 * (portrait ? 55 : 34) + 12);
        CHECK(v.demo && !v.follow.active);
    }
    std::printf("%u follow checks passed\n", checks);
}
