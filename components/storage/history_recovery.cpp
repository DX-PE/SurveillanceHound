// SPDX-License-Identifier: Apache-2.0
#include "history_recovery.hpp"
#include "sniffer/core.hpp"
#include <cerrno>
#include <cstring>
#include <unistd.h>
namespace sniffer::storage::detail {
bool HistoryRecovery::begin(const char *directory) {
    if (active() || input_)
        return false;
    root_ = directory;
    directory_ = opendir(root_);
    if (!directory_)
        error();
    return directory_;
}
bool HistoryRecovery::close_input() {
    if (input_) {
        if (fclose(input_) != 0) {
            close_failed_ = true;
            error();
        }
        input_ = nullptr;
    }
    return !close_failed_;
}
bool HistoryRecovery::cancel() {
    close_input();
    if (directory_) {
        if (closedir(directory_) != 0) {
            close_failed_ = true;
            error();
        }
        directory_ = nullptr;
    }
    return !close_failed_;
}
void HistoryRecovery::finish_file(bool partial) {
    if (ferror(input_))
        error();
    close_input();
    if (!partial || read_only_)
        return;
    // Retain complete validated records; remove only an incomplete trailing row.
    input_ = fopen(path_.data(), "rb+");
    if (!input_) {
        error();
        return;
    }
    if (ftruncate(fileno(input_), good_) != 0 || fsync(fileno(input_)) != 0) {
        close_failed_ = true; // A failed tail repair cannot be acknowledged as a safe eject.
        error();
    }
    close_input();
}
void HistoryRecovery::step(std::span<char, 2048> scratch, unsigned budget) {
    while (active() && budget--) {
        if (!input_) {
            errno = 0;
            auto *entry = readdir(directory_);
            if (!entry) {
                if (errno)
                    error();
                cancel();
                return;
            }
            const std::string_view name(entry->d_name);
            if (!name.ends_with(".JSONL") || name.find('/') != name.npos)
                continue;
            if (++files_ > 4096) {
                error();
                cancel();
                return;
            }
            int n = std::snprintf(path_.data(), path_.size(), "%s/%s", root_, entry->d_name);
            if (n <= 0 || size_t(n) >= path_.size()) {
                error();
                continue;
            }
            input_ = fopen(path_.data(), "rb");
            if (!input_) {
                error();
                continue;
            }
            setvbuf(input_, nullptr, _IOFBF, 512);
            good_ = 0;
            continue;
        }
        if (!fgets(scratch.data(), scratch.size(), input_)) {
            finish_file(false);
            continue;
        }
        size_t n = std::strlen(scratch.data());
        if (!n || scratch[n - 1] != '\n') {
            bool partial = feof(input_);
            if (!partial)
                error();
            finish_file(partial);
            continue;
        }
        if (!valid_log_record({scratch.data(), n})) {
            error(); // A complete corrupt row is never truncated or rewritten.
            finish_file(false);
            continue;
        }
        good_ = ftell(input_);
        if (good_ < 0) {
            error();
            finish_file(false);
            continue;
        }
        ++rows_;
    }
}
} // namespace sniffer::storage::detail
