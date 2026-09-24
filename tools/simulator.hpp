// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace hound_lab {
struct Frame {
    std::string state;
    std::array<uint8_t, 480 * 320 * 2> pixels{};
};
// One RAM-only session per process/module. Returned storage lasts until the next step.
const Frame &step(std::string_view command, int a, int b, uint64_t ms);
} // namespace hound_lab
