// SPDX-License-Identifier: Apache-2.0
#include "assets_generated.h"
#include "signatures_generated.h"
#include "state.hpp"
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
            std::fprintf(stderr, "companion:%d: %s\n", __LINE__, #x);                              \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)

int main() {
    for (bool portrait : {false, true}) {
        Settings s;
        s.onboarded = true;
        s.portrait = portrait;
        Pet pet;
        ui::View v(s, pet);
        v.identity_key[0] = 17;
        v.now = 1000;
        v.screen = ui::Screen::Home;
        Detection d{};
        d.category = Category::FLIPPER;
        d.address = {2, 1, 2, 3, 4, 5};
        d.radio = Radio::Ble;
        d.score = 85;
        d.meal = true;
        d.unix_seconds = 1790035200;
        d.rssi_max = -42;
        d.rules[0] = &production_rules[0];
        d.rule_count = 1;
        pet.xp = 25;
        v.event(d);
        CHECK(v.companion.discoveries() == 1 && v.preview_companion.discoveries() == 0);
        CHECK(v.companion.scents[size_t(d.category)].first_utc == d.unix_seconds);
        CHECK(v.companion.unlocked == 3 && v.unlock_until > v.now);
        CHECK(v.requests & ui::Save);
        // The on-alert action target is frozen even while detections continue.
        v.tap(portrait ? 210 : 360, portrait ? 373 : 201);
        CHECK(v.screen == ui::Screen::AlertActions && !v.alert_until);
        auto another = d;
        another.address[5] = 6;
        v.event(another);
        CHECK(v.action_detection.address == d.address);
        const int row_y = portrait ? 80 : 66, step = portrait ? 55 : 34;
        v.tap(30, row_y + 3 * step + 12);
        CHECK(v.screen == ui::Screen::Home && v.companion.ignored_count() == 1);
        CHECK(!v.alert_allowed(d) && v.alert_allowed(another));
        auto before = v.companion.scents[size_t(d.category)].observations;
        v.alert_until = 0;
        v.event(d);
        CHECK(!v.alert_until && v.meal_until > v.now);
        CHECK(v.companion.scents[size_t(d.category)].observations == before + 1);
        CHECK(v.recent[0].address == d.address);
        auto different = d;
        different.category = Category::PINEAPPLE;
        CHECK(v.alert_allowed(different));
        different = d;
        different.address_type = 1;
        CHECK(v.alert_allowed(different));
        different = d;
        different.radio = Radio::Wifi;
        CHECK(v.alert_allowed(different));
        // Removal restores only the selected ignored device/category.
        v.screen = ui::Screen::Ignored;
        v.tap(30, (portrait ? 80 : 64) + 12);
        CHECK(!v.companion.ignored_count() && v.alert_allowed(d));
        for (int duration = 0; duration < 3; ++duration) {
            constexpr uint64_t minutes[] = {5, 15, 60};
            v.open_actions(d);
            v.tap(30, row_y + duration * step + 12);
            CHECK(v.snooze_until == v.now + minutes[duration] * 60000);
            CHECK(!v.alert_allowed(d) && !v.alert_allowed(another));
            v.now = v.snooze_until - 1;
            CHECK(!v.alert_allowed(d));
            ++v.now;
            CHECK(v.alert_allowed(d));
        }
        v.open_actions(d);
        v.tap(30, row_y + 12);
        v.screen = ui::Screen::Settings;
        v.settings_page = 0;
        v.tap(30, row_y + 4 * step + 12);
        CHECK(!v.snooze_until && v.alert_allowed(d));
        s.alert_categories &= ~(1U << unsigned(d.category));
        CHECK(!v.alert_allowed(d));
        s.alert_categories |= 1U << unsigned(d.category);
        s.threshold = 80;
        auto low = d;
        low.score = 50;
        CHECK(!v.alert_allowed(low));
        // Book navigation reaches all categories, including the final partial page.
        v.screen = ui::Screen::Home;
        v.tap(v.width() - 45, 44);
        CHECK(v.screen == ui::Screen::ScentBook);
        for (int page = 0; page < 4; ++page) {
            v.book_page = page;
            v.tap(30, portrait ? 95 : 80);
            CHECK(v.screen == ui::Screen::ScentCard && v.scent_index == page * 6);
            v.tap(v.width() - 30, v.height() - 65);
            CHECK(v.scent_page == 1);
            v.tap(30, v.height() - 65);
            CHECK(v.screen == ui::Screen::ScentBook);
        }
        // Locked previews cannot be equipped; accumulated XP unlocks permanent gear.
        v.screen = ui::Screen::Wardrobe;
        v.wardrobe_index = 4;
        v.tap(v.width() - 30, v.height() - 65);
        CHECK(v.companion.equipped == 0);
        pet.xp = 300;
        v.event(d);
        v.tap(v.width() - 30, v.height() - 65);
        CHECK(v.companion.equipped == 4 && (v.requests & ui::Save));
        for (unsigned i = 0; i < 10; ++i) {
            auto sighting = d;
            sighting.category = Category(i);
            v.event(sighting);
        }
        CHECK(v.companion.unlocked == 63);
        // Synthetic discoveries, ignores, rewards and snooze never modify the real collection.
        auto real = v.companion;
        auto real_xp = pet.xp;
        v.demo = true;
        auto fake = d;
        fake.demo = true;
        v.event(fake);
        CHECK(v.preview_companion.discoveries() == 1 && v.preview_xp == 25);
        v.open_actions(fake);
        v.tap(30, row_y + 3 * step + 12);
        CHECK(v.preview_companion.ignored_count() == 1 && !v.companion.ignored_count());
        v.open_actions(fake);
        v.tap(30, row_y + 12);
        CHECK(v.preview_snooze_until > v.now && !v.snooze_until);
        CHECK(v.companion.scents[size_t(d.category)].observations ==
              real.scents[size_t(d.category)].observations);
        CHECK(pet.xp == real_xp && v.companion.equipped == real.equipped);
        v.reset_progress();
        CHECK(!v.preview_xp && !v.preview_companion.discoveries() && v.companion.discoveries());
        CHECK(v.preview_companion.ignored_count() == 1);
        v.demo = false;
        // Every new screen, category, outfit, breed and animation frame renders inside its tile.
        std::vector<uint16_t> guarded(ui::View::max_tile_pixels + 2, 0xabcd);
        auto render = [&] {
            for (int y = 0; y < v.height(); y += v.tile_rows())
                v.render(y, {guarded.data() + 1, size_t(v.width() * v.tile_rows())});
            CHECK(guarded.front() == 0xabcd && guarded.back() == 0xabcd);
        };
        for (int screen = int(ui::Screen::ScentBook); screen <= int(ui::Screen::Ignored);
             ++screen) {
            v.screen = ui::Screen(screen);
            for (int page = 0; page < 4; ++page) {
                v.book_page = v.ignored_page = page;
                render();
            }
        }
        v.screen = ui::Screen::ScentCard;
        for (int category = 0; category < int(category_count); ++category)
            for (int page = 0; page < 3; ++page) {
                v.scent_index = category;
                v.scent_page = page;
                render();
            }
        for (int dog = 0; dog < 6; ++dog) {
            s.character = dog;
            for (unsigned outfit = 0; outfit < outfit_count; ++outfit) {
                v.companion.equipped = outfit;
                v.wardrobe_index = outfit;
                v.screen = ui::Screen::Wardrobe;
                render();
                v.screen = ui::Screen::Home;
                v.now = 10000000;
                v.alert_until = 0;
                v.meal_until = v.now + ui::View::meal_duration;
                for (int phase : {0, 1500, 2700, 3500, 4400, 5400, 6000}) {
                    v.now = 10000000 + phase;
                    render();
                }
                v.paused = true;
                v.meal_until = 0;
                render();
                v.paused = false;
            }
        }
    }
    Companion c;
    for (unsigned i = 1; i <= c.ignored.size(); ++i)
        CHECK(c.ignore(i, Category::FLOCK));
    CHECK(c.ignore(1, Category::FLOCK));
    CHECK(!c.ignore(999, Category::AXON) && c.ignored_count() == 16 && c.is_ignored(1));
    CHECK(!c.ignore(0, Category::FLOCK));
    CHECK(valid_companion(c));
    c.equipped = 5;
    CHECK(!valid_companion(c));
    c.unlocked = 63;
    CHECK(valid_companion(c));
    c.ignored[1] = c.ignored[0];
    CHECK(!valid_companion(c));
    storage::StateV2 legacy;
    legacy.meal_key[0] = 1;
    legacy.pet.xp = 320;
    legacy.settings.research = true;
    storage::State upgraded;
    CHECK(storage::migrate(legacy, upgraded));
    upgraded.companion.unlock(upgraded.pet.xp);
    CHECK(upgraded.version == 5 && storage::valid_current(upgraded));
    CHECK(upgraded.settings.research && upgraded.companion.unlocked == 23);
    CHECK(!upgraded.companion.discoveries());
    upgraded.companion.equipped = 6;
    CHECK(!storage::valid_current(upgraded));
    std::printf("%u companion checks passed\n", checks);
}
