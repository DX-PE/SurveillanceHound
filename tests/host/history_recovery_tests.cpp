// SPDX-License-Identifier: Apache-2.0
#include "history_recovery.hpp"
#include "signatures_generated.h"
#include "sniffer/core.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <unistd.h>
using namespace sniffer;
using namespace sniffer::storage::detail;
namespace fs = std::filesystem;
namespace {
unsigned checks{}, repairs{};
bool fail_close{}, fail_sync{}, fail_truncate{};
std::set<FILE *> handles;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::cerr << __LINE__ << ": " #x "\n";                                                 \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)
std::string read(const fs::path &p) {
    std::ifstream f(p);
    return {std::istreambuf_iterator<char>(f), {}};
}
void write(const fs::path &p, const std::string &s) {
    std::ofstream f(p);
    f << s;
}
std::string row() {
    Detection d{};
    d.category = Category::FLIPPER;
    d.radio = Radio::Ble;
    d.score = 75;
    d.first_ms = 1;
    d.last_ms = 1000;
    d.seen_count = 1;
    d.rssi_min = d.rssi_max = d.rssi_sum = -50;
    for (const auto &rule : production_rules)
        if (std::string_view(rule.id) == "ble.flipper.3081")
            d.rules[0] = &rule;
    d.rule_count = 1;
    std::array<char, 2048> out{};
    auto size =
        write_record(d, "0123456789abcdef0123456789abcdef", "ABCDEFGHIJKLMNOPQRST", false, out);
    CHECK(size && valid_log_record({out.data(), size}));
    return {out.data(), size};
}
} // namespace
extern "C" FILE *__real_fopen(const char *, const char *);
extern "C" int __real_fclose(FILE *);
extern "C" int __real_fsync(int);
extern "C" int __real_ftruncate(int, off_t);
extern "C" FILE *__wrap_fopen(const char *p, const char *mode) {
    auto *f = __real_fopen(p, mode);
    if (f)
        handles.insert(f);
    return f;
}
extern "C" int __wrap_fclose(FILE *f) {
    CHECK(handles.erase(f) == 1);
    int rc = __real_fclose(f);
    return fail_close ? EOF : rc;
}
extern "C" int __wrap_fsync(int fd) {
    return fail_sync ? -1 : __real_fsync(fd);
}
extern "C" int __wrap_ftruncate(int fd, off_t n) {
    ++repairs;
    return fail_truncate ? -1 : __real_ftruncate(fd, n);
}
int main() {
    auto dir = fs::temp_directory_path() / ("hound-recovery-test-" + std::to_string(getpid()));
    fs::create_directories(dir);
    const auto directory = dir.string();
    const auto log = dir / "history.JSONL";
    const auto complete = row();
    std::string history;
    for (unsigned i = 0; i < 4000; ++i)
        history += complete;
    const auto partial = history + "{\"schema_version\":1,";
    std::array<char, 2048> scratch{};
    write(log, partial);
    {
        HistoryRecovery recovery;
        CHECK(recovery.begin(directory.c_str()));
        unsigned batches = 0;
        while (recovery.active()) {
            auto previous = recovery.rows();
            recovery.step(scratch, 16);
            CHECK(recovery.rows() - previous <=
                  16); // Always returns control to Save/Eject handling.
            CHECK(++batches < 270);
        }
        CHECK(batches > 250 && recovery.rows() == 4000 && recovery.files() == 1);
        CHECK(!recovery.read_only() && !recovery.errors());
        CHECK(read(log) == history && repairs == 1 && handles.empty());
    }
    write(log, partial);
    {
        HistoryRecovery recovery;
        CHECK(recovery.begin(directory.c_str()));
        recovery.step(scratch, 16);
        CHECK(recovery.active() && recovery.rows() > 0 && recovery.rows() <= 16);
        CHECK(recovery.cancel()); // Eject during the initial scan closes every reader.
        CHECK(!recovery.active() && handles.empty() && read(log) == partial);
        auto rows = recovery.rows();
        recovery.step(scratch, 16);
        CHECK(recovery.rows() == rows && read(log) == partial && repairs == 1);
        CHECK(recovery.cancel());
    }
    for (auto content : {complete + "CORRUPT\n", std::string(3000, 'X') + '\n'}) {
        write(log, content);
        HistoryRecovery recovery;
        CHECK(recovery.begin(directory.c_str()));
        while (recovery.active())
            recovery.step(scratch);
        CHECK(recovery.read_only() && recovery.errors() && read(log) == content);
        CHECK(handles.empty()); // Complete corrupt records are retained for investigation.
    }
    for (unsigned failure = 0; failure < 3; ++failure) {
        write(log, complete + "partial");
        fail_close = failure == 0;
        fail_sync = failure == 1;
        fail_truncate = failure == 2;
        HistoryRecovery recovery;
        CHECK(recovery.begin(directory.c_str()));
        while (recovery.active())
            recovery.step(scratch);
        CHECK(recovery.read_only() && recovery.errors() && !recovery.cancel());
        CHECK(handles.empty());
        fail_close = fail_sync = fail_truncate = false;
    }
    fs::remove(log);
    {
        HistoryRecovery recovery;
        CHECK(recovery.begin(directory.c_str()));
        while (recovery.active())
            recovery.step(scratch, 1);
        CHECK(!recovery.read_only() && !recovery.rows() && !recovery.files());
    }
    fs::remove_all(dir);
    {
        HistoryRecovery recovery;
        CHECK(!recovery.begin(directory.c_str()));
        CHECK(recovery.read_only() && !recovery.active());
    }
    CHECK(handles.empty());
    std::cout << checks << " incremental recovery and cancellation checks passed\n";
}
