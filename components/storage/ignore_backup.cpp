// SPDX-License-Identifier: Apache-2.0
#include "ignore_backup.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <limits>
#include <unistd.h>
namespace sniffer::storage::detail {
namespace {
using Bytes = std::array<uint8_t, ignore_backup_bytes>;
constexpr std::array<uint8_t, 8> magic{'H', 'N', 'D', 'I', 'G', 'N', '0', '2'};
constexpr size_t signed_size = ignore_backup_bytes - 32;
enum class Read { Missing, Invalid, Error, Valid };
bool key_ok(std::span<const uint8_t, 32> key) {
    return std::any_of(key.begin(), key.end(), [](auto b) { return b != 0; });
}
bool path(const char *base, unsigned slot, std::span<char, 192> out) {
    int n = std::snprintf(out.data(), out.size(), "%s-%u.BIN", base, slot);
    return n > 0 && size_t(n) < out.size();
}
bool decode(const Bytes &bytes, std::span<const uint8_t, 32> key, IgnoreList &out,
            uint64_t &generation) {
    if (!key_ok(key) || !std::equal(magic.begin(), magic.end(), bytes.begin()))
        return false;
    std::array<uint8_t, 32> mac{};
    if (!hmac_sha256(key, std::span(bytes).first(signed_size), mac))
        return false;
    unsigned difference = 0;
    for (size_t i = 0; i < mac.size(); ++i)
        difference |= mac[i] ^ bytes[signed_size + i];
    if (difference)
        return false;
    uint64_t gen = 0;
    for (unsigned i = 0; i < 8; ++i)
        gen |= uint64_t(bytes[8 + i]) << (8 * i);
    if (!gen)
        return false;
    IgnoreList list{};
    for (size_t i = 0; i < list.size(); ++i) {
        std::copy_n(bytes.begin() + 16 + i * 9, 8, list[i].digest.begin());
        list[i].category = Category(bytes[24 + i * 9]);
    }
    if (!valid_ignores(list))
        return false;
    out = list;
    generation = gen;
    return true;
}
Read read_slot(const char *base, unsigned slot, std::span<const uint8_t, 32> key, IgnoreList &out,
               uint64_t &generation) {
    std::array<char, 192> name{};
    if (!path(base, slot, name))
        return Read::Error;
    FILE *f = std::fopen(name.data(), "rb");
    if (!f)
        return errno == ENOENT ? Read::Missing : Read::Error;
    std::setvbuf(f, nullptr, _IONBF, 0);
    Bytes bytes{};
    const auto n = std::fread(bytes.data(), 1, bytes.size(), f);
    const bool exact = n == bytes.size() && std::fgetc(f) == EOF;
    const bool error = std::ferror(f);
    const bool closed = std::fclose(f) == 0;
    if (error || !closed)
        return Read::Error;
    return exact && decode(bytes, key, out, generation) ? Read::Valid : Read::Invalid;
}
struct Latest {
    int slot{-1};
    uint64_t generation{};
    bool existing{}, error{};
};
Latest latest(const char *base, std::span<const uint8_t, 32> key, IgnoreList &out) {
    Latest result{};
    IgnoreList candidate{};
    for (unsigned slot = 0; slot < 2; ++slot) {
        uint64_t gen = 0;
        auto status = read_slot(base, slot, key, candidate, gen);
        result.existing |= status != Read::Missing;
        result.error |= status == Read::Error;
        if (status == Read::Valid && gen > result.generation) {
            result.slot = int(slot);
            result.generation = gen;
            out = candidate;
        }
    }
    return result;
}
} // namespace
bool ignore_backup_id(std::span<const uint8_t, 32> key, std::span<char, 17> out) {
    constexpr std::array<uint8_t, 15> domain{'H', 'O', 'U', 'N', 'D', '-', 'I', 'G',
                                             'N', 'O', 'R', 'E', '-', 'I', 'D'};
    std::array<uint8_t, 32> mac{};
    if (!key_ok(key) || !hmac_sha256(key, domain, mac))
        return false;
    constexpr char hex[] = "0123456789ABCDEF";
    for (size_t i = 0; i < 8; ++i) {
        out[2 * i] = hex[mac[i] >> 4];
        out[2 * i + 1] = hex[mac[i] & 15];
    }
    out[16] = 0;
    return true;
}
bool read_ignore_backup(const char *base, std::span<const uint8_t, 32> key, IgnoreList &out,
                        uint64_t &generation) {
    IgnoreList next{};
    const auto found = latest(base, key, next);
    if (found.error || found.slot < 0)
        return false;
    out = next;
    generation = found.generation;
    return true;
}
BackupResult backup_ignores(const char *base, std::span<const uint8_t, 32> key,
                            const IgnoreList &list) {
    if (!key_ok(key) || !valid_ignores(list))
        return BackupResult::Failed;
    IgnoreList previous{};
    const auto found = latest(base, key, previous);
    if (found.error || (found.slot < 0 && found.existing) ||
        found.generation == std::numeric_limits<uint64_t>::max())
        return BackupResult::Failed;
    if (found.slot >= 0 && previous == list)
        return BackupResult::Unchanged;
    Bytes bytes{};
    std::copy(magic.begin(), magic.end(), bytes.begin());
    const auto gen = found.generation + 1;
    for (unsigned i = 0; i < 8; ++i)
        bytes[8 + i] = uint8_t(gen >> (i * 8));
    for (size_t i = 0; i < list.size(); ++i) {
        std::copy(list[i].digest.begin(), list[i].digest.end(), bytes.begin() + 16 + i * 9);
        bytes[24 + i * 9] = uint8_t(list[i].category);
    }
    std::array<uint8_t, 32> mac{};
    if (!hmac_sha256(key, std::span(bytes).first(signed_size), mac))
        return BackupResult::Failed;
    std::copy(mac.begin(), mac.end(), bytes.begin() + signed_size);
    std::array<char, 192> name{};
    if (!path(base, found.slot == 0 ? 1 : 0, name))
        return BackupResult::Failed;
    FILE *f = std::fopen(name.data(), "wb");
    if (!f)
        return BackupResult::Failed;
    std::setvbuf(f, nullptr, _IONBF, 0);
    bool ok = std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
    if (std::fflush(f) != 0)
        ok = false;
    if (fsync(fileno(f)) != 0)
        ok = false;
    if (std::fclose(f) != 0)
        ok = false;
    return ok ? BackupResult::Written : BackupResult::Failed;
}
} // namespace sniffer::storage::detail
