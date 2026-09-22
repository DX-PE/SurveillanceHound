// SPDX-License-Identifier: Apache-2.0
// Host-only interactive adapter. No board, radio, NVS or storage linkage.
#include "assets_generated.h"
#include "signatures_generated.h"
#include "ui.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

namespace {
using namespace sniffer;
constexpr const char *screens[] = {
    "Welcome",   "Calibration", "Choose pet", "Name pet",      "Privacy",    "Pet",
    "Log",       "Detectors",   "Settings",   "Diagnostics",   "Details",    "Research warning",
    "Reset pet", "Alert types", "Set time",   "Battery setup", "Clear logs", "Self test"};
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
    std::array<Rule, category_count> samples{};
    Simulator() {
        settings.onboarded = true;
        view.screen = ui::Screen::Home;
        view.demo = true;
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
    void inject(int type) {
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
        d.score = d.rules[0]->score;
        d.rule_count = 1;
        d.demo = true;
        d.meal = d.score >= 50;
        d.radio = type == 1 ? Radio::Wifi : Radio::Ble;
        d.first_ms = d.last_ms = view.now;
        d.seen_count = 1;
        d.rssi_min = -72;
        d.rssi_max = -61;
        d.rssi_sum = -66;
        if (!(settings.enabled_categories & (1U << static_cast<unsigned>(d.category)))) {
            std::snprintf(view.notice.data(), view.notice.size(), "CATEGORY IS DISABLED");
            return;
        }
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
            std::snprintf(view.notice.data(), view.notice.size(), "SIMULATOR: NO SD OR REAL LOGS");
        if (requests & ui::Eject)
            std::snprintf(view.notice.data(), view.notice.size(), "SIMULATOR: NO CARD TO EJECT");
        if (requests & ui::DiagnosticCopy)
            std::snprintf(view.notice.data(), view.notice.size(), "SIMULATOR: NO SD WRITES");
        if (requests & ui::Calibrate)
            view.calibration_step = 0;
        if (requests & ui::ClearLogs) {
            view.recent_count = 0;
            view.log_page = 0;
        }
        if (requests & ui::Reset)
            pet = Pet{};
    }
    void reply() {
        std::array<char, 80> escaped_name{};
        json_escape(settings.name.data(), escaped_name);
        std::cout << "{\"screen\":\"" << screens[static_cast<unsigned>(view.screen)]
                  << "\",\"pet\":\"" << assets::names[settings.character]
                  << "\",\"name\":" << escaped_name.data() << ",\"xp\":" << pet.xp
                  << ",\"mood\":" << unsigned(pet.mood)
                  << ",\"fullness\":" << unsigned(pet.fullness) << ",\"width\":" << view.width()
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
        std::array<uint16_t, ui::View::max_tile_pixels> tile{};
        std::array<char, ui::View::max_tile_pixels * 2> bytes{};
        for (int y = 0; y < view.height(); y += view.tile_rows()) {
            view.render(y, std::span(tile).first(view.width() * view.tile_rows()));
            for (size_t i = 0; i < size_t(view.width() * view.tile_rows()); ++i) {
                bytes[2 * i] = static_cast<char>(tile[i] & 255);
                bytes[2 * i + 1] = static_cast<char>(tile[i] >> 8);
            }
            std::cout.write(bytes.data(), view.width() * view.tile_rows() * 2);
        }
        std::cout.flush();
    }
};
} // namespace
int main() {
    auto app = std::make_unique<Simulator>();
    std::string command;
    int a, b;
    uint64_t ms;
    while (std::cin >> command >> a >> b >> ms) {
        if (command == "reset" || command == "onboard") {
            app = std::make_unique<Simulator>();
            if (command == "onboard") {
                app->settings.onboarded = false;
                app->view.screen = ui::Screen::Welcome;
            }
        }
        app->view.now = ms;
        if (command == "tap" && a >= 0 && a < app->view.width() && b >= 0 &&
            b < app->view.height()) {
            if (app->view.screen == ui::Screen::Calibration) {
                if (++app->view.calibration_step == 3)
                    app->view.screen =
                        app->settings.onboarded ? ui::Screen::Home : ui::Screen::Welcome;
            } else
                app->view.tap(a, b);
        } else if (command == "screen" &&
                   (a == 2 || a == 5 || a == 6 || a == 7 || a == 8 || a == 9 || a == 13)) {
            app->view.screen = static_cast<ui::Screen>(a);
        } else if (command == "rotate") {
            app->view.rotate();
        } else if (command == "boot") {
            app->view.next();
        } else if (command == "inject" && a >= 0 && a < int(category_count) + 4) {
            app->inject(a);
        }
        app->handle_requests();
        app->reply();
    }
}
