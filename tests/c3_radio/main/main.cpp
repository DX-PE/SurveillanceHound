// SPDX-License-Identifier: Apache-2.0
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "radio.hpp"
#include "signatures_generated.h"
#include "sniffer/core.hpp"
#include <algorithm>
#include <cstdio>
using namespace sniffer;
static Engine engine(production_rules);
extern "C" void app_main() {
    std::printf("HOUND_C3 receive-only test pack=%s\n", signature_pack_version);
    if (nvs_flash_init() != ESP_OK) {
        std::printf("HOUND_C3 NVS_INIT_FAILED\n");
        return;
    }
    if (!radio::start(0)) {
        std::printf("HOUND_C3 RADIO_INIT_FAILED\n");
        return;
    }
    uint32_t valid = 0, bad = 0, samsung = 0, vendor = 0;
    int min = -127, max = -127;
    uint64_t last = 0;
    Settings settings;
    bool paused = false, resumed = false;
    for (;;) {
        radio::Raw raw{};
        while (radio::receive(raw)) {
            Observation o{};
            o.ms = raw.ms;
            o.rssi = raw.rssi;
            o.channel = raw.channel;
            o.address = raw.address;
            o.address_type = raw.address_type;
            bool ok = raw.radio == Radio::Ble ? parse_ble(std::span(raw.bytes).first(raw.size), o)
                                              : parse_wifi(std::span(raw.bytes).first(raw.size), o);
            if (!ok) {
                ++bad;
                continue;
            }
            ++valid;
            if (o.radio == Radio::Ble) {
                bool smart = false;
                for (unsigned i = 0; i < o.uuid_count; ++i)
                    smart |= o.uuids[i] == 0xfd5a;
                if (o.manufacturer_len >= 2 && o.manufacturer[0] == 0x75 && o.manufacturer[1] == 0)
                    ++vendor;
                if (smart) {
                    if (!samsung) {
                        min = max = o.rssi;
                        std::printf("SAMSUNG_SERVICE FD5A service_bytes=%u manufacturer_bytes=%u\n",
                                    o.service_len, o.manufacturer_len);
                    }
                    ++samsung;
                    min = std::min(min, int(o.rssi));
                    max = std::max(max, int(o.rssi));
                }
            }
            std::array<Detection, 4> found{};
            auto n = engine.ingest(o, settings, found);
            for (size_t i = 0; i < n; ++i)
                std::printf("MATCH category=%s score=%u rule=%s rssi=%d\n",
                            categories[unsigned(found[i].category)], found[i].score,
                            found[i].rules[0]->id, o.rssi);
        }
        uint64_t now = esp_timer_get_time() / 1000;
        if (now >= 45000 && !paused) {
            radio::pause(true);
            paused = true;
            std::printf("PAUSE_TEST STOP\n");
        }
        if (now >= 65000 && !resumed) {
            radio::pause(false);
            resumed = true;
            std::printf("PAUSE_TEST START\n");
        }
        if (now - last >= 10000) {
            std::printf("C3_RESULT seconds=%llu wifi=%lu ble=%lu parsed=%lu "
                        "rejected=%lu samsung_service=%lu samsung_vendor=%lu "
                        "rssi_min=%d rssi_max=%d dropped=%lu errors=%lu\n",
                        static_cast<unsigned long long>(now / 1000),
                        static_cast<unsigned long>(radio::metrics.wifi_seen.load()),
                        static_cast<unsigned long>(radio::metrics.ble_seen.load()),
                        static_cast<unsigned long>(valid), static_cast<unsigned long>(bad),
                        static_cast<unsigned long>(samsung), static_cast<unsigned long>(vendor),
                        min, max, static_cast<unsigned long>(radio::metrics.dropped.load()),
                        static_cast<unsigned long>(radio::metrics.errors.load()));
            last = now;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
