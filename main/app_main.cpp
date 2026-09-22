// SPDX-License-Identifier: Apache-2.0
#include "board.hpp"
#include "bootloader_random.h"
#include "esp_heap_caps.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "radio.hpp"
#include "signatures_generated.h"
#include "sniffer/core.hpp"
#include "storage.hpp"
#include "ui.hpp"
#include <array>
#include <cstdio>
#include <cstring>
namespace {
using namespace sniffer;
static storage::State state;
static Engine engine(production_rules);
static MealGuard meals;
static ui::View view(state.settings, state.pet);
static std::array<uint8_t, 32> session_key;
DMA_ATTR static uint16_t tile[ui::View::max_tile_pixels];
static bool radio_started{}, calibrating{}, was_touch{}, was_button{};
static board::Point raw_points[3];

static uint64_t last_save{}, button_at{}, demo_at{}, last_touch{}, last_diag{};
static Pet real_pet;
static const Rule demo_flipper{"demo.flipper",
                               Category::FLIPPER,
                               Kind::Protocol,
                               "synthetic",
                               85,
                               100,
                               false,
                               "Synthetic Flipper",
                               "DEMO: synthetic evidence only"};
static const Rule demo_pineapple{"demo.pineapple",
                                 Category::PINEAPPLE,
                                 Kind::Protocol,
                                 "synthetic",
                                 65,
                                 79,
                                 false,
                                 "Synthetic Pineapple",
                                 "DEMO: synthetic evidence only"};
void save() {
    if (view.demo)
        return;
    state.recent = meals.snapshot(view.now);
    state.battery = view.battery_cal;
    if (!storage::save(state))
        std::snprintf(view.notice.data(), view.notice.size(), "SAVE QUEUE FULL");
    last_save = view.now;
}
void begin_radio() {
    if (view.demo || view.paused)
        return;
    if (!radio_started) {
        radio_started = radio::start(state.settings.region);
        if (!radio_started)
            std::snprintf(view.notice.data(), view.notice.size(), "RADIO INIT FAILED");
    }
    radio::pause(!radio_started);
}
void diagnostics() {
    char json[256];
    std::snprintf(json, sizeof(json),
                  "{\"firmware\":\"0.1.0-dev\",\"reset_reason\":%lu,\"heap\":%lu,\"min_heap\":%lu,"
                  "\"dropped\":%lu,\"battery_adc\":%d}\n",
                  static_cast<unsigned long>(state.reset_reason),
                  static_cast<unsigned long>(view.heap), static_cast<unsigned long>(view.min_heap),
                  static_cast<unsigned long>(view.dropped), view.battery_adc);
    storage::diagnostics(json);
}
void app_task(void *) {
    esp_task_wdt_add(nullptr);
    for (;;) {
        esp_task_wdt_reset();
        view.now = esp_timer_get_time() / 1000;
        if (!view.demo)
            state.pet.tick(view.now);
        radio::Raw raw{};
        // Bounded work per UI frame prevents a radio flood from starving touch/rendering.
        for (int budget = 0; budget < 16 && radio::receive(raw); ++budget) {
            if (view.demo || calibrating || view.paused)
                continue;
            Observation o{};
            o.ms = raw.ms;
            o.rssi = raw.rssi;
            o.channel = raw.channel;
            o.address = raw.address;
            o.address_type = raw.address_type;
            auto bytes = std::span(raw.bytes).first(raw.size);
            bool valid = raw.radio == Radio::Wifi ? parse_wifi(bytes, o) : parse_ble(bytes, o);
            if (!valid) {
                ++view.malformed;
                if (raw.radio == Radio::Wifi)
                    ++view.wifi_malformed;
                else
                    ++view.ble_malformed;
                continue;
            }
            if (raw.radio == Radio::Wifi)
                ++view.wifi_accepted;
            else
                ++view.ble_accepted;
            view.drones.ingest(o);
            std::array<Detection, 4> found{};
            auto count = engine.ingest(o, state.settings, found);
            for (size_t i = 0; i < count; ++i) {
                auto &d = found[i];
                d.unix_seconds = view.utc_now();
                std::array<char, 21> token{};
                if (!private_token(session_key, d, token))
                    continue;
                bool category_allowed =
                    d.category != Category::IBEACON || state.settings.ibeacon_alerts;
                if (category_allowed)
                    d.meal = meals.award(meal_hash(state.meal_key, d), view.now, d.category,
                                         d.score, state.settings.low_feeding);
                if (d.seen_count == 1)
                    ++state.pet.unique;
                unsigned old_level = state.pet.level();
                state.pet.feed(d, state.settings.low_feeding);
                view.event(d);
                if (d.meal)
                    save();
                storage::append(d, token.data(), state.settings.research, state.settings.region);
                if (state.settings.sound && should_alert(state.settings, d))
                    storage::sound(state.pet.level() > old_level ? 2 : d.meal ? 1 : 0);
            }
        }
        board::Point raw_touch{};
        bool pressed = board::touch(raw_touch);
        if (pressed) {
            view.touch_x = raw_touch.x;
            view.touch_y = raw_touch.y;
        }
        if (pressed && !was_touch && view.now - last_touch > 150) {
            last_touch = view.now;
            if (calibrating) {
                raw_points[view.calibration_step] = raw_touch;
                ++view.calibration_step;
                if (view.calibration_step == 3) {
                    board::Point targets[3];
                    for (int i = 0; i < 3; ++i) {
                        auto p = display::to_portrait(display::target(i, state.settings.portrait),
                                                      state.settings.portrait);
                        targets[i] = {p.x, p.y};
                    }
                    if (board::calibrate(raw_points, targets, state.calibration)) {
                        calibrating = false;
                        view.screen =
                            state.settings.onboarded ? ui::Screen::Home : ui::Screen::Welcome;
                        save();
                        if (state.settings.onboarded)
                            begin_radio();
                    } else
                        view.calibration_step = 0;
                }
            } else if (state.calibration.valid) {
                auto point = board::map(raw_touch, state.calibration);
                auto rotated = display::from_portrait({point.x, point.y}, state.settings.portrait);
                view.tap(rotated.x, rotated.y);
            } else if (view.screen == ui::Screen::Welcome) {
                // Before calibration, BOOT chooses the conservative US channel plan.
                // A tap starts calibration; region remains US until Settings is usable.
                state.settings.region = 0;
                view.requests |= ui::Calibrate;
                view.screen = ui::Screen::Calibration;
            }
        }
        was_touch = pressed;
        bool button = board::button();
        if (button && !was_button)
            button_at = view.now;
        if (!button && was_button) {
            if (view.now - button_at >= 1000) {
                state.settings.sound = false;
                save();
            } else if (view.screen == ui::Screen::Welcome) {
                view.requests |= ui::Calibrate;
                view.screen = ui::Screen::Calibration;
            } else
                view.next();
        }
        was_button = button;
        uint32_t request = view.requests;
        view.requests = 0;
        if (request & ui::Calibrate) {
            radio::pause(true);
            calibrating = true;
            view.calibration_step = 0;
        }
        if (request & ui::Pause) {
            if (view.paused)
                radio::pause(true);
            else if (!calibrating)
                begin_radio();
        }
        if (request & ui::ClearLogs) {
            if (!view.demo) {
                if (storage::clear_logs())
                    std::snprintf(view.notice.data(), view.notice.size(), "LOG DELETE QUEUED");
            }
        }
        if (request & ui::Rotate)
            board::orientation(state.settings.portrait);
        if (request & ui::Start)
            begin_radio();
        if (request & ui::Save) {
            save();
            board::brightness(state.settings.brightness);
            radio::metrics.region = state.settings.region;
        }
        if (request & ui::Export) {
            if (!view.demo && storage::export_history())
                std::snprintf(view.notice.data(), view.notice.size(), "HISTORY EXPORT QUEUED");
        }
        if (request & ui::Eject) {
            if (!view.demo) {
                save();
                storage::eject();
            }
        }
        if (request & ui::DiagnosticCopy)
            diagnostics();
        if (request & ui::Reset) {
            if (!view.demo) {
                state.pet = Pet{};
                save();
            }
        }
        if (request & ui::Demo) {
            if (view.demo) {
                real_pet = state.pet;
                radio::pause(true);
            } else {
                state.pet = real_pet;
                begin_radio();
            }
            view.recent_count = 0;
            view.alert_until = 0;
            view.meal_until = 0;
        }
        if (view.demo && !view.paused && view.now - demo_at >= 8000) {
            Detection d{};
            d.demo = true;
            d.meal = true;
            d.category = (view.now / 8000) % 2 ? Category::FLIPPER : Category::PINEAPPLE;
            d.rules[0] = d.category == Category::FLIPPER ? &demo_flipper : &demo_pineapple;
            d.rule_count = 1;
            d.score = d.rules[0]->score;
            d.seen_count = 1;
            d.rssi_min = -70;
            d.rssi_max = -60;
            view.event(d);
            demo_at = view.now;
        }
        if (view.now - last_save >= 60000)
            save();
        view.heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        view.min_heap = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
        view.wifi_count = radio::metrics.wifi_seen;
        view.ble_count = radio::metrics.ble_seen;
        view.dropped = radio::metrics.dropped + storage::health.dropped;
        view.queue_high = radio::metrics.queue_high;
        view.ble = radio::metrics.ble_window;
        view.channel = radio::metrics.channel;
        view.scanning = radio::metrics.ready && !radio::metrics.paused;
        view.sd = storage::health.mounted && !storage::health.ejected;
        view.sd_error = storage::health.failed;
        view.writes = storage::health.writes;
        view.boot_reason = state.reset_reason;
        view.stack_free = uxTaskGetStackHighWaterMark(nullptr);
        view.export_count = storage::health.exports;
        view.storage_stack = storage::health.stack_free;
        view.radio_stack = radio::metrics.stack_free;
        view.ble_stack = radio::metrics.ble_stack;
        view.oversized = radio::metrics.oversized;
        view.radio_errors = radio::metrics.errors;
        view.sd_read_only = storage::health.read_only;
        view.sd_free = storage::health.free_bytes;
        view.sd_total = storage::health.total_bytes;
        if (view.sd_read_only)
            std::snprintf(view.notice.data(), view.notice.size(),
                          "SD READ ONLY - CHECK LOGS ON COMPUTER");
        static uint32_t last_clears = 0;
        if (storage::health.clears != last_clears) {
            last_clears = storage::health.clears;
            view.recent_count = 0;
            view.log_page = 0;
            std::snprintf(view.notice.data(), view.notice.size(), "SD LOGS DELETED");
        }

        if (view.now - last_diag >= 2000) {
            view.battery_adc = board::battery_raw();
            view.battery_millivolts =
                view.battery_cal.enabled ? battery_mv(view.battery_cal, view.battery_adc) : -1;
            view.battery_level = battery_percent(view.battery_millivolts);
            static unsigned critical = 0;
            static bool sleeping = false;
            static uint64_t low_sound = 0;
            critical =
                view.battery_millivolts >= 0 && view.battery_millivolts <= 3300 ? critical + 1 : 0;
            if (!view.demo && view.battery_millivolts >= 0 && view.battery_millivolts < 3500)
                std::snprintf(view.notice.data(), view.notice.size(),
                              "LOW BATTERY - CONNECT POWER");
            if (!view.demo && state.settings.sound && view.battery_millivolts >= 0 &&
                view.battery_millivolts < 3500 && (!low_sound || view.now - low_sound > 60000)) {
                storage::sound(3);
                low_sound = view.now;
            }
            if (!view.demo && critical >= 5 && !sleeping) {
                view.paused = true;
                radio::pause(true);
                save();
                sleeping = storage::sleep(state);
                std::snprintf(view.notice.data(), view.notice.size(),
                              "CRITICAL BATTERY - SAVING AND SLEEPING");
            }
            if (storage::health.sleep_failed)
                std::snprintf(view.notice.data(), view.notice.size(),
                              "SD FLUSH FAILED - CONNECT POWER");
            last_diag = view.now;
        }
        if (view.heap < 80000)
            std::snprintf(view.notice.data(), view.notice.size(), "LOW HEAP - SEE DIAGNOSTICS");
        bool alert = view.alert_until > view.now && !view.paused;
        bool pulse = (view.now / 250) % 2 == 0;
        board::led(alert && view.alert_detection.score >= 80 && pulse,
                   (alert && view.alert_detection.score < 80 && pulse) ||
                       view.meal_until > view.now,
                   !alert && view.scanning && (view.now / 1000) % 2 == 0);
        for (int y = 0; y < view.height(); y += view.tile_rows()) {
            view.render(y, std::span(tile).first(view.width() * view.tile_rows()));
            board::blit(y, view.tile_rows(),
                        std::span(tile).first(view.width() * view.tile_rows()));
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
} // namespace
extern "C" void app_main() {
    if (!board::init()) {
        for (;;) {
            board::led(true, false, false);
            vTaskDelay(pdMS_TO_TICKS(300));
            board::led(false, false, false);
            vTaskDelay(pdMS_TO_TICKS(900));
        }
    }
    bootloader_random_enable();
    if (!storage::load(state)) {
        bootloader_random_disable();
        view.screen = ui::Screen::Diagnostics;
        std::snprintf(view.notice.data(), view.notice.size(), "NVS ERROR - SCAN DISABLED");
        // A failed state store must not silently reset privacy settings or resume scanning.
        for (;;) {
            for (int y = 0; y < view.height(); y += view.tile_rows()) {
                view.render(y, std::span(tile).first(view.width() * view.tile_rows()));
                board::blit(y, view.tile_rows(),
                            std::span(tile).first(view.width() * view.tile_rows()));
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    view.battery_cal = state.battery;
    view.known_level = state.pet.level();
    board::orientation(state.settings.portrait);
    esp_fill_random(session_key.data(), session_key.size());
    char session[33];
    std::array<uint8_t, 16> session_random{};
    esp_fill_random(session_random.data(), session_random.size());
    for (size_t i = 0; i < 16; ++i)
        std::snprintf(session + i * 2, 3, "%02x", session_random[i]);
    bootloader_random_disable();
    meals.restore(state.recent);
    board::brightness(state.settings.brightness);
    storage::start(session);
    view.screen = state.settings.onboarded ? ui::Screen::Home : ui::Screen::Welcome;
    if (!state.calibration.valid) {
        calibrating = true;
        view.screen = ui::Screen::Calibration;
    }
#ifdef CONFIG_SNIFFER_DEMO
    view.demo = true;
    view.screen = ui::Screen::Home;
    real_pet = state.pet;
#endif
    if (state.settings.onboarded && !calibrating)
        begin_radio();
    xTaskCreatePinnedToCore(app_task, "pet_ui_detect", 8192, nullptr, 4, nullptr, 1);
}
