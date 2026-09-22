// SPDX-License-Identifier: Apache-2.0
#include "ui.hpp"
#include "assets_generated.h"
#include "signatures_generated.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
namespace sniffer::ui {
namespace {
constexpr uint16_t bg = 0x0863, panel = 0x18C6, ink = 0xF7BF, muted = 0xA4F7, mint = 0x3F1C,
                   amber = 0xFE6B, pink = 0xFBB6, grid = 0x312B;
// Original compact 5x7 display alphabet, stored as row masks.
constexpr uint8_t font[][7] = {
    {14, 17, 17, 31, 17, 17, 17}, {30, 17, 17, 30, 17, 17, 30}, {14, 17, 16, 16, 16, 17, 14},
    {30, 17, 17, 17, 17, 17, 30}, {31, 16, 16, 30, 16, 16, 31}, {31, 16, 16, 30, 16, 16, 16},
    {14, 17, 16, 23, 17, 17, 15}, {17, 17, 17, 31, 17, 17, 17}, {31, 4, 4, 4, 4, 4, 31},
    {7, 2, 2, 2, 18, 18, 12},     {17, 18, 20, 24, 20, 18, 17}, {16, 16, 16, 16, 16, 16, 31},
    {17, 27, 21, 21, 17, 17, 17}, {17, 25, 25, 21, 19, 19, 17}, {14, 17, 17, 17, 17, 17, 14},
    {30, 17, 17, 30, 16, 16, 16}, {14, 17, 17, 17, 21, 18, 13}, {30, 17, 17, 30, 20, 18, 17},
    {15, 16, 16, 14, 1, 1, 30},   {31, 4, 4, 4, 4, 4, 4},       {17, 17, 17, 17, 17, 17, 14},
    {17, 17, 17, 17, 17, 10, 4},  {17, 17, 17, 21, 21, 27, 17}, {17, 17, 10, 4, 10, 17, 17},
    {17, 17, 10, 4, 4, 4, 4},     {31, 1, 2, 4, 8, 16, 31},     {14, 17, 19, 21, 25, 17, 14},
    {4, 12, 4, 4, 4, 4, 14},      {14, 17, 1, 2, 4, 8, 31},     {30, 1, 1, 14, 1, 1, 30},
    {2, 6, 10, 18, 31, 2, 2},     {31, 16, 16, 30, 1, 1, 30},   {14, 16, 16, 30, 17, 17, 14},
    {31, 1, 2, 4, 8, 8, 8},       {14, 17, 17, 14, 17, 17, 14}, {14, 17, 17, 15, 1, 1, 14},
    {0, 0, 0, 31, 0, 0, 0},       {0, 4, 4, 0, 4, 4, 0},        {0, 0, 0, 0, 0, 4, 4},
    {0, 1, 2, 4, 8, 16, 0},       {14, 17, 1, 2, 4, 0, 4},      {4, 4, 4, 4, 4, 0, 4},
    {0, 0, 0, 0, 0, 0, 0}};
int glyph(char c) {
    c = char(std::toupper(static_cast<unsigned char>(c)));
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= '0' && c <= '9')
        return c - '0' + 26;
    if (c == '-')
        return 36;
    if (c == ':')
        return 37;
    if (c == '.')
        return 38;
    if (c == '/')
        return 39;
    if (c == '?')
        return 40;
    if (c == '!')
        return 41;
    return 42;
}
class Canvas {
  public:
    int tile, width, height, rows;
    std::span<uint16_t> pixels;
    void rect(int x, int y, int w, int h, uint16_t c) {
        int bottom = std::min({y + h, tile + rows, height}), right = std::min(width, x + w);
        for (int yy = std::max(y, tile); yy < bottom; ++yy)
            for (int xx = std::max(0, x); xx < right; ++xx)
                pixels[(yy - tile) * width + xx] = c;
    }
    void text(int x, int y, std::string_view text, uint16_t color = ink, int scale = 2) {
        for (char ch : text) {
            const auto &rows = font[glyph(ch)];
            for (int yy = 0; yy < 7; ++yy)
                for (int xx = 0; xx < 5; ++xx)
                    if (rows[yy] & (1 << (4 - xx)))
                        rect(x + xx * scale, y + yy * scale, scale, scale, color);
            x += 6 * scale;
        }
    }
    void wrap(int x, int y, std::string_view text, int width = 24, uint16_t color = muted,
              int scale = 2) {
        while (!text.empty()) {
            size_t n = std::min<size_t>(width, text.size());
            if (n < text.size()) {
                auto space = text.substr(0, n).find_last_of(' ');
                if (space != text.npos && space)
                    n = space;
            }
            this->text(x, y, text.substr(0, n), color, scale);
            text.remove_prefix(n);
            while (text.starts_with(' '))
                text.remove_prefix(1);
            y += 11 * scale;
        }
    }
    void box(int x, int y, int w, int h, uint16_t edge = grid) {
        rect(x, y, w, h, edge);
        rect(x + 1, y + 1, w - 2, h - 2, panel);
    }
    void button(int x, int y, int w, std::string_view label, bool active = false, int h = 40) {
        box(x, y, w, h, active ? mint : grid);
        int scale = int(label.size()) * 12 + 20 <= w ? 2 : 1;
        text(x + (w - int(label.size()) * 6 * scale) / 2, y + (h - 7 * scale) / 2, label,
             active ? mint : ink, scale);
    }
    void line(int x0, int y0, int x1, int y1, uint16_t color) {
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1, error = dx + dy;
        for (;;) {
            rect(x0, y0, 1, 1, color);
            if (x0 == x1 && y0 == y1)
                break;
            int e = 2 * error;
            if (e >= dy) {
                error += dy;
                x0 += sx;
            }
            if (e <= dx) {
                error += dx;
                y0 += sy;
            }
        }
    }
    void sprite(int x, int y, int character, int frame, int scale = 2) {
        const auto &f = assets::frames[std::clamp(character, 0, 5)][frame % assets::frame_count];
        size_t at = 0;
        for (size_t i = f.offset; i < f.offset + f.length; i += 2) {
            uint8_t run = assets::pixels[i], index = assets::pixels[i + 1];
            for (unsigned j = 0; j < run; ++j, ++at)
                if (index)
                    rect(x + int(at % 64) * scale, y + int(at / 64) * scale, scale, scale,
                         assets::palettes[std::clamp(character, 0, 5)][index]);
        }
    }
    void food(int x, int y, Category category, int bites = 0) {
        if (bites >= 4)
            return;
        // Cut away the mouth-facing side with each bite; no background paint-over.
        auto part = [&](int dx, int dy, int w, int h, uint16_t color) {
            int left = std::max(dx, bites * 10);
            if (left < dx + w)
                rect(x + left - bites * 8, y + dy, dx + w - left, h, color);
        };
        if (category == Category::FLIPPER) {
            // A dolphin-shaped teal biscuit: tail flukes, dorsal fin and long snout.
            part(0, 12, 8, 6, mint);
            part(2, 8, 4, 14, mint);
            part(8, 9, 22, 14, mint);
            part(13, 6, 16, 4, mint);
            part(15, 0, 5, 10, mint);
            part(20, 4, 3, 6, mint);
            part(28, 10, 8, 8, mint);
            part(33, 14, 7, 4, mint);
            part(15, 19, 5, 9, mint);
            part(19, 19, 4, 5, mint);
            part(27, 11, 3, 3, bg);
            part(10, 18, 20, 3, ink);
        } else if (category == Category::PINEAPPLE) {
            part(5, 11, 29, 20, amber);
            part(9, 8, 21, 28, amber);
            part(11, 13, 17, 2, ink);
            for (int row = 0; row < 3; ++row)
                for (int col = 0; col < 3; ++col)
                    part(11 + col * 7 + (row % 2) * 2, 18 + row * 6, 3, 3, 0xAB44);
            part(16, 0, 5, 12, mint);
            part(7, 3, 8, 5, mint);
            part(23, 3, 9, 5, mint);
            part(11, 7, 17, 5, mint);
        } else if (category == Category::FLOCK || category == Category::ALPR) {
            part(0, 8, 40, 24, category == Category::FLOCK ? ink : amber);
            part(4, 12, 32, 16, bg);
            part(7, 15, 4, 9, mint);
            part(14, 15, 10, 3, mint);
            part(14, 21, 10, 3, mint);
            part(28, 15, 4, 9, mint);
        } else if (category == Category::AXON || category == Category::CAMERA) {
            part(4, 8, 32, 24, category == Category::AXON ? pink : mint);
            part(8, 4, 12, 5, ink);
            part(16, 12, 14, 14, bg);
            part(19, 15, 8, 8, ink);
            part(7, 12, 4, 4, amber);
        } else if (category == Category::GLASSES) {
            part(0, 12, 17, 15, mint);
            part(23, 12, 17, 15, mint);
            part(15, 16, 10, 4, mint);
            part(4, 16, 9, 7, bg);
            part(27, 16, 9, 7, bg);
            part(0, 8, 4, 8, mint);
            part(36, 8, 4, 8, mint);
        } else if (category == Category::SKIMMER) {
            part(0, 5, 40, 28, amber);
            part(0, 11, 40, 5, bg);
            part(6, 22, 8, 6, ink);
            part(22, 24, 12, 3, pink);
        } else if (category == Category::RAVEN) {
            part(12, 1, 16, 23, pink);
            part(16, 4, 8, 15, bg);
            part(6, 16, 4, 12, ink);
            part(30, 16, 4, 12, ink);
            part(10, 26, 20, 4, ink);
            part(18, 29, 4, 7, ink);
        } else if (category == Category::AIRTAG) {
            part(8, 2, 24, 32, ink);
            part(3, 8, 34, 20, ink);
            part(12, 10, 16, 16, muted);
            part(16, 14, 8, 8, bg);
        } else if (category == Category::TILE) {
            part(3, 3, 34, 32, mint);
            part(7, 7, 6, 6, bg);
            part(19, 12, 4, 16, bg);
            part(14, 12, 14, 4, bg);
        } else if (category == Category::SAMSUNG_TAG) {
            for (int i = 0; i < 18; ++i)
                part(19 - i, 17 - std::abs(i - 9), 2 * i + 2, 2 * (9 - std::abs(i - 9)) + 8, pink);
            part(17, 5, 6, 6, bg);
            part(14, 18, 12, 5, ink);
        } else if (category == Category::GOOGLE_TAG) {
            part(7, 0, 26, 20, mint);
            part(3, 6, 34, 12, mint);
            part(12, 18, 16, 7, mint);
            part(17, 25, 6, 9, mint);
            part(15, 6, 10, 10, bg);
        } else if (category == Category::DRONE) {
            part(12, 13, 16, 12, amber);
            part(3, 8, 7, 20, muted);
            part(30, 8, 7, 20, muted);
            part(0, 5, 15, 4, mint);
            part(25, 5, 15, 4, mint);
            part(0, 28, 15, 4, mint);
            part(25, 28, 15, 4, mint);
            part(8, 17, 24, 4, ink);
        } else if (category == Category::RING) {
            part(10, 0, 20, 36, ink);
            part(14, 4, 12, 12, bg);
            part(17, 7, 6, 6, mint);
            part(15, 23, 10, 9, mint);
        } else if (category == Category::IBEACON) {
            part(9, 12, 23, 23, pink);
            part(14, 7, 13, 5, mint);
            part(19, 1, 4, 9, mint);
            part(14, 18, 4, 4, ink);
            part(26, 23, 3, 3, ink);
        } else if (category == Category::PWNAGOTCHI) {
            part(7, 3, 26, 29, ink);
            part(3, 10, 34, 20, ink);
            part(3, 29, 7, 7, ink);
            part(16, 29, 8, 7, ink);
            part(30, 29, 7, 7, ink);
            part(11, 12, 6, 8, bg);
            part(25, 12, 6, 8, bg);
        } else if (category == Category::DEAUTH) {
            part(18, 0, 14, 8, amber);
            part(12, 8, 14, 8, amber);
            part(6, 16, 28, 6, amber);
            part(20, 22, 10, 7, amber);
            part(14, 29, 10, 7, amber);
        } else if (category == Category::EVIL_TWIN) {
            part(0, 5, 24, 25, mint);
            part(16, 10, 24, 25, pink);
            part(4, 11, 5, 5, bg);
            part(13, 11, 5, 5, bg);
            part(21, 17, 5, 5, bg);
            part(31, 17, 5, 5, bg);
        } else {
            part(4, 8, 31, 24, amber);
            part(8, 4, 23, 32, amber);
            part(10, 12, 5, 4, 0xAB44);
            part(24, 17, 4, 4, 0xAB44);
            part(15, 25, 5, 4, 0xAB44);
        }
    }
};
struct Rect {
    int x, y, w, h;
    bool has(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};
struct Layout {
    bool portrait;
    int w, h;
    explicit Layout(bool p) : portrait(p), w(display::width(p)), h(display::height(p)) {}
    Rect sniff() const {
        return portrait ? Rect{16, 398, 288, 32} : Rect{306, 226, 162, 30};
    }
    Rect row(int i) const {
        return {12, (portrait ? 80 : 66) + i * (portrait ? 55 : 34), w - 24, portrait ? 48 : 30};
    }
    Rect more() const {
        return {12, h - 82, w - 24, 32};
    }
    Rect choice(int i) const {
        int cols = portrait ? 2 : 3, cw = (w - 24) / cols;
        return {12 + (i % cols) * cw, (portrait ? 88 : 70) + (i / cols) * (portrait ? 106 : 98),
                cw - 8, portrait ? 98 : 90};
    }
    Rect key(int i) const {
        int cols = portrait ? 6 : 10, step = portrait ? 48 : 44;
        return {(portrait ? 16 : 20) + (i % cols) * step,
                (portrait ? 128 : 110) + (i / cols) * (portrait ? 48 : 42), step - 4,
                portrait ? 44 : 38};
    }
    Rect confirm(bool right) const {
        return {right ? w / 2 + 4 : 12, h - 82, w / 2 - 16, 32};
    }
    Rect log(int i) const {
        return detector(i);
    }
    Rect detector(int i) const {
        return {12, (portrait ? 80 : 64) + i * (portrait ? 68 : 43), w - 24, portrait ? 60 : 39};
    }
};
void button(Canvas &c, Rect r, std::string_view label, bool active = false) {
    c.button(r.x, r.y, r.w, label, active, r.h);
}
const char *titles[] = {"HELLO, HOUND",    "CALIBRATE TOUCH",  "PICK YOUR PARTNER",
                        "NAME YOUR HOUND", "YOUR PRIVACY",     "SURVEILLANCE HOUND",
                        "FIELD NOTES",     "DETECTORS",        "SETTINGS",
                        "DIAGNOSTICS",     "EVIDENCE DETAILS", "RESEARCH LOGGING",
                        "RESET PROGRESS",  "ALERT TYPES",      "SET UTC TIME",
                        "BATTERY SETUP",   "CLEAR LOGS",       "SELF TEST"};
} // namespace
void View::event(const Detection &d) {
    if (paused)
        return;
    if (!d.demo && pet.level() > known_level)
        level_until = now + 11000;
    known_level = pet.level();
    for (size_t i = recent.size() - 1; i > 0; --i)
        recent[i] = recent[i - 1];
    recent[0] = d;
    recent_count = std::min(recent_count + 1, recent.size());
    if (!d.demo)
        ++counts[d.score >= 80 ? 0 : d.score >= 50 ? 1 : 2];
    if (should_alert(settings, d)) {
        alert_detection = d;
        alert_until = now + 5000;
    }
    if (d.meal) {
        meal_category = d.category;
        meal_until = now + meal_duration;
    }
}
SnackPose View::snack_pose() const {
    if (now >= meal_until)
        return {};
    if (settings.reduced_animation)
        return {SnackPhase::Chew, assets::side_eat, 100, 0};
    uint64_t elapsed = now - (meal_until - meal_duration);
    if (elapsed < 450)
        return {SnackPhase::Turn, 11 + int(elapsed / 225), 0, 0};
    if (elapsed < 2300)
        return {SnackPhase::Approach, assets::side_walk + int((elapsed - 450) / 140) % 4,
                int((elapsed - 450) * 100 / 1850), 0};
    if (elapsed < 5700)
        return {SnackPhase::Chew, assets::side_eat + int((elapsed - 2300) / 160) % 4, 100,
                std::min(4, int((elapsed - 2300) / 800))};
    return {SnackPhase::Happy, 22 + int((elapsed - 5700) / 180) % 3, 100, 4};
}
void View::rotate() {
    if (screen == Screen::Calibration || settings.rotation_locked)
        return;
    settings.portrait = !settings.portrait;
    requests |= Save | Rotate;
}
void View::next() {
    screen = screen == Screen::Home        ? Screen::Log
             : screen == Screen::Log       ? Screen::Detectors
             : screen == Screen::Detectors ? Screen::Settings
                                           : Screen::Home;
}
void View::tap(int x, int y) {
    const Layout l(settings.portrait);
    if (x < 0 || y < 0 || x >= l.w || y >= l.h || screen == Screen::Calibration)
        return;
    if (y < 26 && x >= l.w - 60) {
        rotate();
        return;
    }
    if (screen == Screen::Home && l.sniff().has(x, y)) {
        paused = !paused;
        requests |= Pause;
        alert_until = meal_until = happy_until = 0;
        scanning = !paused;
        notice[0] = 0;
        return;
    }
    if (screen == Screen::Clock) {
        size_t n = std::strlen(clock_input.data());
        for (int i = 0; i < 10; ++i) {
            Rect r{16 + (i % 5) * (l.w - 32) / 5, 120 + (i / 5) * 48, (l.w - 32) / 5 - 4, 44};
            if (r.has(x, y) && n < 12) {
                clock_input[n] = '0' + i;
                clock_input[n + 1] = 0;
                return;
            }
        }
        if (Rect{16, 80, l.w - 32, 32}.has(x, y) && n)
            clock_input[n - 1] = 0;
        if (l.confirm(false).has(x, y))
            screen = Screen::Settings;
        if (l.confirm(true).has(x, y)) {
            uint64_t seconds;
            if (parse_utc(clock_input.data(), seconds)) {
                clock_seconds = seconds;
                clock_set_ms = now;
                screen = Screen::Settings;
                notice[0] = 0;
            } else
                std::snprintf(notice.data(), notice.size(), "USE YYYYMMDDHHMM / VALID UTC DATE");
        }
        return;
    }
    if (screen == Screen::Battery) {
        if (l.row(0).has(x, y))
            battery_point = (battery_point + 1) % 3;
        if (l.row(1).has(x, y)) {
            int delta = x < l.w / 2 ? -50 : 50;
            auto &v = battery_cal.millivolts[battery_point];
            v = std::clamp(int(v) + delta, 2500, 4500);
            battery_cal.enabled = false;
            requests |= Save;
        }
        if (l.row(2).has(x, y) && battery_adc >= 0 && battery_adc <= 4095) {
            battery_cal.adc[battery_point] = battery_adc;
            battery_cal.enabled = false;
            requests |= Save;
        }
        if (l.row(3).has(x, y)) {
            battery_cal.enabled = !battery_cal.enabled && valid_battery(battery_cal);
            requests |= Save;
        }
        if (l.row(4).has(x, y)) {
            battery_cal = {};
            requests |= Save;
        }
        if (l.more().has(x, y))
            screen = Screen::Settings;
        return;
    }
    if (screen == Screen::SelfTest) {
        if (l.more().has(x, y))
            screen = Screen::Settings;
        else {
            ++self_test_taps;
            touch_x = x;
            touch_y = y;
        }
        return;
    }
    if (screen == Screen::ClearWarning) {
        if (l.confirm(false).has(x, y))
            screen = Screen::Settings;
        if (l.confirm(true).has(x, y)) {
            requests |= ClearLogs;
            screen = Screen::Settings;
        }
        return;
    }
    if (screen == Screen::Welcome) {
        if (l.confirm(false).has(x, y) || l.confirm(true).has(x, y)) {
            settings.region = x < l.w / 2 ? 0 : 1;
            requests |= Save;
            screen = Screen::Choose;
        }
        return;
    }
    if (screen == Screen::Choose) {
        for (int i = 0; i < 6; ++i)
            if (l.choice(i).has(x, y)) {
                settings.character = i;
                screen = Screen::Name;
                requests |= Save;
                return;
            }
        return;
    }
    if (screen == Screen::Name) {
        size_t n = std::strlen(settings.name.data());
        for (int i = 0; i < 30; ++i)
            if (l.key(i).has(x, y) && n < 12) {
                settings.name[n] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123"[i];
                settings.name[n + 1] = 0;
                return;
            }
        if (l.confirm(false).has(x, y) && n)
            settings.name[n - 1] = 0;
        if (l.confirm(true).has(x, y) && n) {
            requests |= Save;
            screen = settings.onboarded ? Screen::Home : Screen::Privacy;
        }
        return;
    }
    if (screen == Screen::Privacy) {
        if (l.confirm(false).has(x, y)) {
            settings.research = false;
            settings.onboarded = true;
            screen = Screen::Home;
            requests |= Save | Start;
        } else if (l.confirm(true).has(x, y))
            screen = Screen::ResearchWarning;
        return;
    }
    if (screen == Screen::ResearchWarning || screen == Screen::ResetWarning) {
        if (l.confirm(false).has(x, y))
            screen = settings.onboarded ? Screen::Settings : Screen::Privacy;
        else if (l.confirm(true).has(x, y)) {
            if (screen == Screen::ResetWarning) {
                requests |= Reset;
                screen = Screen::Settings;
            } else {
                settings.research = true;
                settings.onboarded = true;
                requests |= Save | Start;
                screen = Screen::Home;
            }
        }
        return;
    }
    if (screen == Screen::Details) {
        if (Rect{l.w - 108, 32, 96, 30}.has(x, y))
            detail_page = 1 - detail_page;
        if (l.more().has(x, y))
            screen = Screen::Log;
        return;
    }
    if (settings.onboarded && y >= l.h - 44) {
        constexpr Screen nav[] = {Screen::Home, Screen::Log, Screen::Detectors, Screen::Settings};
        screen = nav[x / (l.w / 4)];
        return;
    }
    if (screen == Screen::Home) {
        if (alert_until > now) {
            alert_until = 0;
            return;
        }
        if (y >= 64 && y < (l.portrait ? 308 : 254) && (l.portrait || x < 300)) {
            pet.stroke();
            happy_until = now + 2500;
        }
    } else if (screen == Screen::Log) {
        if (l.more().has(x, y)) {
            log_page = (log_page + 1) % std::max(1, int((recent_count + 3) / 4));
            return;
        }
        for (unsigned i = 0; i < 4 && size_t(log_page * 4 + i) < recent_count; ++i)
            if (l.log(i).has(x, y)) {
                selected = log_page * 4 + i;
                detail_detection = recent[selected];
                detail_drone = {};
                if (const auto *track =
                        drones.find(detail_detection.address, detail_detection.address_type, now))
                    detail_drone = *track;
                detail_valid = true;
                detail_page = 0;
                screen = Screen::Details;
            }
    } else if (screen == Screen::Detectors) {
        if (l.more().has(x, y))
            detector_page = (detector_page + 1) % 5;
        else
            for (int i = 0; i < 4; ++i)
                if (l.detector(i).has(x, y)) {
                    int index = detector_page * 4 + i;
                    if (index < int(category_count)) {
                        settings.enabled_categories ^= 1U << index;
                        requests |= Save;
                    }
                }
    } else if (screen == Screen::Alerts) {
        if (l.more().has(x, y))
            alert_page = (alert_page + 1) % 5;
        else
            for (int i = 0; i < 4; ++i)
                if (l.detector(i).has(x, y)) {
                    int index = alert_page * 4 + i;
                    if (index < int(category_count)) {
                        settings.alert_categories ^= 1U << index;
                        if (index == int(Category::IBEACON))
                            settings.ibeacon_alerts = settings.alert_categories & (1U << index);
                        if (alert_until > now && int(alert_detection.category) == index &&
                            !should_alert(settings, alert_detection))
                            alert_until = 0;
                        requests |= Save;
                    }
                }
    } else if (screen == Screen::Diagnostics) {
        if (l.more().has(x, y))
            diagnostic_page = (diagnostic_page + 1) % 3;
        if (Rect{l.w - 90, 35, 78, 24}.has(x, y))
            requests |= DiagnosticCopy;
    } else if (screen == Screen::Settings) {
        if (l.more().has(x, y)) {
            settings_page = (settings_page + 1) % 5;
            return;
        }
        int row = -1;
        for (int i = 0; i < 5; ++i)
            if (l.row(i).has(x, y))
                row = i;
        if (row < 0)
            return;
        if (settings_page == 0) {
            switch (row) {
            case 0:
                settings.rotation_locked = !settings.rotation_locked;
                requests |= Save;
                break;
            case 1:
                rotate();
                break;
            case 2:
                settings.brightness = settings.brightness >= 100 ? 20 : settings.brightness + 20;
                requests |= Save;
                break;
            case 3:
                settings.reduced_animation = !settings.reduced_animation;
                requests |= Save;
                break;
            case 4:
                screen = Screen::Alerts;
                break;
            }
        } else if (settings_page == 1) {
            switch (row) {
            case 0:
                settings.sound = !settings.sound;
                break;
            case 1:
                settings.threshold = settings.threshold == 80   ? 20
                                     : settings.threshold == 20 ? 50
                                                                : 80;
                break;
            case 2:
                settings.region = (settings.region + 1) % 2;
                break;
            case 3:
                settings.low_feeding = !settings.low_feeding;
                break;
            case 4:
                settings.ibeacon_alerts = !settings.ibeacon_alerts;
                settings.alert_categories ^= 1U << unsigned(Category::IBEACON);
                break;
            }
            requests |= Save;
        } else if (settings_page == 2) {
            switch (row) {
            case 0:
                if (settings.research) {
                    settings.research = false;
                    requests |= Save;
                } else
                    screen = Screen::ResearchWarning;
                break;
            case 1:
                requests |= Export;
                break;
            case 2:
                requests |= Eject;
                break;
            case 3:
                demo = !demo;
                requests |= Demo;
                screen = Screen::Home;
                break;
            case 4:
                screen = Screen::Diagnostics;
                break;
            }
        } else if (settings_page == 4) {
            switch (row) {
            case 0:
                screen = Screen::Clock;
                break;
            case 1:
                screen = Screen::Battery;
                break;
            case 2:
                screen = Screen::ClearWarning;
                break;
            case 3:
                screen = Screen::SelfTest;
                break;
            case 4:
                requests |= Export;
                break;
            }
        } else {
            switch (row) {
            case 0:
                requests |= Calibrate;
                screen = Screen::Calibration;
                break;
            case 1:
                screen = Screen::ResetWarning;
                break;
            case 2:
                screen = Screen::Choose;
                break;
            case 3:
                settings_page = 0;
                break;
            case 4:
                screen = Screen::Home;
                break;
            }
        }
    }
}
void View::render(int tile_y, std::span<uint16_t> pixels) {
    const Layout l(settings.portrait);
    if (tile_y < 0 || tile_y >= l.h || pixels.size() != size_t(l.w * tile_rows()))
        return;
    std::fill(pixels.begin(), pixels.end(), bg);
    Canvas c{tile_y, l.w, l.h, tile_rows(), pixels};
    char text[100];
    c.rect(0, 0, l.w, 26, panel);
    c.text(12, 9, demo ? "DEMO / NO SAVES" : settings.research ? "RAW LOG" : "PRIVATE", mint, 1);
    c.text(l.w - 174, 9, sd_error ? "SD ERROR" : sd ? "SD OK" : "SD OFF", muted, 1);
    c.text(l.w - 57, 9, settings.rotation_locked ? "LOCKED" : "TURN",
           settings.rotation_locked ? muted : pink, 1);
    c.text(13, 42, titles[int(screen)], pink, 2);
    c.text(12, 40, titles[int(screen)], ink, 2);
    switch (screen) {
    case Screen::Welcome:
        c.sprite(l.w - 154, l.portrait ? 164 : 78, settings.character,
                 settings.reduced_animation ? 0 : int(now / 250) % 4, 2);
        c.text(16, 88, "GOOD DOG.", mint, 3);
        c.text(16, 120, "SHARP NOSE.", pink, 2);
        c.wrap(16, l.portrait ? 312 : 160, "Passive clues. No uploads. No proof of intent.",
               l.portrait ? 24 : 25, muted, 1);
        c.text(16, l.h - 102, "CHOOSE YOUR RADIO REGION", muted, 1);
        button(c, l.confirm(false), "US 1-11");
        button(c, l.confirm(true), "EU 1-13");
        break;
    case Screen::Calibration: {
        c.text(12, 68, "TAP EACH CROSS WITH A STYLUS", muted, 1);
        auto p = display::target(std::clamp(calibration_step, 0, 2), l.portrait);
        c.rect(p.x - 15, p.y - 1, 31, 3, mint);
        c.rect(p.x - 1, p.y - 15, 3, 31, mint);
        break;
    }
    case Screen::Choose:
        for (int i = 0; i < 6; ++i) {
            auto r = l.choice(i);
            c.box(r.x, r.y, r.w, r.h, settings.character == i ? mint : grid);
            c.sprite(r.x + (r.w - 64) / 2, r.y + 2, i, 0, 1);
            c.text(r.x + (r.w - int(std::strlen(assets::names[i])) * 6) / 2, r.y + r.h - 17,
                   assets::names[i], settings.character == i ? mint : ink, 1);
        }
        break;
    case Screen::Name:
        c.text(16, 76, settings.name.data(), mint, 3);
        for (int i = 0; i < 30; ++i) {
            char key[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123"[i], 0};
            button(c, l.key(i), key);
        }
        button(c, l.confirm(false), "DELETE");
        button(c, l.confirm(true), "DONE", true);
        break;
    case Screen::Privacy:
        c.wrap(
            16, 94,
            "Private logs use a new anonymous token each boot. No raw names, packets or location.",
            l.portrait ? 24 : 37);
        button(c, l.confirm(false), "PRIVATE", true);
        button(c, l.confirm(true), "RESEARCH");
        break;
    case Screen::ResearchWarning:
        c.wrap(16, 94,
               "Research mode writes raw radio addresses to your SD card. These can identify "
               "devices. Exports stay sanitized.",
               l.portrait ? 24 : 37, amber);
        button(c, l.confirm(false), "CANCEL");
        button(c, l.confirm(true), "ENABLE");
        break;
    case Screen::ResetWarning:
        c.wrap(16, 94, "Reset this hound's XP and meals? Your observation logs remain on the card.",
               l.portrait ? 24 : 37);
        button(c, l.confirm(false), "CANCEL");
        button(c, l.confirm(true), "RESET");
        break;
    case Screen::Home: {
        int stage_w = l.portrait ? 296 : 280, stage_h = l.portrait ? 226 : 192;
        c.box(12, 64, stage_w, stage_h, grid);
        // A low-cost original night-patrol scene: falling signal marks and a perspective grid.
        for (int col = 0; col < 12; ++col) {
            int x = 22 + col * (stage_w - 20) / 12;
            int y = 72 + (col * 19 + (settings.reduced_animation ? 0 : int(now / 160))) % 80;
            for (int k = 0; k < 3; ++k)
                c.rect(x, y + k * 7, 2, 3, k == 0 ? mint : grid);
        }
        int horizon = 64 + stage_h - 49;
        for (int i = -4; i <= 4; ++i)
            c.line(12 + stage_w / 2, horizon,
                   std::clamp(12 + stage_w / 2 + i * stage_w / 4, 13, stage_w + 10),
                   64 + stage_h - 2, grid);
        for (int y = 0; y < 5; ++y)
            c.rect(13, horizon + y * y * 3, stage_w - 2, 1, grid);
        c.rect(20, 72, stage_w - 16, 21, panel);
        std::snprintf(text, sizeof(text), "%s / LV %u", settings.name.data(), pet.level());
        c.text(24, 78, text, mint, 1);
        const auto snack = snack_pose();
        int frame = settings.reduced_animation ? 0 : int(now / 250) % 4;
        const char *state = "ON PATROL", *line = "ALL QUIET. GOOD COMPANY.";
        if (paused) {
            frame = 17 + (settings.reduced_animation ? 0 : int(now / 900) % 3);
            state = "SLEEPING";
            line = "RESTING MY NOSE.";
        } else if (snack.phase != SnackPhase::None) {
            frame = snack.frame;
            state = snack.phase == SnackPhase::Turn       ? "A NEW TREAT"
                    : snack.phase == SnackPhase::Approach ? "WALKING UP"
                    : snack.phase == SnackPhase::Chew     ? "CHOMP CHOMP"
                                                          : "HAPPY HOUND";
            line = snack.phase == SnackPhase::Happy       ? "GONE. THAT WAS DELICIOUS."
                   : meal_category == Category::FLIPPER   ? "DOLPHIN SNACK!"
                   : meal_category == Category::PINEAPPLE ? "PINEAPPLE SNACK!"
                                                          : "A CLUE AND A CHEW.";
        } else if (level_until > now) {
            frame = 22 + (settings.reduced_animation ? 0 : int(now / 166) % 3);
            state = "LEVEL UP!";
            line = "A LITTLE OLDER. STILL A PUP.";
        } else if (happy_until > now) {
            frame = 22 + int(now / 166) % 3;
            state = "GOOD HOUND";
            line = "THAT IS THE SPOT.";
        } else if (pet.fullness < 25) {
            frame = 20 + int(now / 500) % 2;
            state = "PECKISH";
            line = "NO RUSH. LET'S WANDER.";
        } else if ((now / 15000) % 4 == 1) {
            frame = 4 + int(now / 125) % 4;
            state = "PATROLLING";
        } else if ((now / 15000) % 4 == 2) {
            frame = 8 + int(now / 166) % 3;
            state = "SNIFFING";
            line = "NOSE DOWN. EARS UP.";
        } else if ((now / 15000) % 4 == 3) {
            frame = 17 + int(now / 500) % 3;
            state = "RESTING";
            line = "EVEN HOUNDS TAKE BREAKS.";
        }
        if (settings.reduced_animation && snack.phase == SnackPhase::None)
            frame = paused                                     ? 17
                    : (happy_until > now || level_until > now) ? 22
                    : pet.fullness < 25                        ? 20
                                                               : 0;
        int dog_x = 12 + (stage_w - 128) / 2, dog_y = 95 + (l.portrait ? 22 : 0);
        int food_x = stage_w - 30, food_y = dog_y + 68;
        if (snack.phase == SnackPhase::Approach || snack.phase == SnackPhase::Chew)
            dog_x += (food_x - 124 - dog_x) * snack.approach / 100;
        if (snack.phase != SnackPhase::None && snack.phase != SnackPhase::Happy)
            c.food(food_x, food_y, meal_category, snack.bites);
        c.sprite(dog_x, dog_y, settings.character, frame);
        if (paused)
            for (int i = 0; i < 3; ++i) {
                int drift = settings.reduced_animation ? 0 : int(now / 250) % 12;
                c.text(dog_x + 89 + i * 13, dog_y + 27 - i * 13 - drift, "Z", mint, i == 2 ? 2 : 1);
            }
        if (snack.phase == SnackPhase::Chew && !settings.reduced_animation && snack.bites < 4) {
            int bounce = int(now / 160) % 3;
            uint16_t crumbs = meal_category == Category::FLIPPER ? mint : amber;
            c.rect(food_x + 3, food_y + 26 + bounce * 3, 3, 3, crumbs);
            c.rect(food_x + 12, food_y + 29 + bounce * 2, 2, 2, crumbs);
        }
        c.rect(20, 64 + stage_h - 23, stage_w - 16, 16, panel);
        c.text(25, 64 + stage_h - 19, line, mint, 1);
        int sx = l.portrait ? 16 : 306, sy = l.portrait ? 304 : 72;
        c.text(sx, sy, state, mint, 2);
        c.text(sx, sy + 27, "FULLNESS", muted, 1);
        c.rect(sx + 60, sy + 25, 92, 9, panel);
        c.rect(sx + 60, sy + 25, pet.fullness * 92 / 100, 9, mint);
        c.text(sx, sy + 45, "MOOD", muted, 1);
        c.rect(sx + 60, sy + 43, 92, 9, panel);
        c.rect(sx + 60, sy + 43, pet.mood * 92 / 100, 9, pink);
        std::snprintf(text, sizeof(text), "%lu STRONG", static_cast<unsigned long>(counts[0]));
        c.text(sx, sy + 72, text, ink, l.portrait ? 1 : 2);
        std::snprintf(text, sizeof(text), "%lu LIKELY", static_cast<unsigned long>(counts[1]));
        c.text(l.portrait ? sx + 148 : sx, sy + (l.portrait ? 72 : 94), text, ink,
               l.portrait ? 1 : 2);
        if (!l.portrait)
            c.text(sx, sy + 122,
                   demo       ? "SYNTHETIC SIGHTINGS"
                   : paused   ? "SNIFFING STOPPED"
                   : scanning ? "LISTENING ONLY"
                              : "RADIO PAUSED",
                   muted, 1);
        if (!l.portrait) {
            if (battery_level >= 0) {
                std::snprintf(text, sizeof(text), "BAT %d%% / %d MV", battery_level,
                              battery_millivolts);
                c.text(sx, sy + 143, text, mint, 1);
            } else
                c.text(sx, sy + 143,
                       paused ? "RADIOS STOPPED"
                       : ble  ? "BLE WINDOW"
                              : "WI-FI WINDOW",
                       mint, 1);
        }
        if (notice[0])
            c.text(12, l.h - 58, notice.data(), amber, 1);
        button(c, l.sniff(), paused ? "START SNIFFING" : "STOP SNIFFING", paused);
        if (alert_until > now) {
            const auto &d = alert_detection;
            bool compact = snack.phase != SnackPhase::None;
            int x = compact ? (l.portrait ? 12 : 302) : (l.portrait ? 12 : 40);
            int y = compact ? (l.portrait ? 302 : 66) : (l.portrait ? 108 : 78);
            int w = compact ? (l.portrait ? 296 : 166) : (l.portrait ? 296 : 400);
            int h = compact ? (l.portrait ? 90 : 154) : (l.portrait ? 250 : 178);
            c.box(x, y, w, h, pink);
            c.rect(x + 1, y + 1, w - 2, compact ? 24 : 32, pink);
            c.text(x + 10, y + 9, "NEW SCENT", bg, compact ? 1 : 2);
            c.text(x + 10, y + (compact ? 34 : 46), categories[size_t(d.category)], ink,
                   compact && !l.portrait ? 1 : 2);
            std::snprintf(text, sizeof(text), "%s / %u", badge(d.score), d.score);
            c.text(x + 10, y + (compact ? 56 : 72), text, amber, compact ? 1 : 2);
            if (!(compact && l.portrait))
                c.wrap(x + 10, y + (compact ? 75 : 100),
                       d.rule_count && d.rules[0] ? d.rules[0]->reason : "Observation received",
                       compact ? (l.portrait ? 45 : 23) : (l.portrait ? 42 : 60), muted, 1);
            c.text(x + 10, y + h - 16, d.demo ? "DEMO / TAP TO DISMISS" : "TAP TO DISMISS", mint,
                   1);
        }
        break;
    }
    case Screen::Log:
        if (!recent_count)
            c.wrap(16, 100, "No scents yet. A quiet place is fine. Your hound is happy to wait.",
                   l.portrait ? 24 : 37);
        for (size_t i = 0; i < 4 && size_t(log_page * 4) + i < recent_count; ++i) {
            const auto &d = recent[log_page * 4 + i];
            auto r = l.log(i);
            c.box(r.x, r.y, r.w, r.h);
            c.text(r.x + 10, r.y + 8, categories[size_t(d.category)], ink, 2);
            std::snprintf(text, sizeof(text), "%s %u / %s%s", badge(d.score), d.score,
                          d.radio == Radio::Ble ? "BLE" : "WI-FI", d.demo ? " / DEMO" : "");
            c.text(r.x + 10, r.y + (l.portrait ? 36 : 26), text, mint, 1);
        }
        std::snprintf(text, sizeof(text), "NEXT PAGE / %d OF %u", log_page + 1,
                      unsigned(std::max<size_t>(1, (recent_count + 3) / 4)));
        button(c, l.more(), text);
        break;
    case Screen::Detectors:
        for (int i = 0; i < 4; ++i) {
            int index = detector_page * 4 + i;
            if (index >= int(category_count))
                break;
            bool enabled = settings.enabled_categories & (1U << index), verified = false;
            for (auto &r : production_rules)
                if (int(r.category) == index)
                    verified = true;
            auto r = l.detector(i);
            c.box(r.x, r.y, r.w, r.h);
            c.text(r.x + 10, r.y + 5, categories[index], ink, 2);
            c.text(r.x + 10, r.y + (l.portrait ? 36 : 26),
                   verified ? (enabled ? "RULES LOADED / ENABLED" : "DISABLED")
                            : "EXPERIMENTAL / NO LIVE RULE",
                   verified ? mint : muted, 1);
        }
        std::snprintf(text, sizeof(text), "NEXT PAGE / %d OF 5", detector_page + 1);
        button(c, l.more(), text);
        break;
    case Screen::Alerts:
        for (int i = 0; i < 4; ++i) {
            int index = alert_page * 4 + i;
            if (index >= int(category_count))
                break;
            bool enabled = settings.alert_categories & (1U << index), verified = false;
            for (auto &rule : production_rules)
                if (int(rule.category) == index)
                    verified = true;
            auto r = l.detector(i);
            c.box(r.x, r.y, r.w, r.h, enabled ? mint : grid);
            c.text(r.x + 10, r.y + 5, categories[index], ink, 2);
            c.text(r.x + r.w - 47, r.y + 5, enabled ? "ON" : "OFF", enabled ? mint : muted, 2);
            c.text(r.x + 10, r.y + (l.portrait ? 36 : 26),
                   verified ? "POP-UP + SOUND / LOG KEPT" : "DEMO ONLY / NO LIVE RULE", muted, 1);
        }
        std::snprintf(text, sizeof(text), "NEXT PAGE / %d OF 5", alert_page + 1);
        button(c, l.more(), text);
        break;
    case Screen::Settings:
        for (int i = 0; i < 5; ++i) {
            const char *label = "";
            if (settings_page == 0) {
                switch (i) {
                case 0:
                    label = settings.rotation_locked ? "ROTATION LOCK: ON" : "ROTATION LOCK: OFF";
                    break;
                case 1:
                    label = settings.portrait ? "VIEW: PORTRAIT" : "VIEW: LANDSCAPE";
                    break;
                case 2:
                    std::snprintf(text, sizeof(text), "BRIGHTNESS: %u", settings.brightness);
                    label = text;
                    break;
                case 3:
                    label = settings.reduced_animation ? "ANIMATION: REDUCED" : "ANIMATION: FULL";
                    break;
                case 4:
                    label = "ALERT TYPES";
                    break;
                }
            } else if (settings_page == 1) {
                const char *labels[] = {
                    settings.sound ? "SOUND: ON" : "SOUND: OFF",
                    settings.threshold == 80   ? "ALERTS: HIGH"
                    : settings.threshold == 50 ? "ALERTS: MED/HIGH"
                                               : "ALERTS: ALL",
                    settings.region == 0 ? "REGION: US" : "REGION: EU",
                    settings.low_feeding ? "LOW FEEDING: ON" : "LOW FEEDING: OFF",
                    settings.ibeacon_alerts ? "IBEACON ALERTS: ON" : "IBEACON ALERTS: OFF"};
                label = labels[i];
            } else if (settings_page == 2) {
                const char *labels[] = {settings.research ? "PRIVACY: RESEARCH"
                                                          : "PRIVACY: PRIVATE",
                                        "EXPORT HISTORY", "EJECT SD", "DEMO MODE", "DIAGNOSTICS"};
                label = labels[i];
            } else if (settings_page == 4) {
                const char *labels[] = {"SET UTC TIME", "BATTERY SETUP", "CLEAR SD LOGS",
                                        "SELF TEST", "EXPORT HISTORY"};
                label = labels[i];
            } else {
                const char *labels[] = {"CALIBRATE TOUCH", "RESET PROGRESS", "CHANGE HOUND",
                                        "DISPLAY SETTINGS", "BACK TO PATROL"};
                label = labels[i];
            }
            auto r = l.row(i);
            button(c, r, label, settings_page == 0 && i == 0);
            if (settings_page == 0 && i == 1 && settings.rotation_locked)
                c.text(r.x + r.w - 52, r.y + (r.h - 7) / 2, "LOCKED", muted, 1);
        }
        std::snprintf(text, sizeof(text), "MORE / %d OF 5", settings_page + 1);
        button(c, l.more(), text);
        break;
    case Screen::Clock:
        c.text(16, 68, "UTC YYYYMMDDHHMM - TAP VALUE TO DELETE", muted, 1);
        c.text(16, 88, clock_input.data(), mint, 2);
        for (int i = 0; i < 10; ++i) {
            char digit[] = {char('0' + i), 0};
            button(c, {16 + (i % 5) * (l.w - 32) / 5, 120 + (i / 5) * 48, (l.w - 32) / 5 - 4, 44},
                   digit);
        }
        c.text(16, 222, notice[0] ? notice.data() : "MANUAL TIME RESETS AFTER POWER LOSS",
               notice[0] ? amber : muted, 1);
        button(c, l.confirm(false), "CANCEL");
        button(c, l.confirm(true), "SET TIME", true);
        break;
    case Screen::Battery:
        for (int i = 0; i < 5; ++i) {
            if (i == 0)
                std::snprintf(text, sizeof(text), "CALIBRATION POINT: %d OF 3", battery_point + 1);
            if (i == 1)
                std::snprintf(text, sizeof(text), "LESS / %u MV / MORE",
                              battery_cal.millivolts[battery_point]);
            if (i == 2)
                std::snprintf(text, sizeof(text), "CAPTURE ADC %d / SAVED %u", battery_adc,
                              battery_cal.adc[battery_point]);
            if (i == 3)
                std::snprintf(text, sizeof(text), "BATTERY MODE: %s",
                              battery_cal.enabled          ? "ON"
                              : valid_battery(battery_cal) ? "OFF"
                                                           : "NEEDS 3 POINTS");
            if (i == 4)
                std::snprintf(text, sizeof(text), "CLEAR CALIBRATION");
            button(c, l.row(i), text, i == battery_point);
        }
        button(c, l.more(), "BACK / MEASURE WITH METER");
        break;
    case Screen::ClearWarning:
        c.wrap(16, 85,
               "Permanently delete all observation logs from the SD card? Exports and hound "
               "progress remain.",
               l.portrait ? 24 : 37);
        button(c, l.confirm(false), "CANCEL");
        button(c, l.confirm(true), "DELETE LOGS");
        break;
    case Screen::SelfTest:
        c.text(16, 72, "DISPLAY / TOUCH / COLOR TEST", mint, 1);
        for (int i = 0; i < 3; ++i)
            c.rect(16 + i * (l.w - 32) / 3, 94, (l.w - 32) / 3 - 4, 44,
                   i == 0   ? 0xf800
                   : i == 1 ? 0x07e0
                            : 0x001f);
        c.box(12, 145, l.w - 24, 70, mint);
        std::snprintf(text, sizeof(text), "TAPS %u / X %d Y %d", self_test_taps, touch_x, touch_y);
        c.text(20, 160, text, ink, 1);
        c.text(20, 183, "TOUCH CORNERS AND CHECK ALL COLORS", muted, 1);
        button(c, l.more(), "BACK TO SETTINGS");
        break;
    case Screen::Diagnostics: {
        c.text(l.w - 80, 42, "SAVE SD", mint, 1);
        const char *labels0[] = {"HEAP",    "MIN HEAP", "WIFI RX",    "BLE RX",
                                 "WIFI OK", "BLE OK",   "WIFI BAD",   "BLE BAD",
                                 "DROPPED", "OVERSIZE", "QUEUE HIGH", "RADIO ERR"};
        uint64_t values0[] = {heap,          min_heap,     wifi_count,     ble_count,
                              wifi_accepted, ble_accepted, wifi_malformed, ble_malformed,
                              dropped,       oversized,    queue_high,     radio_errors};
        const char *labels1[] = {"UI STACK",  "RADIO STACK", "BLE STACK",   "SD STACK",
                                 "SD WRITES", "EXPORTS",     "SD FREE KB",  "SD TOTAL KB",
                                 "BAT ADC",   "BAT MV",      "BAT PERCENT", "RESET"};
        uint64_t values1[] = {stack_free,
                              radio_stack,
                              ble_stack,
                              storage_stack,
                              writes,
                              export_count,
                              sd_free / 1024,
                              sd_total / 1024,
                              uint64_t(std::max(0, battery_adc)),
                              uint64_t(std::max(0, battery_millivolts)),
                              uint64_t(std::max(0, battery_level)),
                              boot_reason};
        if (diagnostic_page < 2)
            for (int i = 0; i < 12; ++i) {
                std::snprintf(
                    text, sizeof(text), "%s: %llu", diagnostic_page ? labels1[i] : labels0[i],
                    static_cast<unsigned long long>(diagnostic_page ? values1[i] : values0[i]));
                c.text(l.portrait ? 16 : 16 + (i / 6) * 234,
                       l.portrait ? 90 + i * 23 : 82 + (i % 6) * 24, text, ink, 1);
            }
        else {
            c.text(16, 85,
                   sd_read_only ? "SD READ ONLY / CHECK CARD"
                   : sd_error   ? "SD ERROR"
                   : sd         ? "SD READY"
                                : "SD UNAVAILABLE",
                   amber, 1);
            c.text(16, 112,
                   battery_cal.enabled ? "BATTERY ESTIMATE / CALIBRATED" : "BATTERY NOT ENABLED",
                   mint, 1);
            char utc[24];
            format_utc(utc_now(), utc);
            c.text(16, 139, utc[0] ? utc : "TIME: UPTIME ONLY", ink, 1);
            c.text(16, 166, signature_pack_version, muted, 1);
            c.text(16, 193, "HARDWARE VALIDATION STILL REQUIRED", muted, 1);
        }
        std::snprintf(text, sizeof(text), "NEXT PAGE / %d OF 3", diagnostic_page + 1);
        button(c, l.more(), text);
        break;
    }
    case Screen::Details:
        if (detail_valid || selected < recent_count) {
            const auto &d = detail_valid ? detail_detection : recent[selected];
            button(c, {l.w - 108, 32, 96, 30}, detail_page ? "EVIDENCE" : "MORE");
            if (detail_page) {
                c.text(16, 80, "REMOTE ID / LOCAL SNAPSHOT", mint, 1);
                if (d.category != Category::DRONE || !detail_drone.present) {
                    c.wrap(16, 112, "No fresh Remote ID detail for this sighting.",
                           l.portrait ? 24 : 36, muted, 2);
                } else {
                    const auto &t = detail_drone;
                    char serial[21]{};
                    sanitize(t.basic.id, serial);
                    std::snprintf(text, sizeof(text), "BASIC ID: %s",
                                  t.present & 1 ? serial : "NOT RECEIVED");
                    c.text(16, 108, text, ink, 1);
                    if ((t.present & 2) && t.location.location_valid)
                        std::snprintf(text, sizeof(text), "AIRCRAFT: %.5f / %.5f",
                                      t.location.latitude_e7 / 1e7, t.location.longitude_e7 / 1e7);
                    else
                        std::snprintf(text, sizeof(text), "AIRCRAFT LOCATION: UNAVAILABLE");
                    c.text(16, 130, text, ink, 1);
                    if ((t.present & 2) && t.location.altitude_valid)
                        std::snprintf(text, sizeof(text), "ALTITUDE: %.1f M",
                                      t.location.altitude_dm / 10.0);
                    else
                        std::snprintf(text, sizeof(text), "ALTITUDE: UNAVAILABLE");
                    c.text(16, 152, text, ink, 1);
                    if ((t.present & 4) && t.system.location_valid)
                        std::snprintf(text, sizeof(text), "OPERATOR: %.5f / %.5f",
                                      t.system.latitude_e7 / 1e7, t.system.longitude_e7 / 1e7);
                    else
                        std::snprintf(text, sizeof(text), "OPERATOR LOCATION: UNAVAILABLE");
                    c.text(16, 174, text, ink, 1);
                    c.text(16, 210, "BROADCAST CLAIMS / NOT AUTHENTICATED", amber, 1);
                }
                button(c, l.more(), "BACK TO LOG");
                break;
            }
            c.text(16, 78, categories[size_t(d.category)], mint, 2);
            c.text(l.portrait ? 16 : 250, l.portrait ? 108 : 78, badge(d.score), amber, 2);
            c.wrap(16, l.portrait ? 140 : 110,
                   d.rule_count && d.rules[0] ? d.rules[0]->reason : "No rule details",
                   l.portrait ? 24 : 37, muted, l.portrait ? 2 : 1);
            std::snprintf(text, sizeof(text), "RSSI %d TO %d DBM / SEEN %lu", d.rssi_min,
                          d.rssi_max, static_cast<unsigned long>(d.seen_count));
            c.text(16, l.portrait ? 252 : 154, text, ink, 1);
            for (size_t i = 0; i < d.rule_count; ++i)
                if (d.rules[i])
                    c.text(16, (l.portrait ? 286 : 175) + int(i) * 14, d.rules[i]->id, muted, 1);
            std::snprintf(text, sizeof(text), "CHANNEL %u / FIRST %lluS / LAST %lluS", d.channel,
                          static_cast<unsigned long long>(d.first_ms / 1000),
                          static_cast<unsigned long long>(d.last_ms / 1000));
            c.text(16, l.portrait ? 356 : 222, text, mint, 1);
            button(c, l.more(), "BACK TO LOG");
        }
        break;
    }
    bool nav = settings.onboarded && (screen == Screen::Home || screen == Screen::Log ||
                                      screen == Screen::Detectors || screen == Screen::Settings ||
                                      screen == Screen::Diagnostics || screen == Screen::Alerts);
    if (nav) {
        constexpr const char *labels[] = {"HOUND", "LOG", "SCAN", "SET"};
        constexpr Screen pages[] = {Screen::Home, Screen::Log, Screen::Detectors, Screen::Settings};
        for (int i = 0; i < 4; ++i) {
            int x = i * l.w / 4;
            c.rect(x, l.h - 44, l.w / 4 - 2, 44, panel);
            if (screen == pages[i])
                c.rect(x, l.h - 44, l.w / 4 - 2, 2, mint);
            c.text(x + (l.w / 4 - int(std::strlen(labels[i])) * 12) / 2, l.h - 28, labels[i],
                   screen == pages[i] ? mint : muted, 2);
        }
    }
}
} // namespace sniffer::ui
