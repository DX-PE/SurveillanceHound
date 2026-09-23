// SPDX-License-Identifier: Apache-2.0
#include "history_export.hpp"
#include "signatures_generated.h"
#include "sniffer/core.hpp"
#include <array>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
namespace sniffer::storage::detail {
namespace {
bool write_all(FILE *f, const char *text, size_t len) {
    return f && fwrite(text, 1, len, f) == len;
}
bool finish(FILE *f) {
    bool ok = fflush(f) == 0;
    ok = (fsync(fileno(f)) == 0) && ok;
    return (fclose(f) == 0) && ok;
}
bool write_export(const char *logs_path, const char *dir, std::span<const uint8_t> export_key,
                  std::span<char, 2048> line, uint32_t &rows, HashFile hash, void (*checkpoint)()) {
    char path[220];
    std::snprintf(path, sizeof(path), "%s/SIGHTINGS.JSONL", dir);
    FILE *out = fopen(path, "wx");
    bool ok = out;
    DIR *logs = opendir(logs_path);
    if (!logs)
        ok = false;
    std::array<char, 2048> exported{};
    unsigned files = 0;
    if (logs)
        while (ok) {
            errno = 0;
            auto *e = readdir(logs);
            if (!e) {
                if (errno)
                    ok = false;
                break;
            }
            std::string_view name(e->d_name);
            if (!name.ends_with(".JSONL") || name.find('/') != name.npos)
                continue;
            if (++files > 4096) {
                ok = false;
                break;
            }
            char source[300];
            std::snprintf(source, sizeof(source), "%s/%s", logs_path, e->d_name);
            FILE *input = fopen(source, "rb");
            if (!input) {
                ok = false;
                break;
            }
            while (ok && fgets(line.data(), line.size(), input)) {
                checkpoint();
                size_t len = std::strlen(line.data());
                if (!len || line[len - 1] != '\n') {
                    ok = false;
                    break;
                }
                auto n = export_log_record({line.data(), len}, export_key, exported);
                ok = n && write_all(out, exported.data(), n);
                if (ok)
                    ++rows;
                checkpoint();
            }
            if (ferror(input))
                ok = false;
            ok = (fclose(input) == 0) && ok;
        }
    if (logs)
        ok = (closedir(logs) == 0) && ok;
    if (out)
        ok = finish(out) && ok;
    const char *names[] = {"SIGHTINGS.JSONL", "SUMMARY.JSON", "README.TXT"};
    const char *readme =
        "Surveillance Hound complete historical event export. Repeated updates are retained.\n"
        "No location is collected; not map-ready. No raw MAC, SSID, payload, or drone serial.\n"
        "Identifiers are rekeyed per export. Host export tool can aggregate events.\n"
        "Radio clues may be spoofed. Confidence is evidence strength, not proof of surveillance.\n";
    std::snprintf(path, sizeof(path), "%s/README.TXT", dir);
    out = fopen(path, "wx");
    ok = write_all(out, readme, std::strlen(readme)) && ok;
    if (out)
        ok = finish(out) && ok;
    std::snprintf(path, sizeof(path), "%s/SUMMARY.JSON", dir);
    out = fopen(path, "wx");
    int n = std::snprintf(
        line.data(), line.size(),
        "{\"schema_version\":1,\"scope\":\"full_history_events\",\"file_limit\":4096,\"rows\":%lu,"
        "\"firmware_version\":\"%s\",\"hardware\":\"E32R40T\",\"signature_pack\":\"%s\"}\n",
        static_cast<unsigned long>(rows), firmware_version, signature_pack_version);
    ok = write_all(out, line.data(), n) && ok;
    if (out)
        ok = finish(out) && ok;
    std::snprintf(path, sizeof(path), "%s/MANIFEST.SHA256", dir);
    FILE *manifest = ok ? fopen(path, "wx") : nullptr;
    ok = bool(manifest) && ok;
    for (auto name : names) {
        if (!ok)
            break;
        std::snprintf(path, sizeof(path), "%s/%s", dir, name);
        char digest[65]{};
        if (!hash(path, digest)) {
            ok = false;
            break;
        }
        if (manifest && std::fprintf(manifest, "%s  %s\n", digest, name) < 0)
            ok = false;
    }
    if (manifest)
        ok = finish(manifest) && ok;
    return ok;
}
} // namespace
bool close_log(FILE *&writer) {
    if (!writer)
        return true;
    bool ok = finish(writer);
    writer = nullptr;
    return ok;
}
bool export_history_files(FILE *&writer, const char *writer_path, const char *logs_path,
                          const char *directory, std::span<const uint8_t> export_key,
                          std::span<char, 2048> scratch, uint32_t &rows, HashFile hash,
                          void (*checkpoint)()) {
    rows = 0;
    if (!writer)
        return false;
    // FatFs FF_FS_LOCK rejects opening a reader while this file has a writer.
    // Closing also releases its file/sector buffers during the export.
    bool closed = close_log(writer);
    if (!closed)
        return false;
    bool created = mkdir(directory, 0755) == 0;
    bool ok =
        created && write_export(logs_path, directory, export_key, scratch, rows, hash, checkpoint);
    // r+b must fail if the log vanished: never recreate or truncate saved history.
    writer = fopen(writer_path, "r+b");
    if (writer) {
        setvbuf(writer, nullptr, _IOFBF, 512);
        if (fseek(writer, 0, SEEK_END) != 0) {
            fclose(writer);
            writer = nullptr;
        }
    }
    ok = bool(writer) && ok;
    if (!ok && created) {
        char manifest[220];
        std::snprintf(manifest, sizeof(manifest), "%s/MANIFEST.SHA256", directory);
        unlink(manifest);
    }
    return ok;
}
} // namespace sniffer::storage::detail
