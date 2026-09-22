// SPDX-License-Identifier: Apache-2.0
#include "assets_generated.h"
#include "signatures_generated.h"
#include "state.hpp"
#include "ui.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace sniffer;
static int checks{};
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::cerr << "Failed line " << __LINE__ << ": " #x "\n";                               \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)
int main() {
    Settings s;
    Pet p;
    ui::View v(s, p);
    CHECK(v.width() == 480 && v.height() == 320 && s.rotation_locked);
    v.rotate();
    CHECK(!s.portrait && !v.requests);
    s.onboarded = true;
    v.screen = ui::Screen::Settings;
    v.tap(30, 78);
    CHECK(!s.rotation_locked && (v.requests & ui::Save));
    v.requests = 0;
    v.tap(30, 111);
    CHECK(s.portrait && v.width() == 320 && v.height() == 480 && (v.requests & ui::Rotate));
    v.tap(30, 95);
    CHECK(s.rotation_locked);
    v.requests = 0;
    v.tap(280, 12);
    CHECK(s.portrait && !v.requests);
    v.tap(30, 95);
    v.tap(280, 12);
    CHECK(!s.portrait);
    v.screen = ui::Screen::Calibration;
    v.rotate();
    CHECK(!s.portrait);
    for (bool portrait : {false, true}) {
        const int w = display::width(portrait), h = display::height(portrait);
        for (int y : {0, 239, 479})
            for (int x : {0, 159, 319}) {
                auto mapped = display::from_portrait({x, y}, portrait);
                CHECK(mapped.x >= 0 && mapped.x < w && mapped.y >= 0 && mapped.y < h);
                auto back = display::to_portrait(mapped, portrait);
                CHECK(back.x == x && back.y == y);
            }
        for (int breed = 0; breed < 6; ++breed) {
            s = Settings{};
            s.portrait = portrait;
            v.screen = ui::Screen::Welcome;
            v.requests = 0;
            v.tap(w - 30, h - 70);
            CHECK(v.screen == ui::Screen::Choose && s.region == 1);
            int cols = portrait ? 2 : 3, cw = (w - 24) / cols;
            v.tap(20 + (breed % cols) * cw,
                  (portrait ? 88 : 70) + (breed / cols) * (portrait ? 106 : 98) + 10);
            CHECK(v.screen == ui::Screen::Name && s.character == breed);
            s.name.fill(0);
            for (int key : {24, 25})
                v.tap((portrait ? 16 : 20) + (key % (portrait ? 6 : 10)) * (portrait ? 48 : 44) +
                          10,
                      (portrait ? 128 : 110) + (key / (portrait ? 6 : 10)) * (portrait ? 48 : 42) +
                          10);
            CHECK(std::string_view(s.name.data()) == "YZ");
            v.tap(w - 30, h - 70);
            CHECK(v.screen == ui::Screen::Privacy);
            v.tap(30, h - 70);
            CHECK(s.onboarded && !s.research && v.screen == ui::Screen::Home);
            auto xp = p.xp;
            v.tap(140, 180);
            CHECK(p.xp == xp && v.happy_until > v.now);
            v.tap(w - 10, h - 20);
            CHECK(v.screen == ui::Screen::Settings);
        }
        Detection d{};
        d.rules[0] = &production_rules[0];
        d.rule_count = 1;
        d.category = d.rules[0]->category;
        d.score = 85;
        d.demo = true;
        d.meal = true;
        v.event(d);
        CHECK(v.recent_count > 0 && v.alert_until > v.now && v.counts[0] == 0);
        v.screen = ui::Screen::Home;
        v.tap(40, 120);
        CHECK(v.alert_until == 0);
        std::vector<uint16_t> guarded(w * v.tile_rows() + 2);
        guarded.front() = 0x1234;
        guarded.back() = 0x5678;
        for (int screen = 0; screen < 14; ++screen)
            for (int page = 0; page < 4; ++page) {
                v.screen = static_cast<ui::Screen>(screen);
                v.settings_page = page;
                for (int y = 0; y < h; y += v.tile_rows())
                    v.render(y, std::span(guarded).subspan(1, w * v.tile_rows()));
                CHECK(guarded.front() == 0x1234 && guarded.back() == 0x5678);
            }
        // Bad touch coordinates cannot navigate or toggle state.
        v.screen = ui::Screen::Settings;
        v.requests = 0;
        for (auto point : {display::Point{-1, 0}, display::Point{w, 80}, display::Point{10, h}})
            v.tap(point.x, point.y);
        CHECK(v.requests == 0 && v.screen == ui::Screen::Settings);
    }
    // Alert preferences are independent of observation collection and confidence thresholds.
    for (bool portrait : {false, true}) {
        s = Settings{};
        s.portrait = portrait;
        s.onboarded = true;
        v.screen = ui::Screen::Settings;
        v.settings_page = 0;
        v.tap(30, (portrait ? 80 : 66) + 4 * (portrait ? 55 : 34) + 10);
        CHECK(v.screen == ui::Screen::Alerts);
        for (int index = 0; index < int(category_count); ++index) {
            v.alert_page = index / 4;
            auto bit = 1U << index;
            auto before = s.alert_categories;
            v.tap(30, (portrait ? 80 : 64) + (index % 4) * (portrait ? 68 : 43) + 10);
            CHECK(s.alert_categories == (before ^ bit) && (v.requests & ui::Save));
            Detection alert{};
            alert.category = static_cast<Category>(index);
            alert.score = 85;
            alert.rules[0] = &production_rules[0];
            alert.rule_count = 1;
            v.alert_until = 0;
            auto logs = v.recent_count;
            v.event(alert);
            CHECK(v.recent_count == std::min(logs + 1, v.recent.size()));
            CHECK((v.alert_until > v.now) == bool(s.alert_categories & bit));
            alert.score = 20;
            CHECK(!should_alert(s, alert));
        }
    }
    // Qualifying meals remain visible under an alert and keep their own food identity.
    for (bool portrait : {false, true})
        for (int breed = 0; breed < 6; ++breed) {
            Settings options{};
            options.portrait = portrait;
            options.character = breed;
            options.onboarded = true;
            Pet dog;
            ui::View scene(options, dog);
            scene.screen = ui::Screen::Home;
            scene.now = 1000;
            Detection meal{};
            meal.category = Category::FLIPPER;
            meal.score = 85;
            meal.demo = true;
            meal.meal = true;
            meal.rules[0] = &production_rules[0];
            meal.rule_count = 1;
            scene.event(meal);
            CHECK(scene.snack_pose().phase == ui::SnackPhase::Turn);
            scene.now = 1800;
            CHECK(scene.snack_pose().phase == ui::SnackPhase::Approach);
            auto start = scene.snack_pose().approach;
            scene.now = 3000;
            CHECK(scene.snack_pose().approach > start);
            CHECK(scene.snack_pose().frame >= assets::side_walk &&
                  scene.snack_pose().frame < assets::side_eat);
            scene.now = 3500;
            CHECK(scene.snack_pose().phase == ui::SnackPhase::Chew &&
                  scene.snack_pose().bites == 0);
            // A later low-confidence observation cannot replace the treat or alert being shown.
            auto later = meal;
            later.category = Category::PINEAPPLE;
            later.meal = false;
            later.score = 20;
            scene.event(later);
            CHECK(scene.recent[0].category == Category::PINEAPPLE &&
                  scene.meal_category == Category::FLIPPER);
            CHECK(scene.alert_detection.category == Category::FLIPPER);
            std::vector<uint16_t> visible(scene.width() * scene.height()),
                dismissed(visible.size());
            auto capture = [&](std::vector<uint16_t> &out) {
                for (int y = 0; y < scene.height(); y += scene.tile_rows())
                    scene.render(y, std::span(out).subspan(y * scene.width(),
                                                           scene.width() * scene.tile_rows()));
            };
            capture(visible);
            scene.tap(20, 100);
            CHECK(scene.alert_until == 0 && scene.snack_pose().phase == ui::SnackPhase::Chew);
            capture(dismissed);
            bool stage_unchanged = true;
            for (int y = 64; y < (portrait ? 290 : 256); ++y)
                for (int x = 12; x < (portrait ? 308 : 292); ++x)
                    stage_unchanged &=
                        visible[y * scene.width() + x] == dismissed[y * scene.width() + x];
            CHECK(stage_unchanged); // Regression: the old overlay obscured this whole area.
            for (int bite = 1; bite <= 4; ++bite) {
                scene.now = 3300 + bite * 800;
                CHECK(scene.snack_pose().bites == bite);
                capture(visible); // Exercise every bite and all profile frames under ASan/UBSan.
            }
            scene.now = 6750;
            CHECK(scene.snack_pose().phase == ui::SnackPhase::Happy);
            scene.now = 8000;
            CHECK(scene.snack_pose().phase == ui::SnackPhase::None);
            CHECK(dog.xp == 0 && dog.meals == 0);
            meal.category = Category::PINEAPPLE;
            scene.event(meal);
            CHECK(scene.meal_category == Category::PINEAPPLE);
            options.reduced_animation = true;
            auto still = scene.snack_pose();
            scene.now += 4000;
            CHECK(scene.snack_pose().frame == still.frame &&
                  scene.snack_pose().bites == still.bites);
            CHECK(still.phase == ui::SnackPhase::Chew && still.approach == 100);
            capture(visible);
        }
    constexpr const char *dogs[] = {"CORGI", "BEAGLE", "SHIBA", "HUSKY", "LABRADOR", "DALMATIAN"};
    for (int i = 0; i < 6; ++i)
        CHECK(std::string_view(assets::names[i]) == dogs[i]);
    storage::StateV1 old{};
    old.settings.character = 5;
    old.settings.region = 1;
    old.settings.research = true;
    old.settings.onboarded = true;
    old.settings.name = {'B', 'U', 'D', 'D', 'Y', 0};
    old.pet.xp = 321;
    old.pet.meals = 17;
    old.meal_key[4] = 93;
    old.recent[3] = {123456, 70000};
    old.calibration.valid = true;
    old.calibration.ax = 0.23f;
    storage::State current{};
    CHECK(storage::migrate(old, current));
    CHECK(current.version == 3 && current.settings.version == 2);
    CHECK(current.pet.xp == 321 && current.pet.meals == 17 && current.settings.character == 5 &&
          current.settings.region == 1);
    CHECK(current.settings.name == old.settings.name && current.settings.research &&
          current.settings.onboarded);
    CHECK(current.meal_key == old.meal_key && current.recent[3].hash == 123456 &&
          current.recent[3].remaining_ms == 70000);
    CHECK(current.calibration.valid && current.calibration.ax == old.calibration.ax);
    CHECK(!current.settings.portrait && current.settings.rotation_locked);
    old.settings.character = 6;
    CHECK(!storage::migrate(old, current));
    CHECK(sizeof(storage::SettingsV1) == 32);
    std::cout << checks << " UI, rotation, touch mapping and save migration checks passed\n";
}
