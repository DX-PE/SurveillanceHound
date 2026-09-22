// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "sniffer/core.hpp"
#include "sniffer/display.hpp"
namespace sniffer::ui {
enum class Screen {
    Welcome,
    Calibration,
    Choose,
    Name,
    Privacy,
    Home,
    Log,
    Detectors,
    Settings,
    Diagnostics,
    Details,
    ResearchWarning,
    ResetWarning,
    Alerts,
    Clock,
    Battery,
    ClearWarning,
    SelfTest
};
enum Request : uint32_t {
    Save = 1,
    Start = 2,
    Export = 4,
    Eject = 8,
    Demo = 16,
    Calibrate = 32,
    DiagnosticCopy = 64,
    Reset = 128,
    Rotate = 256,
    Pause = 512,
    ClearLogs = 1024,
    Sleep = 2048
};
enum class SnackPhase { None, Turn, Approach, Chew, Happy };
struct SnackPose {
    SnackPhase phase{SnackPhase::None};
    int frame{}, approach{}, bites{};
};
class View {
  public:
    View(Settings &s, Pet &p) : settings(s), pet(p) {}
    Settings &settings;
    Pet &pet;
    Screen screen{Screen::Welcome};
    uint32_t requests{};
    uint64_t now{}, alert_until{}, meal_until{}, happy_until{};
    Category meal_category{Category::FLIPPER};
    Detection alert_detection{}, detail_detection{};
    DroneTrack detail_drone{};
    bool detail_valid{};
    int detail_page{};
    bool demo{}, sd{}, sd_error{}, ble{}, scanning{}, paused{}, sd_read_only{};
    uint32_t wifi_accepted{}, ble_accepted{}, wifi_malformed{}, ble_malformed{}, radio_stack{},
        storage_stack{}, ble_stack{}, oversized{}, radio_errors{};
    uint64_t sd_free{}, sd_total{}, clock_seconds{}, clock_set_ms{}, level_until{};
    BatteryCalibration battery_cal{};
    int battery_millivolts{-1}, battery_level{-1}, battery_point{}, diagnostic_page{}, log_page{};
    std::array<char, 13> clock_input{};
    unsigned known_level{1}, self_test_taps{};
    DroneCache drones{};
    uint32_t heap{}, min_heap{}, wifi_count{}, ble_count{}, malformed{}, dropped{}, queue_high{},
        writes{}, boot_reason{}, stack_free{}, export_count{};
    int battery_adc{-1}, touch_x{}, touch_y{}, calibration_step{}, channel{}, detector_page{},
        settings_page{}, alert_page{};
    unsigned selected{};
    std::array<Detection, 32> recent{};
    size_t recent_count{};
    std::array<uint32_t, 3> counts{};
    std::array<char, 64> notice{};
    int width() const {
        return display::width(settings.portrait);
    }
    int height() const {
        return display::height(settings.portrait);
    }
    int tile_rows() const {
        return settings.portrait ? 16 : 8;
    }
    static constexpr int max_tile_pixels = 320 * 16;
    static constexpr uint64_t meal_duration = 7000;
    SnackPose snack_pose() const;
    uint64_t utc_now() const {
        return clock_seconds && now >= clock_set_ms ? clock_seconds + (now - clock_set_ms) / 1000
                                                    : 0;
    }
    void rotate();
    void event(const Detection &);
    void tap(int x, int y);
    void next();
    void render(int tile_y, std::span<uint16_t> pixels);
};
} // namespace sniffer::ui
