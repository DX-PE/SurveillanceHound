// SPDX-License-Identifier: Apache-2.0
#include "signatures_generated.h"
#include "sniffer/core.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *, size_t);
int main(int argc, char **argv) {
    unsigned seconds = argc > 1 ? std::clamp(std::atoi(argv[1]), 1, 3600) : 60;
    std::vector<std::vector<uint8_t>> seeds{{2, 1, 6, 3, 3, 0x5a, 0xfd},
                                            {30, 0xff, 0x4c, 0, 0x12, 0x19}};
    seeds.back().resize(31);
    std::vector<uint8_t> samsung{0x5a, 0xfd, 0x15, 0x21, 0x43, 0x01, 1, 2, 3, 4, 5, 6, 7, 8};
    samsung.resize(22);
    seeds.push_back(samsung);
    samsung.insert(samsung.begin(), {23, 0x16});
    seeds.push_back(samsung);
    seeds.push_back({0xfa, 0xff, 0x0d, 0, 0x02, 0x12, 'X'});
    seeds.back().resize(29);
    std::string pwn = R"({"name":"test","version":"1.0","pwnd_tot":4})";
    seeds.emplace_back(pwn.begin(), pwn.end());
    std::vector<uint8_t> beacon(36);
    beacon[0] = 0x80;
    beacon[10] = 0xde;
    beacon[11] = 0xad;
    beacon[12] = 0xbe;
    beacon[13] = 0xef;
    beacon[14] = 0xde;
    beacon[15] = 0xad;
    beacon.insert(beacon.end(), {222, uint8_t(pwn.size())});
    beacon.insert(beacon.end(), pwn.begin(), pwn.end());
    seeds.push_back(beacon);
    sniffer::Detection d{};
    d.category = sniffer::production_rules[0].category;
    d.rules[0] = &sniffer::production_rules[0];
    d.rule_count = 1;
    d.score = 85;
    d.seen_count = 1;
    d.rssi_min = d.rssi_max = d.rssi_sum = -50;
    std::array<char, 2048> record{};
    auto n = sniffer::write_record(d, "0123456789abcdef0123456789abcdef", "ABCDEFGHIJKLMNOPQRST",
                                   true, record);
    seeds.emplace_back(record.begin(), record.begin() + n);
    uint32_t state = 0x51a721;
    auto next = [&]() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    };
    auto end = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    uint64_t runs = 0;
    for (const auto &seed : seeds)
        LLVMFuzzerTestOneInput(seed.data(), seed.size());
    do {
        auto bytes = seeds[next() % seeds.size()];
        for (unsigned edits = 1 + next() % 8; edits; --edits) {
            auto at = next() % (bytes.size() + 1);
            switch (next() % 4) {
            case 0:
                if (at < bytes.size())
                    bytes[at] ^= 1 << (next() % 8);
                break;
            case 1:
                if (bytes.size() < 2048)
                    bytes.insert(bytes.begin() + at, uint8_t(next()));
                break;
            case 2:
                if (at < bytes.size())
                    bytes.erase(bytes.begin() + at);
                break;
            case 3:
                if (at < bytes.size())
                    bytes[at] = uint8_t(next());
                break;
            }
        }
        LLVMFuzzerTestOneInput(bytes.data(), bytes.size());
        ++runs;
    } while (std::chrono::steady_clock::now() < end);
    std::printf("%llu seeded mutation inputs passed in %u seconds\n",
                static_cast<unsigned long long>(runs), seconds);
}
