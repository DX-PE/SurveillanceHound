// SPDX-License-Identifier: Apache-2.0
#include "radio.hpp"
#include "esp_event.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include <algorithm>
#include <cstring>
namespace sniffer::radio {
Metrics metrics;
namespace {
StaticQueue_t queue_state;
uint8_t queue_memory[16 * sizeof(Raw)];
QueueHandle_t queue;
std::atomic<bool> synced{};
void enqueue(const Raw &raw) {
    if (xQueueSend(queue, &raw, 0) != pdTRUE)
        ++metrics.dropped;
    auto used = uxQueueMessagesWaiting(queue);
    auto high = metrics.queue_high.load();
    while (used > high && !metrics.queue_high.compare_exchange_weak(high, used)) {
    }
}
void wifi_callback(void *buffer, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT || metrics.paused)
        return;
    ++metrics.wifi_seen;
    auto *packet = static_cast<wifi_promiscuous_pkt_t *>(buffer);
    auto size = packet->rx_ctrl.sig_len;
    if (size < 28)
        return;
    size -= 4; // Driver includes the trailing FCS in sig_len.
    if (size > 512) {
        ++metrics.oversized;
        return;
    }
    Raw raw{};
    raw.radio = Radio::Wifi;
    raw.ms = esp_timer_get_time() / 1000;
    raw.size = size;
    raw.channel = packet->rx_ctrl.channel;
    raw.rssi = packet->rx_ctrl.rssi;
    std::memcpy(raw.bytes.data(), packet->payload, size);
    enqueue(raw);
}
int gap_event(ble_gap_event *event, void *) {
    if (event->type != BLE_GAP_EVENT_DISC || metrics.paused)
        return 0;
    ++metrics.ble_seen;
    metrics.ble_stack = uxTaskGetStackHighWaterMark(nullptr);
    const auto &d = event->disc;
    if (d.length_data > 31) {
        ++metrics.oversized;
        return 0;
    }
    Raw raw{};
    raw.radio = Radio::Ble;
    raw.ms = esp_timer_get_time() / 1000;
    raw.size = d.length_data;
    raw.address_type = d.addr.type;
    raw.rssi = d.rssi;
    // NimBLE stores addresses little-endian; normalize before hashing/matching.
    std::reverse_copy(d.addr.val, d.addr.val + 6, raw.address.begin());
    std::memcpy(raw.bytes.data(), d.data, d.length_data);
    enqueue(raw);
    return 0;
}
void sync() {
    synced = ble_hs_util_ensure_addr(0) == 0;
}
void host(void *) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}
void schedule(void *) {
    esp_task_wdt_add(nullptr);
    for (int tries = 0; !synced && tries < 100; ++tries) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (!synced) {
        ++metrics.errors;
        esp_task_wdt_delete(nullptr);
        vTaskDelete(nullptr);
        return;
    }
    metrics.ready = true;
    for (;;) {
        esp_task_wdt_reset();
        metrics.stack_free = uxTaskGetStackHighWaterMark(nullptr);
        if (metrics.paused) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        uint8_t last = metrics.region == 0 ? 11 : 13;
        wifi_country_t country{};
        std::memcpy(country.cc, metrics.region == 0 ? "US" : "EU", 2);
        country.schan = 1;
        country.nchan = last;
        country.policy = WIFI_COUNTRY_POLICY_MANUAL;
        if (esp_wifi_set_country(&country) != ESP_OK)
            ++metrics.errors;
        if (esp_wifi_set_promiscuous(true) != ESP_OK)
            ++metrics.errors;
        for (uint8_t channel = 1; channel <= last && !metrics.paused; ++channel) {
            if (esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE) != ESP_OK)
                ++metrics.errors;
            esp_task_wdt_reset();
            metrics.channel = channel;
            vTaskDelay(pdMS_TO_TICKS(CONFIG_SNIFFER_WIFI_DWELL_MS));
        }
        esp_wifi_set_promiscuous(false);
        metrics.channel = 0;
        if (!metrics.paused) {
            ble_gap_disc_params params{};
            params.passive = 1;
            params.itvl = 80;
            params.window = 80;
            params.filter_duplicates = 0;
            metrics.ble_window = true;
            int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, CONFIG_SNIFFER_BLE_WINDOW_MS, &params,
                                  gap_event, nullptr);
            if (rc) {
                ++metrics.errors;
            } else {
                unsigned waited = 0;
                while (ble_gap_disc_active() && !metrics.paused &&
                       waited < CONFIG_SNIFFER_BLE_WINDOW_MS + 500) {
                    vTaskDelay(pdMS_TO_TICKS(50));
                    waited += 50;
                    esp_task_wdt_reset();
                }
                if (ble_gap_disc_active())
                    ble_gap_disc_cancel();
                // Do not restart Wi-Fi while the controller still reports discovery active.
                for (int i = 0; i < 10 && ble_gap_disc_active(); ++i)
                    vTaskDelay(pdMS_TO_TICKS(20));
                if (ble_gap_disc_active()) {
                    ++metrics.errors;
                    metrics.paused = true;
                }
            }
            metrics.ble_window = false;
        }
        ++metrics.cycles;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
} // namespace
bool start(uint8_t region) {
    metrics.region = region;
    queue = xQueueCreateStatic(16, sizeof(Raw), queue_memory, &queue_state);
    if (!queue)
        return false;
    auto rc = esp_event_loop_create_default();
    if (rc != ESP_OK && rc != ESP_ERR_INVALID_STATE)
        return false;
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&config) != ESP_OK || esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK ||
        esp_wifi_set_mode(WIFI_MODE_NULL) != ESP_OK || esp_wifi_start() != ESP_OK)
        return false;
    wifi_promiscuous_filter_t filter{};
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
    if (esp_wifi_set_promiscuous_filter(&filter) != ESP_OK ||
        esp_wifi_set_promiscuous_rx_cb(wifi_callback) != ESP_OK)
        return false;
    if (nimble_port_init() != ESP_OK)
        return false;
    ble_hs_cfg.sync_cb = sync;
    nimble_port_freertos_init(host);
    return xTaskCreatePinnedToCore(schedule, "radio_schedule", 4096, nullptr, 5, nullptr, 0) ==
           pdPASS;
}
bool receive(Raw &raw) {
    return queue && xQueueReceive(queue, &raw, 0) == pdTRUE;
}
void pause(bool paused) {
    metrics.paused = paused;
}
} // namespace sniffer::radio
