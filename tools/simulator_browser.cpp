// SPDX-License-Identifier: Apache-2.0
#include "simulator.hpp"
#include <cmath>
#include <emscripten.h>

namespace {
const hound_lab::Frame *current{};
}
extern "C" {
EMSCRIPTEN_KEEPALIVE int hound_step(const char *command, int a, int b, double ms) {
    if (!command || !std::isfinite(ms) || ms < 0 || ms > 9007199254740991.0)
        return 0;
    current = &hound_lab::step(command, a, b, static_cast<uint64_t>(ms));
    return 1;
}
EMSCRIPTEN_KEEPALIVE const char *hound_state() {
    return current ? current->state.c_str() : nullptr;
}
EMSCRIPTEN_KEEPALIVE const uint8_t *hound_pixels() {
    return current ? current->pixels.data() : nullptr;
}
}
