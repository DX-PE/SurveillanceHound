// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "state.hpp"
#include <atomic>
namespace sniffer::storage {
struct Health {
    std::atomic<bool> mounted{}, failed{}, ejected{}, read_only{}, sleep_failed{};
    std::atomic<uint32_t> dropped{}, writes{}, errors{}, exports{}, save_errors{};
    std::atomic<uint32_t> export_rows{}, stack_free{}, clears{};
    std::atomic<uint64_t> free_bytes{}, total_bytes{};
};
extern Health health;
bool load(State &);
bool start(const char *session);
bool append(const Detection &, const char *token, bool research, uint8_t region);
bool save(const State &);
bool export_history();
bool clear_logs();
bool sleep(const State &);
bool eject();
void sound(unsigned effect);
void diagnostics(const char *text);
} // namespace sniffer::storage
