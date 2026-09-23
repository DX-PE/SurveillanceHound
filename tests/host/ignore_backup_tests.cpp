// SPDX-License-Identifier: Apache-2.0
#include "ignore_backup.hpp"
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
using namespace sniffer;
using namespace sniffer::storage::detail;
namespace fs = std::filesystem;
namespace {
unsigned checks{};
int fault{};
FILE *writer{};
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::cerr << __LINE__ << ": " #x "\n";                                                 \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)
std::string bytes(const fs::path &p) {
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}
void write(const fs::path &p, const std::string &s) {
    std::ofstream f(p, std::ios::binary);
    f.write(s.data(), s.size());
}
} // namespace
extern "C" FILE *__real_fopen(const char *, const char *);
extern "C" int __real_fclose(FILE *);
extern "C" int __real_fsync(int);
extern "C" int __real_fflush(FILE *);
extern "C" size_t __real_fwrite(const void *, size_t, size_t, FILE *);
extern "C" FILE *__wrap_fopen(const char *p, const char *m) {
    if (fault == 1 && m[0] == 'w') {
        errno = EIO;
        return nullptr;
    }
    if (fault == 6 && m[0] == 'r') {
        errno = EIO;
        return nullptr;
    }
    auto *f = __real_fopen(p, m);
    if (m[0] == 'w')
        writer = f;
    return f;
}
extern "C" int __wrap_fclose(FILE *f) {
    bool failed = fault == 5 && f == writer;
    if (f == writer)
        writer = nullptr;
    int rc = __real_fclose(f);
    return failed ? EOF : rc;
}
extern "C" int __wrap_fflush(FILE *f) {
    return fault == 3 ? EOF : __real_fflush(f);
}
extern "C" int __wrap_fsync(int fd) {
    return fault == 4 ? -1 : __real_fsync(fd);
}
extern "C" size_t __wrap_fwrite(const void *p, size_t size, size_t n, FILE *f) {
    return __real_fwrite(p, size, fault == 2 ? n / 2 : n, f);
}
int main() {
    auto dir = fs::temp_directory_path() / ("hound-ignore-test-" + std::to_string(getpid()));
    fs::create_directories(dir);
    auto base = (dir / "board").string();
    auto slot0 = base + "-0.BIN", slot1 = base + "-1.BIN";
    std::array<uint8_t, 32> key{}, other{};
    key.fill(0xa5);
    other.fill(0x3c);
    std::array<char, 17> id{}, other_id{};
    CHECK(ignore_backup_id(key, id) && ignore_backup_id(other, other_id) && id != other_id);
    IgnoreList list{}, loaded{};
    uint64_t generation = 0;
    for (size_t i = 0; i < list.size(); ++i)
        list[i] = {0xfedcba9876540000ULL + i, Category(i % category_count)};
    CHECK(backup_ignores(base.c_str(), key, list) == BackupResult::Written);
    CHECK(fs::file_size(slot0) == ignore_backup_bytes && !fs::exists(slot1));
    const auto original = bytes(slot0);
    CHECK(original.find(std::string(32, char(0xa5))) == std::string::npos);
    CHECK(read_ignore_backup(base.c_str(), key, loaded, generation));
    CHECK(loaded == list && generation == 1);
    CHECK(backup_ignores(base.c_str(), key, list) == BackupResult::Unchanged);
    CHECK(!fs::exists(slot1) && bytes(slot0) == original);
    CHECK(!read_ignore_backup(base.c_str(), other, loaded, generation));
    CHECK(backup_ignores(base.c_str(), other, list) == BackupResult::Failed);
    CHECK(bytes(slot0) == original);
    auto next = list;
    next[0] = {};
    next[63] = {12345, Category::AIRTAG};
    for (int failure = 1; failure <= 6; ++failure) {
        fs::remove(slot1);
        fault = failure;
        CHECK(backup_ignores(base.c_str(), key, next) == BackupResult::Failed);
        fault = 0;
        CHECK(bytes(slot0) == original); // Last good generation never opened for writing.
        CHECK(read_ignore_backup(base.c_str(), key, loaded, generation));
        CHECK(loaded == list || loaded == next); // A reported close/sync failure can still persist.
    }
    fs::remove(slot1);
    CHECK(backup_ignores(base.c_str(), key, next) == BackupResult::Written);
    const auto newest = bytes(slot1);
    CHECK(read_ignore_backup(base.c_str(), key, loaded, generation) && generation == 2 &&
          loaded == next);
    // Every possible torn length, including header and MAC; older copy remains readable.
    for (size_t n = 0; n < newest.size(); ++n) {
        write(slot1, newest.substr(0, n));
        CHECK(read_ignore_backup(base.c_str(), key, loaded, generation) && generation == 1 &&
              loaded == list);
    }
    for (size_t i = 0; i < newest.size(); ++i) {
        auto corrupt = newest;
        corrupt[i] ^= 0x40;
        write(slot1, corrupt);
        CHECK(read_ignore_backup(base.c_str(), key, loaded, generation) && generation == 1 &&
              loaded == list);
    }
    write(slot1, newest + "X");
    CHECK(read_ignore_backup(base.c_str(), key, loaded, generation) && generation == 1);
    write(slot1, newest);
    // Empty list is a real deletion snapshot, not a request to import the older copy.
    CHECK(backup_ignores(base.c_str(), key, {}) == BackupResult::Written);
    CHECK(read_ignore_backup(base.c_str(), key, loaded, generation) && generation == 3 &&
          loaded == IgnoreList{});
    auto duplicate = list;
    duplicate[1] = duplicate[0];
    CHECK(backup_ignores(base.c_str(), key, duplicate) == BackupResult::Failed);
    duplicate = list;
    duplicate[3].category = Category(255);
    CHECK(backup_ignores(base.c_str(), key, duplicate) == BackupResult::Failed);
    CHECK(backup_ignores(base.c_str(), std::array<uint8_t, 32>{}, list) == BackupResult::Failed);
    write(slot0, "broken");
    write(slot1, "broken");
    CHECK(!read_ignore_backup(base.c_str(), key, loaded, generation));
    CHECK(backup_ignores(base.c_str(), key, list) == BackupResult::Failed);
    CHECK(bytes(slot0) == "broken" && bytes(slot1) == "broken");
    fs::remove_all(dir);
    std::cout << checks << " ignore backup checks passed\n";
}
