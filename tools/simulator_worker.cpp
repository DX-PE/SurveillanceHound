// SPDX-License-Identifier: Apache-2.0
// Native pipe adapter for the shared synthetic session.
#include "simulator.hpp"
#include <iostream>

int main() {
    std::string command;
    int a, b;
    uint64_t ms;
    while (std::cin >> command >> a >> b >> ms) {
        const auto &frame = hound_lab::step(command, a, b, ms);
        std::cout << frame.state;
        std::cout.write(reinterpret_cast<const char *>(frame.pixels.data()), frame.pixels.size());
        std::cout.flush();
    }
}
