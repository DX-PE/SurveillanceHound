// SPDX-License-Identifier: Apache-2.0
#include "history_export.hpp"
#include "signatures_generated.h"
#include "sniffer/core.hpp"
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace sniffer;
namespace {
unsigned checks{}, lock_rejections{}, checkpoints{};
#define CHECK(test)                                                                                \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(test)) {                                                                             \
            std::cerr << "line " << __LINE__ << ": " << #test << '\n';                             \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)
struct OpenFile {
    std::string path;
    bool writer;
};
std::map<FILE *, OpenFile> handles;
bool fail_hash{}, fail_resume{}, fail_sync{}, fail_flush{}, fail_close{};
std::string current_log, sync_fail_path;
std::string read(const fs::path &path) {
    std::ifstream stream(path);
    return {std::istreambuf_iterator<char>(stream), {}};
}
bool hash_file(const char *path, char *out) {
    if (fail_hash)
        return false;
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    std::string data;
    std::array<char, 1024> buf{};
    size_t n;
    while ((n = fread(buf.data(), 1, buf.size(), f)))
        data.append(buf.data(), n);
    bool ok = !ferror(f);
    ok = (fclose(f) == 0) && ok;
    std::array<uint8_t, 32> digest{};
    if (!ok || !sha256({reinterpret_cast<const uint8_t *>(data.data()), data.size()}, digest))
        return false;
    for (size_t i = 0; i < digest.size(); ++i)
        std::snprintf(out + 2 * i, 3, "%02x", digest[i]);
    return true;
}
void checkpoint() {
    ++checkpoints;
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
    CHECK(d.rules[0]);
    d.rule_count = 1;
    std::array<char, 2048> data{};
    auto n =
        write_record(d, "0123456789abcdef0123456789abcdef", "ABCDEFGHIJKLMNOPQRST", true, data);
    CHECK(n);
    return {data.data(), n};
}
} // namespace

// glibc permits duplicated writer/reader opens. Model the board's FF_FS_LOCK
// contract here so the production export fails if it reintroduces that sequence.
extern "C" FILE *__real_fopen(const char *, const char *);
extern "C" int __real_fclose(FILE *);
extern "C" int __real_fsync(int);
extern "C" int __real_fflush(FILE *);
extern "C" FILE *__wrap_fopen(const char *path, const char *mode) {
    bool writer = std::strpbrk(mode, "wa+");
    for (const auto &[handle, open] : handles)
        if (open.path == path && (writer || open.writer)) {
            ++lock_rejections;
            errno = EACCES;
            return nullptr;
        }
    if (fail_resume && current_log == path && std::string_view(mode) == "r+b") {
        errno = EIO;
        return nullptr;
    }
    auto *f = __real_fopen(path, mode);
    if (f)
        handles[f] = {path, writer};
    return f;
}
extern "C" int __wrap_fclose(FILE *file) {
    handles.erase(file);
    const int result = __real_fclose(file);
    return fail_close ? EOF : result;
}
extern "C" int __wrap_fflush(FILE *file) {
    const int result = __real_fflush(file);
    return fail_flush ? EOF : result;
}
extern "C" int __wrap_fsync(int fd) {
    bool targeted = false;
    for (const auto &[handle, open] : handles)
        if (fileno(handle) == fd && open.path == sync_fail_path)
            targeted = true;
    if (fail_sync || targeted) {
        errno = EIO;
        return -1;
    }
    return __real_fsync(fd);
}

