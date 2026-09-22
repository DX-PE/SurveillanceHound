// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <span>
namespace board {
inline constexpr int tft_cs = 15, tft_dc = 2, tft_clk = 14, tft_mosi = 13, tft_miso = 12,
                     backlight = 27;
inline constexpr int touch_cs = 33, touch_irq = 36, sd_cs = 5, sd_mosi = 23, sd_clk = 18,
                     sd_miso = 19;
inline constexpr int amp_enable = 4, audio_dac = 26, red = 22, green = 16, blue = 17, boot = 0,
                     battery = 34;
inline constexpr int i2c_clk = 25, i2c_data = 32, expansion_1 = 35, expansion_2 = 39;
struct Point {
    int x{}, y{};
};
struct Calibration {
    float ax{}, bx{}, cx{}, ay{}, by{}, cy{};
    bool valid{};
};
bool init();
void orientation(bool portrait);
void inversion(bool enabled);
void blit(int y, int height, std::span<uint16_t> pixels);
bool touch(Point &raw);
Point map(Point raw, const Calibration &);
bool calibrate(const Point raw[3], const Point screen[3], Calibration &result);
void brightness(unsigned percent);
bool button();
void led(bool r, bool g, bool b);
int battery_raw();
void sound(unsigned effect, bool enabled);
bool mount_sd();
void sleep();
} // namespace board
