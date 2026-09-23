// SPDX-License-Identifier: Apache-2.0
#include "sniffer/core.hpp"
#include <algorithm>
#ifdef ESP_PLATFORM
#include "psa/crypto.h"
#else
#include <openssl/evp.h>
#include <openssl/hmac.h>
#endif
namespace sniffer {
bool hmac_sha256(std::span<const uint8_t> key, std::span<const uint8_t> input,
                 std::span<uint8_t, 32> out) {
#ifdef ESP_PLATFORM
    if (psa_crypto_init() != PSA_SUCCESS)
        return false;
    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);
    psa_set_key_algorithm(&attr, PSA_ALG_HMAC(PSA_ALG_SHA_256));
    mbedtls_svc_key_id_t id{};
    size_t n = 0;
    auto result = psa_import_key(&attr, key.data(), key.size(), &id);
    psa_reset_key_attributes(&attr);
    if (result != PSA_SUCCESS)
        return false;
    result = psa_mac_compute(id, PSA_ALG_HMAC(PSA_ALG_SHA_256), input.data(), input.size(),
                             out.data(), out.size(), &n);
    psa_destroy_key(id);
    return result == PSA_SUCCESS && n == out.size();
#else
    unsigned n = 0;
    return HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()), input.data(), input.size(),
                out.data(), &n) &&
           n == 32;
#endif
}
bool sha256(std::span<const uint8_t> input, std::span<uint8_t, 32> out) {
#ifdef ESP_PLATFORM
    size_t n = 0;
    return psa_crypto_init() == PSA_SUCCESS &&
           psa_hash_compute(PSA_ALG_SHA_256, input.data(), input.size(), out.data(), out.size(),
                            &n) == PSA_SUCCESS &&
           n == 32;
#else
    unsigned n = 0;
    return EVP_Digest(input.data(), input.size(), out.data(), &n, EVP_sha256(), nullptr) == 1 &&
           n == 32;
#endif
}
namespace {
// Different length and domain from legacy MAC hashes; no raw broadcast ID is persisted.
std::array<uint8_t, 13> samsung_input(const Detection &d, uint8_t purpose) {
    std::array<uint8_t, 13> input{'S', 'H', 1, purpose, uint8_t(Category::SAMSUNG_TAG)};
    std::copy(d.samsung.id.begin(), d.samsung.id.end(), input.begin() + 5);
    return input;
}
uint64_t digest_hash(std::span<const uint8_t> key, std::span<const uint8_t> input) {
    std::array<uint8_t, 32> digest{};
    if (!hmac_sha256(key, input, digest))
        return 0;
    uint64_t value = 0;
    for (size_t i = 0; i < 8; ++i)
        value = (value << 8) | digest[i];
    return value ? value : 1;
}
} // namespace
bool private_token(std::span<const uint8_t> key, const Detection &d, std::span<char, 21> out) {
    std::array<uint8_t, 8> input{};
    std::copy(d.address.begin(), d.address.end(), input.begin());
    input[6] = static_cast<uint8_t>(d.radio);
    input[7] = d.address_type;
    std::array<uint8_t, 32> digest{};
    const auto samsung = samsung_input(d, 1);
    if (!hmac_sha256(key, samsung_identity(d) ? std::span<const uint8_t>(samsung) : input,
                     digest)) {
        out[0] = 0;
        return false;
    }
    constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    uint32_t bits = 0;
    unsigned available = 0;
    size_t n = 0;
    for (size_t i = 0; i < 12; ++i) {
        bits = (bits << 8) | digest[i];
        available += 8;
        while (available >= 5) {
            available -= 5;
            out[n++] = alphabet[(bits >> available) & 31];
        }
    }
    if (available)
        out[n++] = alphabet[(bits << (5 - available)) & 31];
    out[n] = 0;
    return true;
}
uint64_t address_hash(std::span<const uint8_t> key, const Detection &d) {
    // Deliberately independent of the matched rule set: adding evidence cannot farm a new meal.
    std::array<uint8_t, 9> input{};
    std::copy(d.address.begin(), d.address.end(), input.begin());
    input[6] = static_cast<uint8_t>(d.radio);
    input[7] = d.address_type;
    input[8] = static_cast<uint8_t>(d.category);
    return digest_hash(key, input);
}
uint64_t meal_hash(std::span<const uint8_t> key, const Detection &d) {
    return samsung_identity(d) ? digest_hash(key, samsung_input(d, 2)) : address_hash(key, d);
}
} // namespace sniffer
