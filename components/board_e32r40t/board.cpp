// SPDX-License-Identifier: Apache-2.0
#include "board.hpp"
#include "driver/dac_oneshot.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_sleep.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdmmc_cmd.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
namespace board {
namespace {
spi_device_handle_t display{}, digitizer{};
adc_oneshot_unit_handle_t adc{};
dac_oneshot_handle_t dac{};
bool portrait_mode{true};
void command(uint8_t cmd, std::span<const uint8_t> data = {}) {
    gpio_set_level(gpio_num_t(tft_dc), 0);
    spi_transaction_t t{};
    t.length = 8;
    t.tx_buffer = &cmd;
    ESP_ERROR_CHECK(spi_device_polling_transmit(display, &t));
    if (!data.empty()) {
        gpio_set_level(gpio_num_t(tft_dc), 1);
        t = {};
        t.length = data.size() * 8;
        t.tx_buffer = data.data();
        ESP_ERROR_CHECK(spi_device_polling_transmit(display, &t));
    }
}
uint16_t conversion(uint8_t cmd) {
    spi_transaction_t t{};
    t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    t.length = 24;
    t.tx_data[0] = cmd;
    if (spi_device_polling_transmit(digitizer, &t) != ESP_OK)
        return 0;
    return uint16_t((uint16_t(t.rx_data[1]) << 8) | t.rx_data[2]) >> 3;
}
} // namespace
bool init() {
    gpio_config_t outputs{};
    outputs.mode = GPIO_MODE_OUTPUT;
    outputs.pin_bit_mask =
        (1ULL << tft_dc) | (1ULL << red) | (1ULL << green) | (1ULL << blue) | (1ULL << amp_enable);
    if (gpio_config(&outputs) != ESP_OK)
        return false;
    gpio_set_level(gpio_num_t(amp_enable), 1);
    led(false, false, false);
    gpio_config_t inputs{};
    inputs.pin_bit_mask = (1ULL << boot);
    inputs.mode = GPIO_MODE_INPUT;
    inputs.pull_up_en = GPIO_PULLUP_ENABLE;
    if (gpio_config(&inputs) != ESP_OK)
        return false;
    inputs.pin_bit_mask = 1ULL << touch_irq;
    inputs.pull_up_en = GPIO_PULLUP_DISABLE;
    if (gpio_config(&inputs) != ESP_OK)
        return false;
    spi_bus_config_t bus{};
    bus.mosi_io_num = tft_mosi;
    bus.miso_io_num = tft_miso;
    bus.sclk_io_num = tft_clk;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = 320 * 16 * 2;
    if (spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK)
        return false;
    spi_device_interface_config_t panel{};
    panel.clock_speed_hz = 40000000;
    panel.spics_io_num = tft_cs;
    panel.queue_size = 1;
    if (spi_bus_add_device(SPI2_HOST, &panel, &display) != ESP_OK)
        return false;
    panel.clock_speed_hz = 1000000;
    panel.spics_io_num = touch_cs;
    if (spi_bus_add_device(SPI2_HOST, &panel, &digitizer) != ESP_OK)
        return false;
    // ST7796 command interface. Analog defaults must be checked on the physical panel.
    command(0x01);
    vTaskDelay(pdMS_TO_TICKS(150));
    command(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));
    const uint8_t format[] = {0x55};
    command(0x3a, format);
    orientation(false);
    command(0x21);
    command(0x29);
    ledc_timer_config_t timer{};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.duty_resolution = LEDC_TIMER_10_BIT;
    timer.timer_num = LEDC_TIMER_0;
    timer.freq_hz = 5000;
    timer.clk_cfg = LEDC_AUTO_CLK;
    if (ledc_timer_config(&timer) != ESP_OK)
        return false;
    ledc_channel_config_t channel{};
    channel.gpio_num = backlight;
    channel.speed_mode = LEDC_LOW_SPEED_MODE;
    channel.channel = LEDC_CHANNEL_0;
    channel.timer_sel = LEDC_TIMER_0;
    channel.duty = 700;
    if (ledc_channel_config(&channel) != ESP_OK)
        return false;
    adc_oneshot_unit_init_cfg_t adc_init{};
    adc_init.unit_id = ADC_UNIT_1;
    if (adc_oneshot_new_unit(&adc_init, &adc) == ESP_OK) {
        adc_oneshot_chan_cfg_t ch{};
        ch.bitwidth = ADC_BITWIDTH_DEFAULT;
        ch.atten = ADC_ATTEN_DB_12;
        adc_oneshot_config_channel(adc, ADC_CHANNEL_6, &ch);
    }
    dac_oneshot_config_t d{};
    d.chan_id = DAC_CHAN_1; // ESP32 channel 1 = GPIO26.
    if (dac_oneshot_new_channel(&d, &dac) == ESP_OK)
        dac_oneshot_output_voltage(dac, 128);
    return true;
}
void orientation(bool portrait) {
    // ST7796 MADCTL: portrait MX+BGR, landscape MV+BGR.
    const uint8_t orient[] = {uint8_t(portrait ? 0x48 : 0x28)};
    command(0x36, orient);
    portrait_mode = portrait;
}
void blit(int y, int height, std::span<uint16_t> pixels) {
    int width = portrait_mode ? 320 : 480, screen_height = portrait_mode ? 480 : 320;
    if (y < 0 || height <= 0 || height > 16 || y + height > screen_height ||
        pixels.size() != size_t(width * height))
        return;
    const uint8_t columns[] = {0, 0, uint8_t((width - 1) >> 8), uint8_t(width - 1)};
    const uint8_t rows[] = {uint8_t(y >> 8), uint8_t(y), uint8_t((y + height - 1) >> 8),
                            uint8_t(y + height - 1)};
    command(0x2a, columns);
    command(0x2b, rows);
    // Byte-swap the caller's DMA tile in place. Its next render replaces all pixels.
    for (auto &pixel : pixels)
        pixel = uint16_t((pixel << 8) | (pixel >> 8));
    command(0x2c, {reinterpret_cast<const uint8_t *>(pixels.data()), pixels.size() * 2});
}
bool touch(Point &raw) {
    if (gpio_get_level(gpio_num_t(touch_irq)))
        return false;
    std::array<uint16_t, 5> x{}, y{};
    for (size_t i = 0; i < x.size(); ++i) {
        x[i] = conversion(0xd0);
        y[i] = conversion(0x90);
    }
    std::sort(x.begin(), x.end());
    std::sort(y.begin(), y.end());
    if (x[2] < 100 || x[2] > 4000 || y[2] < 100 || y[2] > 4000)
        return false;
    raw = {x[2], y[2]};
    return true;
}
Point map(Point p, const Calibration &c) {
    if (!c.valid)
        return {};
    return {std::clamp(int(c.ax * p.x + c.bx * p.y + c.cx), 0, 319),
            std::clamp(int(c.ay * p.x + c.by * p.y + c.cy), 0, 479)};
}
bool calibrate(const Point p[3], const Point s[3], Calibration &c) {
    float det = float(p[0].x) * (p[1].y - p[2].y) + float(p[1].x) * (p[2].y - p[0].y) +
                float(p[2].x) * (p[0].y - p[1].y);
    if (std::fabs(det) < 10000)
        return false;
    auto solve = [&](int v0, int v1, int v2, float &a, float &b, float &d) {
        a = (float(v0) * (p[1].y - p[2].y) + float(v1) * (p[2].y - p[0].y) +
             float(v2) * (p[0].y - p[1].y)) /
            det;
        b = (float(v0) * (p[2].x - p[1].x) + float(v1) * (p[0].x - p[2].x) +
             float(v2) * (p[1].x - p[0].x)) /
            det;
        d = v0 - a * p[0].x - b * p[0].y;
    };
    solve(s[0].x, s[1].x, s[2].x, c.ax, c.bx, c.cx);
    solve(s[0].y, s[1].y, s[2].y, c.ay, c.by, c.cy);
    c.valid = true;
    return true;
}
void brightness(unsigned percent) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, std::clamp(percent, 5U, 100U) * 1023 / 100);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
bool button() {
    return !gpio_get_level(gpio_num_t(boot));
}
void led(bool r, bool g, bool b) {
    gpio_set_level(gpio_num_t(red), !r);
    gpio_set_level(gpio_num_t(green), !g);
    gpio_set_level(gpio_num_t(blue), !b);
}
int battery_raw() {
    int raw = -1;
    if (adc)
        adc_oneshot_read(adc, ADC_CHANNEL_6, &raw);
    return raw;
}
void sound(unsigned effect, bool enabled) {
    if (!enabled || !dac) {
        gpio_set_level(gpio_num_t(amp_enable), 1);
        return;
    }
    gpio_set_level(gpio_num_t(amp_enable), 0);
    // Original short, quiet step envelope. Called only by the low-priority storage task.
    const unsigned length[] = {50, 80, 120, 150};
    effect = std::min(effect, 3U);
    for (unsigned i = 0; i < length[effect]; ++i) {
        unsigned period = effect == 2 ? (i < 60 ? 3 : 2) : effect == 3 ? 5 : 2;
        int amplitude = (i < 6 || i + 6 >= length[effect]) ? 3 : 8;
        if (effect == 1 && (i / 20) % 2)
            amplitude = 0;
        dac_oneshot_output_voltage(dac, 128 + ((i / period) % 2 ? amplitude : -amplitude));
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    dac_oneshot_output_voltage(dac, 128);
    gpio_set_level(gpio_num_t(amp_enable), 1);
}
bool mount_sd() {
    spi_bus_config_t bus{};
    bus.mosi_io_num = sd_mosi;
    bus.miso_io_num = sd_miso;
    bus.sclk_io_num = sd_clk;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = 4096;
    if (spi_bus_initialize(SPI3_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK)
        return false;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;
    host.max_freq_khz = 10000;
    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = gpio_num_t(sd_cs);
    slot.host_id = SPI3_HOST;
    esp_vfs_fat_sdmmc_mount_config_t config{};
    config.format_if_mount_failed = false;
    config.max_files = 5;
    config.allocation_unit_size = 16384;
    sdmmc_card_t *card{};
    return esp_vfs_fat_sdspi_mount("/sd", &host, &slot, &config, &card) == ESP_OK;
}
void sleep() {
    sound(0, false);
    brightness(0);
    led(false, false, false);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    esp_deep_sleep_start();
}
} // namespace board
