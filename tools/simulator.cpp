// SPDX-License-Identifier: Apache-2.0
// Shared synthetic session for native and browser adapters. No board or storage linkage.
#include "simulator.hpp"
#include "assets_generated.h"
#include "signatures_generated.h"
#include "ui.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>

namespace hound_lab {
namespace {
using namespace sniffer;
constexpr const char *screens[] = {"Welcome",
                                   "Calibration",
                                   "Choose pet",
                                   "Name pet",
                                   "Privacy",
                                   "Pet",
                                   "Log",
                                   "Detectors",
                                   "Settings",
                                   "Diagnostics",
                                   "Details",
                                   "Research warning",
                                   "Reset pet",
                                   "Alert types",
                                   "Set time",
                                   "Battery setup",
                                   "Clear logs",
                                   "Self test",
                                   "Scent Book",
                                   "Scent card",
                                   "Wardrobe",
                                   "Quiet alerts",
                                   "Ignored scents",
                                   "Follow Scent",
                                   "Atmosphere",
                                   "Display options",
                                   "Tag Watch",
                                   "Watch progress",
                                   "Screen saver",
                                   "Ignore backup",
                                   "Detection coverage"};
static_assert(std::size(screens) == size_t(ui::Screen::Coverage) + 1);
constexpr Rule flipper{"demo.flipper",
                       Category::FLIPPER,
                       Kind::Protocol,
                       "synthetic",
                       85,
                       100,
                       false,
                       "Synthetic Flipper",
                       "DEMO: synthetic evidence only"};
constexpr Rule pineapple{"demo.pineapple",
                         Category::PINEAPPLE,
                         Kind::Protocol,
                         "synthetic",
                         65,
                         79,
                         false,
                         "Synthetic Pineapple",
                         "DEMO: synthetic evidence only"};
struct Simulator {
    Settings settings;
    Pet pet;
    ui::View view{settings, pet};
    unsigned events{};
    uint64_t time_offset{};
    std::array<Rule, category_count> samples{};
    Simulator() {
        settings.onboarded = true;
        view.screen = ui::Screen::Home;
        view.demo = true;
        view.identity_key.fill(0xA5); // Fixed identity key only for these synthetic samples.
        for (size_t i = 0; i < category_count; ++i)
            samples[i] = {"demo.category",
                          Category(i),
                          Kind::Protocol,
                          "synthetic",
                          85,
                          100,
                          false,
                          categories[i],
                          "DEMO: synthetic category evidence"};
    }
    void inject(int type, int dbm, bool meal = true) {
        if (view.paused) {
            std::snprintf(view.notice.data(), view.notice.size(),
                          "HOUND SLEEPING - START SNIFFING FIRST");
            return;
        }
        Detection d{};
        if (type < 2) {
            d.rules[0] = type == 0 ? &flipper : &pineapple;
        } else if (type >= 4 && type < int(category_count) + 4) {
            d.rules[0] = &samples[type - 4];
        } else {
            const char *id = type == 2 ? "ble.google.find_hub" : "ble.google.fast_pair";
            for (const auto &rule : production_rules)
                if (std::string_view(rule.id) == id)
                    d.rules[0] = &rule;
        }
        if (!d.rules[0])
            return;
        d.category = d.rules[0]->category;
        d.address = {2, 0, 0, 0, 0, uint8_t(unsigned(d.category) + 1)};
        d.unix_seconds = view.utc_now();
        d.score = d.rules[0]->score;
        d.rule_count = 1;
        d.demo = true;
        d.meal = meal && d.score >= 50;
        d.radio = type == 1 ? Radio::Wifi : Radio::Ble;
        d.first_ms = d.last_ms = view.now;
        d.seen_count = 1;
        d.rssi_min = d.rssi_max = dbm >= 30 && dbm <= 100 ? -dbm : -66;
        d.rssi_sum = d.rssi_min;
        if (!(settings.enabled_categories & (1U << static_cast<unsigned>(d.category)))) {
            std::snprintf(view.notice.data(), view.notice.size(), "CATEGORY IS DISABLED");
            return;
        }
        Observation o{};
        o.address = d.address;
        o.radio = d.radio;
        o.ms = view.now;
        o.rssi = d.rssi_min;
        view.observe(o, true);
        if (view.screen != ui::Screen::Follow)
            view.screen = ui::Screen::Home;
        view.event(d);
        ++events;
    }
    void handle_requests() {
        unsigned requests = view.requests;
        view.requests = 0;
        view.demo = true; // All host activity remains synthetic, even after the
                          // device demo toggle.
        if (requests & ui::Export)
            view.export_status = ui::ExportStatus::Demo;
        if (requests & ui::Eject)
            view.eject_status = ui::EjectStatus::Demo;
        if (requests & ui::DiagnosticCopy)
            std::snprintf(view.notice.data(), view.notice.size(), "SIMULATOR: NO SD WRITES");
        if (requests & ui::Calibrate)
            view.calibration_step = 0;
        if (requests & ui::ClearLogs) {
            view.recent_count = 0;
            view.log_page = 0;
        }
        if (requests & ui::Reset)
            view.reset_progress();
    }
    void render(Frame &frame) {
        std::ostringstream state;
        std::array<char, 80> escaped_name{};
        json_escape(settings.name.data(), escaped_name);
        state << "{\"screen\":\"" << screens[static_cast<unsigned>(view.screen)] << "\",\"pet\":\""
              << assets::names[settings.character] << "\",\"name\":" << escaped_name.data()
              << ",\"xp\":" << pet.xp << ",\"preview_xp\":" << view.preview_xp
              << ",\"discoveries\":" << view.collection().discoveries()
              << ",\"ignored\":" << view.collection().ignored_count()
              << ",\"ignore_capacity\":" << ignore_capacity
              << ",\"snoozed\":" << (view.snoozed_until() > view.now ? "true" : "false")
              << ",\"following\":" << (view.follow.active ? "true" : "false")
              << ",\"tag_watch_armed\":" << (view.tag_watch.armed ? "true" : "false")
              << ",\"tag_watch_warning\":" << (view.watch_warning() ? "true" : "false")
              << ",\"tag_watch_red\":" << (view.watch_red() ? "true" : "false")
              << ",\"display_mode\":" << unsigned(view.display_mode)
              << ",\"brightness\":" << view.display_brightness()
              << ",\"outfit\":" << unsigned(view.collection().equipped)
              << ",\"mood\":" << unsigned(view.mood())
              << ",\"fullness\":" << unsigned(view.fullness()) << ",\"width\":" << view.width()
              << ",\"height\":" << view.height()
              << ",\"rotation_locked\":" << (settings.rotation_locked ? "true" : "false")
              << ",\"alert_categories\":" << settings.alert_categories
              << ",\"alert_active\":" << (view.alert_until > view.now ? "true" : "false")
              << ",\"paused\":" << (view.paused ? "true" : "false")
              << ",\"log_page\":" << view.log_page
              << ",\"snack_phase\":" << int(view.snack_pose().phase)
              << ",\"snack_bites\":" << view.snack_pose().bites
              << ",\"settings_page\":" << view.settings_page << ",\"events\":" << events
              << ",\"demo\":true,\"onboarded\":" << (settings.onboarded ? "true" : "false")
              << "}\n";
        frame.state = state.str();
        std::array<uint16_t, ui::View::max_tile_pixels> tile{};

        for (int y = 0; y < view.height(); y += view.tile_rows()) {
            view.render(y, std::span(tile).first(view.width() * view.tile_rows()));
            for (size_t i = 0; i < size_t(view.width() * view.tile_rows()); ++i) {
                // Emulate the panel command; native firmware changes LCD polarity instead.
                uint16_t pixel = view.look().inverted ? uint16_t(~tile[i]) : tile[i];
                // Model the physical backlight, including true zero while blanked.
                unsigned light = view.display_brightness();
                pixel = uint16_t(((((pixel >> 11) & 31) * light / 100) << 11) |
                                 ((((pixel >> 5) & 63) * light / 100) << 5) |
                                 ((pixel & 31) * light / 100));
                frame.pixels[2 * (size_t(y) * view.width() + i)] =
                    static_cast<uint8_t>(pixel & 255);
                frame.pixels[2 * (size_t(y) * view.width() + i) + 1] =
                    static_cast<uint8_t>(pixel >> 8);
            }
        }
    }
};
} // namespace
const Frame &step(std::string_view command, int a, int b, uint64_t ms) {
    static auto app = std::make_unique<Simulator>();
    static Frame frame;
    if (command == "reset" || command == "onboard") {
        app = std::make_unique<Simulator>();
        if (command == "onboard") {
            app->settings.onboarded = false;
            app->view.screen = ui::Screen::Welcome;
        }
    }
    ms += app->time_offset;
    app->view.now = ms;
    app->view.tick_pet();
    app->view.update_tag_watch();
    if (command == "reset" || command == "onboard")
        app->view.wake_display();
    app->view.update_display();
    if (command == "tap" && a >= 0 && a < app->view.width() && b >= 0 && b < app->view.height()) {
        if (app->view.screen == ui::Screen::Calibration) {
            if (++app->view.calibration_step == 3)
                app->view.screen = app->settings.onboarded ? ui::Screen::Home : ui::Screen::Welcome;
        } else
            app->view.tap(a, b);
    } else if (command == "screen" && (a == 2 || a == 5 || a == 6 || a == 7 || a == 8 || a == 9 ||
                                       a == 13 || a == 18 || a == 20 || a == 22 || a == 23 ||
                                       a == 24 || a == 25 || a == 26 || a == 27 || a == 28)) {
        app->view.wake_display();
        app->view.screen = static_cast<ui::Screen>(a);
        if (app->view.screen == ui::Screen::Settings)
            app->view.settings_page = 0;
        if (app->view.screen == ui::Screen::TagWatch)
            app->view.open_tag_watch();
    } else if (command == "rotate") {
        app->view.rotate();
    } else if (command == "boot") {
        app->view.next();
    } else if (command == "signal" && b >= 30 && b <= 100) {
        Observation o{};
        o.address = app->view.follow.address;
        o.radio = app->view.follow.radio;
        o.address_type = app->view.follow.address_type;
        o.ms = ms;
        o.rssi = -b;
        app->view.observe(o, true);
    } else if (command == "tag_watch_test" && !app->view.paused &&
               (a == 9 || a == 10 || a == 11 || a == 12)) {
        // Exercise the real ten-minute rule on a virtual clock; never relax firmware
        // thresholds.
        app->view.tag_watch.start(ms, true);
        for (unsigned minute = 0; minute <= 10; ++minute) {
            app->view.now = ms + minute * 60000;
            app->inject(a, b, false);
        }
        app->time_offset += 600000;
    } else if (command == "pet_care" && a >= 0 && a <= 2) {
        app->view.screen = ui::Screen::Home;
        app->view.meal_until = app->view.celebration_until = app->view.happy_until = 0;
        app->view.unlock_until = app->view.level_until = app->view.alert_until = 0;
        if (a < 2) {
            app->view.preview_fullness = a == 0 ? 15 : 60;
            app->view.preview_mood = a == 1 ? 15 : 60;
            app->view.preview_decay_ms = ms;
        } else {
            app->time_offset += 1800000;
            app->view.now = ms + 1800000;
            app->view.tick_pet();
        }
    } else if (command == "idle_test" && a >= 0 && a <= 60) {
        if (a == 0)
            app->view.preview_saver();
        else {
            app->time_offset += uint64_t(a) * 60000;
            app->view.now += uint64_t(a) * 60000;
            app->view.tick_pet();
        }
    } else if (command == "inject" && a >= 0 && a < int(category_count) + 4) {
        app->inject(a, b);
    }
    app->handle_requests();
    app->view.update_tag_watch();
    app->view.update_display();
    app->render(frame);
    return frame;
}
} // namespace hound_lab
