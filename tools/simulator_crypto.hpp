// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstddef>
#include <cstdint>

// Browser-only Web Crypto bridge. ESP32 and native builds retain their existing backends.
extern "C" {
int browser_hmac(const uint8_t *key, size_t key_size, const uint8_t *input, size_t input_size,
                 uint8_t *output);
int browser_sha256(const uint8_t *input, size_t input_size, uint8_t *output);
}
