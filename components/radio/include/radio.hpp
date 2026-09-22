// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "sniffer/core.hpp"
#include <atomic>
namespace sniffer::radio {
struct Raw {
    std::array<uint8_t, 512> bytes{};
    uint64_t ms{};
    Address address{};
    uint16_t size{};
    uint8_t channel{}, address_type{};
    int8_t rssi{};
    Radio radio{};
};
struct Metrics {
    std::atomic<uint32_t> wifi_seen{}, ble_seen{}, dropped{}, oversized{}, queue_high{}, errors{},
        cycles{}, stack_free{}, ble_stack{};
    std::atomic<uint8_t> channel{}, region{};
    std::atomic<bool> ble_window{}, paused{}, ready{};
};
extern Metrics metrics;
bool start(uint8_t region);
bool receive(Raw &raw);
void pause(bool paused);
} // namespace sniffer::radio
