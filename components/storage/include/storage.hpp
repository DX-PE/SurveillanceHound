// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "state.hpp"
#include <atomic>
namespace sniffer::storage {
struct Health {
    std::atomic<bool> mounted{}, failed{}, ejected{}, read_only{}, sleep_failed{};
    std::atomic<bool> eject_failed{}, checking{};
    std::atomic<uint32_t> checked_rows{}, checked_files{}, manual_backups{};
    std::atomic<IgnoreBackupStatus> ignore_backup{IgnoreBackupStatus::Waiting};
    std::atomic<uint32_t> ignore_backup_count{};
    std::atomic<uint32_t> dropped{}, writes{}, errors{}, exports{}, saves{}, save_errors{};
    std::atomic<uint32_t> export_rows{}, stack_free{}, clears{};
    std::atomic<uint64_t> free_bytes{}, total_bytes{};
};
extern Health health;
bool load(State &);
bool start(const char *session);
bool append(const Detection &, const char *token, bool research, uint8_t region);
bool save(const State &, bool backup_ignores = false);
bool export_history();
bool clear_logs();
bool sleep(const State &);
bool eject();
void sound(unsigned effect);
void diagnostics(const char *text);
} // namespace sniffer::storage
