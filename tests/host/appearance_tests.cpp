// SPDX-License-Identifier: Apache-2.0
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
            std::fprintf(stderr, "appearance:%d: %s\n", __LINE__, #x);                             \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)
int main() {
    storage::State saved;
    saved.meal_key[0] = 1;
    CHECK(storage::valid_current(saved));
    for (auto member : {&Appearance::theme, &Appearance::scene, &Appearance::speech,
                        &Appearance::compact, &Appearance::inverted, &Appearance::dim_minutes,
                        &Appearance::saver_minutes, &Appearance::off_minutes}) {
        auto invalid = saved;
        invalid.appearance.*member = 255;
        CHECK(!storage::valid_current(invalid));
    }
    saved.appearance.version = 4;
    CHECK(!storage::valid_current(saved));
    storage::StateV2 legacy;
    legacy.meal_key[0] = 2;
    legacy.settings.research = true;
    CHECK(storage::migrate(legacy, saved));
    CHECK(saved.version == 5 && saved.appearance.theme == 0 && saved.appearance.speech == 1 &&
          saved.settings.research);
    AppearanceV1 old_look;
    old_look.theme = 2;
    old_look.scene = 1;
    old_look.speech = 0;
    old_look.compact = 1;
    Appearance upgraded;
    upgraded.inverted = 1;
    CHECK(migrate_appearance(old_look, upgraded));
    CHECK(upgraded.version == 3 && upgraded.theme == 2 && upgraded.scene == 1 &&
          upgraded.speech == 0 && upgraded.compact == 1 && !upgraded.inverted);
    old_look.scene = 255;
    CHECK(!migrate_appearance(old_look, upgraded) && upgraded.scene == 1);
    for (bool portrait : {false, true}) {
        Settings s;
        Pet p;
        ui::View v(s, p);
        s.portrait = portrait;
        s.onboarded = true;
        v.now = 50000;
        int ry = portrait ? 80 : 66, step = portrait ? 55 : 34;
        // Entering Settings from the main navigation or BOOT always opens daily controls.
        v.settings_page = 5;
        v.screen = ui::Screen::Home;
        v.tap(v.width() - 20, v.height() - 20);
        CHECK(v.screen == ui::Screen::Settings && v.settings_page == 0);
        v.tap(30, ry + 10);
        CHECK(v.screen == ui::Screen::Wardrobe);
        v.settings_page = 5;
        v.screen = ui::Screen::Detectors;
        v.next();
        CHECK(v.screen == ui::Screen::Settings && v.settings_page == 0);
        v.tap(30, ry + step * 2 + 10);
        CHECK(v.screen == ui::Screen::ScentBook);
        v.screen = ui::Screen::Settings;
        v.settings_page = 0;
        v.tap(30, ry + step + 10);
        CHECK(v.screen == ui::Screen::Appearance);
        for (int row = 0; row < 4; ++row)
            v.tap(30, ry + step * row + 10);
        CHECK(v.appearance.theme == 1 && v.appearance.scene == 1 && v.appearance.speech == 2 &&
              v.appearance.compact == 1);
        CHECK(v.requests & ui::Save);
        v.tap(30, v.height() - 65);
        CHECK(v.screen == ui::Screen::Home);
        v.tap(120, 150);
        CHECK(v.pets == 1 && v.happy_until > v.now && v.speaking());
        v.look().speech = 0;
        CHECK(!v.speaking());
        v.look().speech = 1;
        v.happy_until = 0;
        v.now = 24000;
        CHECK(v.speaking());
        v.now = 30000;
        CHECK(!v.speaking());
        v.screen = ui::Screen::Settings;
        v.settings_page = 3;
        v.tap(30, ry + step * 3 + 10);
        CHECK(v.demo && v.look().theme == 1);
        v.screen = ui::Screen::Appearance;
        v.tap(30, ry + 10);
        CHECK(v.look().theme == 2 && v.appearance.theme == 1);
        v.demo = false;
        // The on-screen hardware option requests an immediate panel command and save.
        v.screen = ui::Screen::Settings;
        v.settings_page = 1;
        v.tap(30, ry + step * 2 + 10);
        CHECK(v.screen == ui::Screen::Display);
        v.requests = 0;
        v.tap(30, ry + step + 10);
        CHECK(v.appearance.inverted == 1 && (v.requests & ui::Invert) && (v.requests & ui::Save));
        v.tap(30, ry + step + 10);
        CHECK(v.appearance.inverted == 0);
        v.screen = ui::Screen::Settings;
        v.settings_page = 3;
        v.tap(30, ry + step * 3 + 10);
        CHECK(v.demo && !v.preview_appearance.inverted);
        v.screen = ui::Screen::Display;
        v.tap(30, ry + step + 10);
        CHECK(v.preview_appearance.inverted == 1 && !v.appearance.inverted);
        v.screen = ui::Screen::Settings;
        v.settings_page = 3;
        v.tap(30, ry + step * 3 + 10);
        CHECK(!v.demo && !v.look().inverted && (v.requests & ui::Demo));
        auto frame = [&] {
            std::vector<uint16_t> result;
            std::vector<uint16_t> tile(ui::View::max_tile_pixels + 2, 0xabcd);
            for (int y = 0; y < v.height(); y += v.tile_rows()) {
                v.render(y, {tile.data() + 1, size_t(v.width() * v.tile_rows())});
                result.insert(result.end(), tile.begin() + 1,
                              tile.begin() + 1 + v.width() * v.tile_rows());
            }
            CHECK(tile.front() == 0xabcd && tile.back() == 0xabcd);
            return result;
        };
        for (int theme = 0; theme < 3; ++theme) {
            v.look().theme = theme;
            for (int scene = 0; scene < 3; ++scene) {
                v.look().scene = scene;
                v.look().speech = 0;
                for (bool compact : {false, true}) {
                    v.look().compact = compact;
                    v.screen = ui::Screen::Home;
                    s.reduced_animation = true;
                    v.now = 100000;
                    v.happy_until = 0;
                    auto a = frame();
                    v.now += 800;
                    auto b = frame();
                    CHECK(a == b);
                    v.paused = true;
                    a = frame();
                    v.now += 800;
                    b = frame();
                    CHECK(a == b);
                    v.paused = false;
                    s.reduced_animation = false;
                }
            }
            for (int screen = 0; screen <= int(ui::Screen::Display); ++screen) {
                v.screen = ui::Screen(screen);
                frame();
            }
        }
    }
    std::printf("%u appearance checks passed\n", checks);
}
