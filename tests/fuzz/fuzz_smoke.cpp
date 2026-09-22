// SPDX-License-Identifier: Apache-2.0
#include "sniffer/core.hpp"
#include <array>
#include <iostream>
int main() {
    uint32_t state = 0x91545;
    std::array<uint8_t, 512> bytes{};
    auto next = [&]() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    };
    for (int trial = 0; trial < 100000; ++trial) {
        auto size = next() % 513;
        for (size_t i = 0; i < size; ++i)
            bytes[i] = next();
        sniffer::Observation o{};
        sniffer::RemoteId rid{};
        sniffer::parse_wifi(std::span(bytes).first(size), o);
        sniffer::parse_ble(std::span(bytes).first(std::min<size_t>(size, 31)), o);
        sniffer::parse_remote_id(std::span(bytes).first(std::min<size_t>(size, 29)), rid);
        std::array<char, 3074> out{};
        sniffer::valid_log_record({reinterpret_cast<const char *>(bytes.data()), size});
        sniffer::parse_pwnagotchi(std::span(bytes).first(size), out);
        sniffer::json_escape({reinterpret_cast<const char *>(bytes.data()), size}, out);
    }
    std::cout << "100000 deterministic parser/JSON fuzz cases passed\n";
}
