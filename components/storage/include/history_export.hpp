// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdint>
#include <cstdio>
#include <span>
namespace sniffer::storage::detail {
using HashFile = bool (*)(const char *, char *);
// Always release the stream; report any flush, sync or close failure.
bool close_log(FILE *&writer);
// Called only by the storage worker; it owns the writer and serializes queued records.
// Flush and close the active log before reading history, then reopen it without creating
// or truncating it. A failed export never carries a completion manifest.
bool export_history_files(FILE *&writer, const char *writer_path, const char *logs_path,
                          const char *directory, std::span<const uint8_t> export_key,
                          std::span<char, 2048> scratch, uint32_t &rows, HashFile hash,
                          void (*checkpoint)());
} // namespace sniffer::storage::detail
