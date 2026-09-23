// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <dirent.h>
#include <span>
namespace sniffer::storage::detail {
// Incremental full validation, with at most budget lines/directory entries per step.
// The worker can service NVS saves and Eject between steps. No growing allocation.
class HistoryRecovery {
    const char *root_{};
    DIR *directory_{};
    FILE *input_{};
    std::array<char, 300> path_{};
    long good_{};
    bool read_only_{}, close_failed_{};
    uint32_t files_{}, rows_{}, errors_{};
    void error() {
        read_only_ = true;
        ++errors_;
    }
    bool close_input();
    void finish_file(bool partial);

  public:
    HistoryRecovery() = default;
    HistoryRecovery(const HistoryRecovery &) = delete;
    HistoryRecovery &operator=(const HistoryRecovery &) = delete;
    ~HistoryRecovery() {
        cancel();
    }
    bool begin(const char *directory);
    void step(std::span<char, 2048> scratch, unsigned budget = 16);
    bool cancel(); // Closes reader/directory; never repairs a file during cancellation.
    bool active() const {
        return directory_ != nullptr;
    }
    bool read_only() const {
        return read_only_;
    }
    uint32_t files() const {
        return files_;
    }
    uint32_t rows() const {
        return rows_;
    }
    uint32_t errors() const {
        return errors_;
    }
};
} // namespace sniffer::storage::detail