int main(int argc, char **argv) {
    char temp[] = "/tmp/hound-export-test-XXXXXX";
    auto *name = mkdtemp(temp);
    CHECK(name);
    fs::path root(name), logs = root / "LOGS";
    fs::create_directory(logs);
    current_log = (logs / "CURRENT.JSONL").string();
    auto record = row();
    auto prior = logs / "PRIOR.JSONL";
    {
        std::ofstream(prior) << record;
    }
    FILE *writer = fopen(current_log.c_str(), "wx");
    CHECK(writer);
    CHECK(fwrite(record.data(), 1, record.size(), writer) == record.size());
    CHECK(!fopen(current_log.c_str(), "rb")); // Reproduce the old export's forbidden open.
    CHECK(lock_rejections == 1);
    std::array<uint8_t, 32> key{};
    key[0] = 1;
    std::array<char, 2048> scratch{};
    uint32_t rows{};
    auto run = [&](const char *dir) {
        return storage::detail::export_history_files(writer, current_log.c_str(), logs.c_str(),
                                                     (root / dir).c_str(), key, scratch, rows,
                                                     hash_file, checkpoint);
    };
    CHECK(run("FIRST"));
    CHECK(rows == 2);
    CHECK(checkpoints > 0);
    CHECK(writer && handles.size() == 1);
    CHECK(lock_rejections == 1);
    CHECK(read(current_log) == record);
    CHECK(read(prior) == record);
    CHECK(read(root / "FIRST/SIGHTINGS.JSONL").find("ABCDEFGHIJKLMNOPQRST") == std::string::npos);
    CHECK(read(root / "FIRST/SUMMARY.JSON").find("\"rows\":2") != std::string::npos);
    CHECK(read(root / "FIRST/MANIFEST.SHA256").find("SIGHTINGS.JSONL") != std::string::npos);

    // A second export includes records appended after the first without truncation.
    CHECK(fwrite(record.data(), 1, record.size(), writer) == record.size());
    CHECK(run("SECOND"));
    CHECK(rows == 3);
    CHECK(read(current_log) == record + record);
    auto completed_manifest = read(root / "SECOND/MANIFEST.SHA256");
    CHECK(!run("SECOND")); // Existing export directory remains untouched.
    CHECK(read(root / "SECOND/MANIFEST.SHA256") == completed_manifest);
    CHECK(writer && handles.size() == 1);

    fail_hash = true;
    CHECK(!run("BADHASH"));
    CHECK(!fs::exists(root / "BADHASH/MANIFEST.SHA256"));
    CHECK(writer && handles.size() == 1);
    fail_hash = false;
    CHECK(run("RETRY"));
    CHECK(rows == 3);

    // Malformed history must not produce a completed export or damage other logs.
    auto bad = logs / "BAD.JSONL";
    {
        std::ofstream(bad) << "partial";
    }
    CHECK(!run("BADROW"));
    CHECK(!fs::exists(root / "BADROW/MANIFEST.SHA256"));
    CHECK(read(bad) == "partial");
    CHECK(writer && handles.size() == 1);
    fs::remove(bad);

    // Failure while syncing export content or its completion marker stays incomplete.
    for (const auto *artifact :
         {"SIGHTINGS.JSONL", "README.TXT", "SUMMARY.JSON", "MANIFEST.SHA256"}) {
        std::string folder = std::string("SYNC-") + artifact;
        sync_fail_path = (root / folder / artifact).string();
        CHECK(!run(folder.c_str()));
        CHECK(!fs::exists(root / folder / "MANIFEST.SHA256"));
        CHECK(writer && handles.size() == 1);
    }
    sync_fail_path.clear();
    CHECK(read(current_log) == record + record);

    fail_resume = true;
    CHECK(!run("BADRESUME"));
    CHECK(!writer && handles.empty());
    CHECK(!fs::exists(root / "BADRESUME/MANIFEST.SHA256"));
    CHECK(read(current_log) == record + record);
    fail_resume = false;
    writer = fopen(current_log.c_str(), "r+b");
    CHECK(writer);
    fail_sync = true;
    CHECK(!run("BADSYNC"));
    CHECK(!writer && handles.empty());
    CHECK(!fs::exists(root / "BADSYNC"));
    fail_sync = false;
    CHECK(!run("NOWRITER"));
    CHECK(!fs::exists(root / "NOWRITER"));
    CHECK(lock_rejections == 1);
    // Eject shares the production flush/sync/close path. Every failure is reported,
    // and the handle is always released; callers must retain a failed result.
    for (int fault = 0; fault < 4; ++fault) {
        writer = fopen(current_log.c_str(), "r+b");
        CHECK(writer);
        fail_flush = fault == 1;
        fail_sync = fault == 2;
        fail_close = fault == 3;
        CHECK(storage::detail::close_log(writer) == (fault == 0));
        CHECK(!writer && handles.empty());
        fail_flush = fail_sync = fail_close = false;
    }
    CHECK(storage::detail::close_log(writer));
    if (argc > 1) {
        fs::create_directories(argv[1]);
        fs::copy(root / "SECOND", fs::path(argv[1]) / "export", fs::copy_options::recursive);
    }
    fs::remove_all(root);
    std::cout << checks << " storage export checks passed\n";
}
