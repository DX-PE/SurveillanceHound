// SPDX-License-Identifier: Apache-2.0
#include "signatures_generated.h"
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
            std::fprintf(stderr, "tag_watch:%d: %s\n", __LINE__, #x);                              \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)

static Detection tag(Category c = Category::SAMSUNG_TAG) {
    Detection d{};
    d.category = c;
    d.radio = Radio::Ble;
    d.score = 60;
    d.address = {2, 3, 4, 5, 6, 7};
    return d;
}
static void qualify(TagWatch &w, Detection d, uint64_t hash, uint64_t begin) {
    for (unsigned minute = 0; minute <= 10; ++minute) {
        d.last_ms = begin + minute * 60000;
        w.observe(hash, d, d.last_ms);
    }
}
static void qualify(ui::View &v, Detection d) {
    const auto start = v.now;
    for (unsigned minute = 0; minute <= 10; ++minute) {
        d.first_ms = 0; // Cached engine lifetime must not count time before arming.
        d.seen_count = UINT32_MAX;
        d.last_ms = v.now = start + minute * 60000;
        v.event(d);
    }
}
int main() {
    auto d = tag();
    TagWatch w;
    qualify(w, d, 1, 1000);
    CHECK(!w.warning(601000)); // Explicitly armed, never automatic.
    w.start(1000, false);
    for (uint64_t t = 1000; t < 20000; ++t) {
        d.last_ms = t;
        w.observe(1, d, t);
    }
    CHECK(w.find(1)->minutes == 1 && !w.warning(20000));
    d.last_ms = 601000;
    w.observe(1, d, d.last_ms);
    CHECK(w.find(1)->first == 601000 && !w.warning(601000)); // A gap resets.
    CHECK(w.gap_resets == 1);
    w.start(1000, false);
    for (uint64_t t = 1000; t <= 601000; t += 120000) {
        d.last_ms = t;
        w.observe(1, d, t);
    }
    CHECK(w.find(1)->minutes == 6 && !w.warning(601000)); // Elapsed time alone is insufficient.
    w.start(1000, false);
    qualify(w, d, 1, 1000);
    CHECK(w.warning(601000) && w.find(1)->minutes == 11);
    CHECK(w.warning(721000) && !w.warning(721001));
    w.start(1000, false);
    for (uint64_t offset : {0, 90000, 180000, 270000, 360000, 450000, 540000, 600000}) {
        d.last_ms = 1000 + offset;
        w.observe(1, d, d.last_ms);
    }
    CHECK(w.find(1)->minutes == 8 && w.warning(601000));
    w.find(1)->acknowledged = true;
    d.last_ms = 661000;
    w.observe(1, d, d.last_ms);
    CHECK(!w.warning(661000) && w.find(1)->acknowledged);
    d.last_ms = 800000;
    w.observe(1, d, d.last_ms);
    CHECK(!w.find(1)->acknowledged && w.find(1)->first == 800000);
    for (Category c :
         {Category::AIRTAG, Category::SAMSUNG_TAG, Category::TILE, Category::GOOGLE_TAG}) {
        w.start(1000, false);
        qualify(w, tag(c), 1, 1000);
        CHECK(w.warning(601000) && w.warning(601000)->category == c);
    }
    for (unsigned c = 0; c < category_count; ++c) {
        if (TagWatch::tag(Category(c)))
            continue;
        w.start(1000, false);
        qualify(w, tag(Category(c)), 1, 1000);
        CHECK(!w.find(1) && !w.warning(601000));
    }
    for (unsigned bad = 0; bad < 7; ++bad) {
        w.start(1000, false);
        d = tag();
        d.last_ms = 1000;
        if (bad == 0)
            d.score = 49;
        if (bad == 1)
            d.score = 101;
        if (bad == 2)
            d.radio = Radio::Wifi;
        if (bad == 3)
            d.demo = true;
        if (bad == 4)
            d.last_ms = 999;
        if (bad == 5)
            d.last_ms = 2001;
        w.observe(bad == 6 ? 0 : 1, d, 2000);
        CHECK(!w.find(1));
    }
    w.start(1000, false);
    d = tag();
    d.last_ms = 61000;
    w.observe(1, d, 61000);
    d.last_ms = 1000;
    w.observe(1, d, 61000);
    CHECK(w.find(1)->first == 61000 && w.find(1)->last == 61000);
    for (uint64_t id = 2; id <= 17; ++id)
        w.observe(id, d, 61000);
    CHECK(!w.find(17) && w.find(1)); // Capacity bounded without evicting present tags.
    d.last_ms = 300000;
    w.observe(17, d, 300000);
    CHECK(w.find(17));

    // Production Samsung classifier feeds the exact event path used by the board.
    Settings settings;
    Pet pet;
    ui::View v(settings, pet);
    settings.onboarded = true;
    v.scanning = true;
    v.identity_key[0] = 1;
    v.now = 1000;
    v.tag_watch.start(v.now, false);
    Engine engine(production_rules);
    Observation o{};
    o.radio = Radio::Ble;
    o.address = tag().address;
    o.uuids[0] = 0xFD5A;
    o.uuid_count = 1;
    for (unsigned minute = 0; minute <= 10; ++minute) {
        v.now = o.ms = 1000 + minute * 60000;
        std::array<Detection, 4> found{};
        auto n = engine.ingest(o, settings, found);
        CHECK(n > 0);
        for (size_t i = 0; i < n; ++i)
            v.event(found[i]);
        CHECK((v.watch_warning() != nullptr) == (minute == 10));
    }
    CHECK(pet.xp == 0 && v.meal_until == 0); // Warning does not award meals or animation.

    // A radio packet can arrive after the timestamp taken at the start of a UI batch.
    // Future timestamps still fail closed; the board now refreshes its clock per packet.
    v.tag_watch.start(1000, false);
    v.now = 1000;
    auto new_packet = tag();
    new_packet.last_ms = 1001;
    v.event(new_packet);
    CHECK(v.tag_watch.clock_skips == 1 && v.watch_count() == 0);
    v.now = 1001;
    v.event(new_packet);
    CHECK(v.watch_count() == 1);
    v.now = 121002;
    v.update_tag_watch();
    CHECK(v.watch_count() == 0 && v.tag_watch.gap_resets == 1);
    new_packet.last_ms = v.now;
    v.event(new_packet);
    settings.threshold = 80;
    v.update_tag_watch();
    CHECK(v.watch_count() == 0 && v.tag_watch.filter_resets == 1);
    CHECK(std::string_view(v.watch_hint()).find("HIGH FILTER") != std::string_view::npos);
    v.tag_watch.gap_resets = UINT32_MAX;
    auto &entry = v.tag_watch.entries[0];
    entry.hash = 1;
    v.tag_watch.drop(entry, true);
    CHECK(v.tag_watch.gap_resets == UINT32_MAX && !entry.hash);

    for (bool portrait : {false, true}) {
        Settings s;
        Pet p;
        ui::View ui(s, p);
        s.onboarded = true;
        s.portrait = portrait;
        ui.identity_key[0] = 2;
        ui.scanning = true;
        ui.now = 2000;
        ui.screen = ui::Screen::Settings;
        ui.tap(ui.width() - 130, 44);
        CHECK(ui.screen == ui::Screen::TagWatch);
        const int row_y = portrait ? 80 : 66, row_step = portrait ? 55 : 34;
        ui.tap(30, row_y + 10);
        CHECK(ui.tag_watch.armed && ui.screen == ui::Screen::WatchProgress);
        ui.tap(ui.width() - 40, 44);
        CHECK(ui.screen == ui::Screen::TagWatch);
        ui.tap(ui.width() - 110, 44);
        CHECK(ui.screen == ui::Screen::WatchProgress);
        ui.tap(30, ui.height() - 65);
        CHECK(ui.screen == ui::Screen::Home);
        ui.tap(ui.width() - 140, 44);
        CHECK(ui.screen == ui::Screen::WatchProgress); // Armed Home shortcut opens progress.
        ui.screen = ui::Screen::Home;
        d = tag();
        qualify(ui, d);
        CHECK(ui.watch_warning() && ui.watch_red());
        const auto hash = ui.identity(d);
        CHECK(ui.tag_watch.find(hash));
        // Distinct address/type identities never merge into the same warning.
        auto other = d;
        other.address_type = 1;
        CHECK(ui.identity(other) != hash);
        other.address[5]++;
        CHECK(ui.identity(other) != hash);
        ui.now += 1000;
        CHECK(!ui.watch_red() && ui.watch_warning());
        s.reduced_animation = true;
        CHECK(ui.watch_red());
        s.reduced_animation = false;
        ui.tag_watch.flash = false;
        CHECK(ui.watch_red());
        ui.tag_watch.flash = true;
        ui.now -= 1000;
        for (unsigned theme = 0; theme < 3; ++theme)
            for (unsigned scene = 0; scene < 3; ++scene) {
                ui.appearance.theme = theme;
                ui.appearance.scene = scene;
                std::vector<uint16_t> tile(ui::View::max_tile_pixels + 2, 0x1234);
                for (auto screen : {ui::Screen::Home, ui::Screen::Settings, ui::Screen::Log,
                                    ui::Screen::TagWatch, ui::Screen::WatchProgress}) {
                    ui.screen = screen;
                    for (int y = 0; y < ui.height(); y += ui.tile_rows())
                        ui.render(y, {tile.data() + 1, size_t(ui.width() * ui.tile_rows())});
                    CHECK(tile.front() == 0x1234 && tile.back() == 0x1234);
                    ui.render(96, {tile.data() + 1, size_t(ui.width() * ui.tile_rows())});
                    CHECK(tile[1] == 0x7800); // Red background in either orientation/theme.
                }
            }
        ui.screen = ui::Screen::Home;
        ui.tap(portrait ? 220 : 330, portrait ? 377 : 200);
        CHECK(ui.screen == ui::Screen::TagWatch && ui.watch_review == hash);
        auto next_tag = tag(Category::AIRTAG);
        qualify(ui, next_tag);
        CHECK(ui.watch_review == hash); // Review target does not change underneath a tap.
        // Original tag expired while the other qualified, so an old ACK must do nothing.
        ui.tap(30, row_y + 2 * row_step + 10);
        CHECK(ui.screen == ui::Screen::TagWatch && ui.watch_warning());
        ui.open_tag_watch();
        ui.tap(30, row_y + 2 * row_step + 10);
        CHECK(ui.screen == ui::Screen::Home && !ui.watch_warning());
        d = next_tag;
        d.last_ms = ++ui.now;
        ui.event(d);
        CHECK(!ui.watch_warning()); // ACK is not undone by repeat packets.
        ui.tag_watch.start(ui.now, false);
        qualify(ui, d);
        CHECK(ui.watch_warning());
        ui.snooze_until = ui.now + 1000;
        CHECK(!ui.watch_warning() && !ui.watch_red());
        ui.now += 1000;
        CHECK(ui.watch_warning());
        ui.open_tag_watch();
        d.last_ms = ++ui.now;
        ui.event(d); // A regular card can queue underneath the Watch review screen.
        CHECK(ui.alert_until > ui.now);
        ui.tap(30, row_y + 3 * row_step + 10);
        CHECK(ui.companion.is_ignored(ui.identity(d)) && !ui.watch_warning());
        CHECK(!ui.alert_until); // Ignoring Watch must also clear that ordinary card.
        ui.companion.ignored = {};
        ui.tag_watch.start(ui.now, false);
        qualify(ui, d);
        auto weak = d;
        weak.last_ms = ++ui.now;
        weak.score = 49;
        ui.event(weak);
        CHECK(!ui.watch_warning() && !ui.tag_watch.find(ui.identity(d)));
        qualify(ui, d);
        s.enabled_categories &= ~(1U << unsigned(d.category));
        CHECK(!ui.watch_warning());
        ui.update_tag_watch();
        CHECK(ui.tag_watch.filter_resets > 0);
        s.enabled_categories |= 1U << unsigned(d.category);
        CHECK(!ui.tag_watch.find(ui.identity(d)));
        qualify(ui, d);
        s.alert_categories &= ~(1U << unsigned(d.category));
        CHECK(!ui.watch_warning());
        ui.update_tag_watch();
        s.alert_categories |= 1U << unsigned(d.category);
        CHECK(!ui.watch_warning() && !ui.tag_watch.find(ui.identity(d)));
        qualify(ui, d);
        s.threshold = 80;
        ui.update_tag_watch();
        CHECK(!ui.watch_warning());
        s.threshold = 50;
        qualify(ui, d);
        ui.screen = ui::Screen::Home;
        ui.tap(portrait ? 160 : 380, portrait ? 415 : 241);
        CHECK(ui.paused && !ui.watch_warning());
        ui.now += 600000;
        ui.tap(portrait ? 160 : 380, portrait ? 415 : 241);
        CHECK(!ui.paused && !ui.watch_warning());
        d.last_ms = ui.now;
        ui.event(d);
        CHECK(ui.tag_watch.find(ui.identity(d))->minutes == 1);
        ui.demo = true;
        ui.update_tag_watch();
        CHECK(!ui.tag_watch.armed && !ui.watch_warning());
        ui.tag_watch.start(ui.now, true);
        d.demo = false;
        qualify(ui, d);
        CHECK(!ui.watch_warning());
        d.demo = true;
        qualify(ui, d);
        CHECK(ui.watch_warning() && p.xp == 0);
        ui.demo = false;
        ui.update_tag_watch();
        CHECK(!ui.watch_warning() && !ui.tag_watch.armed);
        ui.companion.ignored = {};
        ui.tag_watch.start(ui.now, false);
        ui.screen = ui::Screen::TagWatch;
        ui.watch_review = 0;
        d.demo = false;
        qualify(ui, d);
        ui.update_tag_watch();
        CHECK(ui.watch_review == ui.identity(d)); // Warning arrives while its page is open.
        ui.tap(30, row_y + row_step + 10);
        CHECK(!ui.tag_watch.flash && ui.watch_red());
        ui.screen = ui::Screen::WatchProgress;
        ui.watch_page = 3; // Removing/expiring rows while on the last page must stay usable.
        ui.tag_watch.entries = {};
        std::vector<uint16_t> tile(ui::View::max_tile_pixels + 2, 0xabcd);
        for (int y = 0; y < ui.height(); y += ui.tile_rows())
            ui.render(y, {tile.data() + 1, size_t(ui.width() * ui.tile_rows())});
        CHECK(tile.front() == 0xabcd && tile.back() == 0xabcd);
        ui.tap(ui.width() - 30, ui.height() - 65);
        CHECK(ui.watch_page == 0);
    }
    std::printf("%u tag-watch checks passed\n", checks);
}
