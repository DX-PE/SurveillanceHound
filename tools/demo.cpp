// SPDX-License-Identifier: Apache-2.0
#include "signatures_generated.h"
#include "sniffer/core.hpp"
#include <iostream>
int main() {
    // Project-owned synthetic Find Hub frame; never touches SD or persistent state.
    std::array<uint8_t, 26> payload{25, 0x16, 0xaa, 0xfe, 0x40};
    sniffer::Observation o{};
    o.ms = 1000;
    o.rssi = -65;
    o.address = {0xc0, 0, 0, 0, 0, 1};
    o.address_type = 1;
    sniffer::parse_ble(payload, o);
    sniffer::Engine engine(sniffer::production_rules);
    sniffer::Settings settings;
    std::array<sniffer::Detection, 4> output{};
    auto n = engine.ingest(o, settings, output);
    for (size_t i = 0; i < n; ++i)
        std::cout << "DEMO ONLY: " << sniffer::badge(output[i].score) << " / "
                  << output[i].rules[0]->label << " / " << output[i].rules[0]->reason << "\n";
    return n ? 0 : 1;
}
