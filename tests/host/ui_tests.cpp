// SPDX-License-Identifier: Apache-2.0
#include "assets_generated.h"
#include "scent_guide.hpp"
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
void export_feedback_tests() {
    for (bool portrait : {false, true})
        for (int page : {3, 5}) {
            Settings settings;
            settings.onboarded = true;
            settings.portrait = portrait;
            Pet dog;
            ui::View view(settings, dog);
            view.screen = ui::Screen::Settings;
            view.settings_page = page;
            view.sd = true;
            view.export_progress(1, 12); // An earlier export must not complete a new one.
            int row = page == 3 ? 1 : 4;
            int top = (portrait ? 80 : 66) + row * (portrait ? 55 : 34);
            auto tap = [&] { view.tap(30, top + 10); };
            auto frame = [&] {
                std::vector<uint16_t> pixels(view.width() * view.height() + 2);
                pixels.front() = 0x1234;
                pixels.back() = 0x5678;
                for (int y = 0; y < view.height(); y += view.tile_rows())
                    view.render(y, std::span(pixels).subspan(1 + y * view.width(),
                                                             view.width() * view.tile_rows()));
                CHECK(pixels.front() == 0x1234 && pixels.back() == 0x5678);
                return pixels;
            };
            const auto before = frame();
            tap();
            CHECK(view.requests & ui::Export);
            CHECK(view.export_status == ui::ExportStatus::Exporting);
            view.requests = 0;
            tap();
            CHECK(!view.requests); // Repeated taps cannot flood the storage worker.
            view.export_queued(true);
            view.export_progress(1, 12);
            CHECK(view.export_status == ui::ExportStatus::Exporting);
            view.export_progress(2, 32);
            CHECK(view.export_status == ui::ExportStatus::Complete);
            CHECK(view.exported_rows == 32 && view.export_count == 2);
            std::array<char, 64> message{};
            view.export_message(message);
            CHECK(std::string_view(message.data()) == "COMPLETE: 32 RECORDS");
            const auto after = frame();
            CHECK(after != before);
            for (int y = 0; y < view.height(); ++y)
                if (y < top || y >= top + (portrait ? 48 : 30))
                    CHECK(std::equal(before.begin() + 1 + y * view.width(),
                                     before.begin() + 1 + (y + 1) * view.width(),
                                     after.begin() + 1 + y * view.width()));
            view.export_progress(2, 32);
            CHECK(view.export_status == ui::ExportStatus::Complete); // Persistent acknowledgement.
            tap();
            CHECK(view.export_status == ui::ExportStatus::Exporting);
            view.export_queued(false);
            CHECK(view.export_status == ui::ExportStatus::QueueFull);
            view.requests = 0;
            tap(); // Retry queue saturation.
            CHECK(view.requests & ui::Export);
            view.export_progress(3, 0);
            CHECK(view.export_status == ui::ExportStatus::Complete && view.exported_rows == 0);
            view.requests = 0;
            view.demo = true;
            tap();
            CHECK(!view.requests && view.export_status == ui::ExportStatus::Demo);
            view.demo = false;
            view.sd = false;
            tap();
            CHECK(!view.requests && view.export_status == ui::ExportStatus::NoCard);
            view.sd = true;
            view.sd_read_only = true;
            tap();
            CHECK(!view.requests && view.export_status == ui::ExportStatus::ReadOnly);
            view.sd_read_only = false;
            view.sd_error = true;
            tap();
            CHECK(!view.requests && view.export_status == ui::ExportStatus::Failed);
            view.sd_error = false;
            tap();
            CHECK(view.export_status == ui::ExportStatus::Exporting);
            view.sd_error = true; // A failed worker operation never reports completion.
            view.export_progress(3, 0);
            CHECK(view.export_status == ui::ExportStatus::Failed);
            view.exported_rows = UINT32_MAX;
            for (auto status :
                 {ui::ExportStatus::Exporting, ui::ExportStatus::Complete, ui::ExportStatus::NoCard,
                  ui::ExportStatus::ReadOnly, ui::ExportStatus::Failed, ui::ExportStatus::QueueFull,
                  ui::ExportStatus::Demo}) {
                view.export_status = status;
                view.export_message(message);
                CHECK(std::strlen(message.data()) * 6U <= unsigned(view.width() - 44));
                CHECK(!message.empty() && message[0]);
                frame();
            }
        }
}
void eject_feedback_tests() {
    for (bool portrait : {false, true}) {
        Settings settings;
        settings.onboarded = true;
        settings.portrait = portrait;
        Pet dog;
        ui::View view(settings, dog);
        view.screen = ui::Screen::Settings;
        view.settings_page = 3;
        view.sd = true;
        const int y = (portrait ? 80 : 66) + 2 * (portrait ? 55 : 34) + 10;
        auto tap = [&] { view.tap(30, y); };
        tap();
        CHECK(view.requests & ui::Eject);
        CHECK(view.eject_status == ui::EjectStatus::Ejecting);
        view.requests = 0;
        tap();
        CHECK(!view.requests);
        view.eject_queued(false);
        CHECK(view.eject_status == ui::EjectStatus::QueueFull);
        tap();
        CHECK(view.requests & ui::Eject);
        view.requests = 0;
        view.eject_queued(true);
        view.request_export();
        CHECK(!view.requests && view.export_status == ui::ExportStatus::NoCard);
        view.eject_progress(false, false);
        CHECK(view.eject_status == ui::EjectStatus::Ejecting);
        view.sd_error = true; // An older SD error must not hide actual close success.
        view.eject_progress(true, false);
        CHECK(view.eject_status == ui::EjectStatus::Safe);
        CHECK(std::string_view(view.eject_message()) == "SAFE TO REMOVE");
        tap();
        CHECK(!view.requests);
        view.eject_progress(true, true);
        CHECK(view.eject_status == ui::EjectStatus::Failed);
        tap();
        CHECK(!view.requests && view.eject_status == ui::EjectStatus::Failed);
        view.demo = true;
        tap();
        view.eject_progress(true, false);
        CHECK(!view.requests && view.eject_status == ui::EjectStatus::Demo);
        view.demo = false;
        view.sd = false;
        view.eject_status = ui::EjectStatus::Idle;
        tap();
        CHECK(!view.requests && view.eject_status == ui::EjectStatus::NoCard);
        for (auto status :
             {ui::EjectStatus::Ejecting, ui::EjectStatus::Safe, ui::EjectStatus::Failed,
              ui::EjectStatus::QueueFull, ui::EjectStatus::NoCard, ui::EjectStatus::Demo}) {
            view.eject_status = status;
            CHECK(std::strlen(view.eject_message()) * 6U <= unsigned(view.width() - 44));
            std::vector<uint16_t> pixels(view.width() * view.height() + 2);
            pixels.front() = 0x1234;
            pixels.back() = 0x5678;
            for (int row = 0; row < view.height(); row += view.tile_rows())
                view.render(row, std::span(pixels).subspan(1 + row * view.width(),
                                                           view.width() * view.tile_rows()));
            CHECK(pixels.front() == 0x1234 && pixels.back() == 0x5678);
        }
    }
}
void care_tests() {
    for (bool portrait : {false, true})
        for (int breed = 0; breed < 6; ++breed) {
            Settings s;
            s.onboarded = true;
            s.portrait = portrait;
            s.character = breed;
            Pet p;
            ui::View v(s, p);
            v.screen = ui::Screen::Home;
            p.fullness = 25;
            CHECK(v.needs_care());
            p.fullness = 70;
            p.mood = 25;
            CHECK(v.needs_care());
            p.mood = 26;
            CHECK(!v.needs_care());
            v.demo = true;
            v.now = Pet::needs_interval_ms;
            v.tick_pet();
            CHECK(v.fullness() == 67 && v.mood() == 68);
            CHECK(p.fullness == 70 && p.mood == 26); // Demo cannot change saved needs.
            v.preview_fullness = v.preview_mood = 0;
            CHECK(v.needs_care());
            Detection d{};
            d.demo = true;
            d.meal = true;
            d.score = 60;
            d.category = Category::SAMSUNG_TAG;
            v.event(d);
            CHECK(v.fullness() == 35 && v.mood() == 40 && !v.needs_care());
            CHECK(p.meals == 0 && p.xp == 0 && p.mood == 26);
            auto start = v.now;
            v.now = start + 5700;
            CHECK(v.celebrating() && v.celebration_jump() == 0);
            v.now = start + 6000;
            CHECK(v.celebrating() && v.celebration_jump() == 14);
            v.now = start + 7500;
            CHECK(v.celebrating());
            s.reduced_animation = true;
            CHECK(v.celebration_jump() == 0);
            v.paused = true;
            CHECK(!v.celebrating());
            v.paused = false;
            v.now = start + 9500;
            CHECK(!v.celebrating());
            v.demo = false;
            v.now = 10800000;
            v.reset_progress();
            v.tick_pet();
            CHECK(p.fullness == 70 && p.mood == 70 && p.decay_ms == v.now);
            v.demo = true;
            std::vector<uint16_t> tile(v.width() * v.tile_rows() + 2);
            tile.front() = 0x1234;
            tile.back() = 0x5678;
            for (bool reduced : {false, true}) {
                s.reduced_animation = reduced;
                for (uint8_t need : {uint8_t(0), uint8_t(25), uint8_t(26)}) {
                    v.preview_fullness = v.preview_mood = need;
                    for (int y = 0; y < v.height(); y += v.tile_rows())
                        v.render(y, std::span(tile).subspan(1, v.width() * v.tile_rows()));
                    CHECK(tile.front() == 0x1234 && tile.back() == 0x5678);
                }
            }
        }
}
void coverage_tests() {
    for (bool portrait : {false, true})
        for (uint8_t theme : {0, 1, 2}) {
            Settings s;
            s.onboarded = true;
            s.portrait = portrait;
            Pet dog;
            ui::View v(s, dog);
            v.look().theme = theme;
            v.scanning = true;
            v.screen = ui::Screen::ScentBook;
            v.now = 1000;
            auto frame = [&] {
                std::vector<uint16_t> pixels(v.width() * v.height() + 2, 0xabcd);
                for (int y = 0; y < v.height(); y += v.tile_rows())
                    v.render(
                        y, std::span(pixels).subspan(1 + y * v.width(), v.width() * v.tile_rows()));
                CHECK(pixels.front() == 0xabcd && pixels.back() == 0xabcd);
                return pixels;
            };
            for (int book_page : {0, 3}) {
                v.book_page = book_page;
                frame();
                v.tap(30, v.height() - 26);
                CHECK(v.screen == ui::Screen::Coverage && v.coverage_page == 0);
                const auto first = frame();
                for (size_t page = 0; page < std::size(ui::coverage_guide); ++page) {
                    CHECK(v.coverage_page == int(page));
                    const auto pixels = frame();
                    if (page)
                        CHECK(pixels != first);
                    v.tap(v.width() - 30, v.height() - 65);
                }
                CHECK(v.coverage_page == 0); // Last page wraps to the beginning.
                v.tap(30, v.height() - 65);
                CHECK(v.screen == ui::Screen::ScentBook && v.book_page == book_page);
            }
            CHECK(v.listening() && !v.requests);
            CHECK(v.collection().discoveries() == 0 && v.collection().ignored_count() == 0);
            CHECK(dog.meals == 0 && dog.xp == 0 && dog.fullness == 70 && dog.mood == 70);
            v.tap(30, v.height() - 26);
            // Reading help must not continually wake the display or stop reception.
            v.now += 120000;
            frame();
            v.update_display();
            CHECK(v.display_mode == ui::DisplayMode::Dim && v.listening());
            v.tap(v.width() - 30, v.height() - 65);
            CHECK(v.display_mode == ui::DisplayMode::Active && v.coverage_page == 0);
            v.tap(v.width() - 30, v.height() - 65);
            CHECK(v.coverage_page == 1);
            Detection d{};
            d.category = Category::SAMSUNG_TAG;
            d.score = 60;
            d.last_ms = v.now;
            v.event(d);
            CHECK(v.screen == ui::Screen::Coverage && v.listening());
            CHECK(v.collection().discoveries() == 1);
        }
}
int main() {
    coverage_tests();
    care_tests();
    export_feedback_tests();
    eject_feedback_tests();
    Settings s;
    Pet p;
    ui::View v(s, p);
    v.save_pending = true;
    CHECK(std::string_view(v.home_notice()) == "SAVE WAITING / RETRYING");
    std::snprintf(v.notice.data(), v.notice.size(), "CRITICAL BATTERY");
    CHECK(std::string_view(v.home_notice()) == "CRITICAL BATTERY");
    v.save_pending = false;
    CHECK(std::string_view(v.home_notice()) == "CRITICAL BATTERY");
    v.notice[0] = 0;
    CHECK(!v.home_notice()[0]);
    v.memory_status(70000, 69000, 32768);
    CHECK(v.low_heap && std::string_view(v.home_notice()) == "LOW HEAP - SEE DIAGNOSTICS");
    CHECK(v.heap == 70000 && v.min_heap == 69000 && v.largest_heap == 32768);
    CHECK(v.notice[0] == 0); // A health warning never becomes a sticky user notice.
    std::snprintf(v.notice.data(), v.notice.size(), "CRITICAL BATTERY - SAVING AND SLEEPING");
    CHECK(std::string_view(v.home_notice()) == "CRITICAL BATTERY - SAVING AND SLEEPING");
    v.memory_status(80000, 69000, 40000);
    CHECK(!v.low_heap && v.notice[0]);
    v.notice[0] = 0;
    CHECK(!v.home_notice()[0]); // Recovery clears the banner despite the older
                                // minimum.
    v.memory_status(79999, 69000, 40000);
    CHECK(v.low_heap);
    v.memory_status(90000, 69000, 40000);
    CHECK(!v.low_heap);
    CHECK(v.width() == 480 && v.height() == 320 && s.rotation_locked);
    v.rotate();
    CHECK(!s.portrait && !v.requests);
    s.onboarded = true;
    v.screen = ui::Screen::Settings;
    v.settings_page = 1;
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
        v.settings_page = 1;
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
    CHECK(current.version == 5 && current.settings.version == 2);
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
