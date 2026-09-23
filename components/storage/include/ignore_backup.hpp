// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "sniffer/companion.hpp"
namespace sniffer::storage::detail {
// Explicit little-endian format: magic(8), generation(8), 64 x (digest(8), category(1)),
// HMAC-SHA256(32). Authenticated with the board's existing identity key; never exports it.
constexpr size_t ignore_backup_bytes = 16 + ignore_capacity * 9 + 32;
enum class BackupResult { Failed, Unchanged, Written };
bool ignore_backup_id(std::span<const uint8_t, 32> key, std::span<char, 17> out);
// base includes directory and board ID; alternating -0.BIN / -1.BIN files retain
// the previous valid generation if writing the next file is interrupted.
BackupResult backup_ignores(const char *base, std::span<const uint8_t, 32> key,
                            const IgnoreList &list);
// Validated recovery primitive. Normal boot does not import SD state over NVS.
bool read_ignore_backup(const char *base, std::span<const uint8_t, 32> key, IgnoreList &out,
                        uint64_t &generation);
} // namespace sniffer::storage::detail
