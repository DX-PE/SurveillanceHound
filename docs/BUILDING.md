<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Building

## Pinned SDK

Use a path without spaces. ESP-IDF's project path cannot contain spaces.

```sh
git clone --branch v6.0.2 --depth 1 --recursive --shallow-submodules https://github.com/espressif/esp-idf.git "$HOME/esp/esp-idf-v6.0.2"
"$HOME/esp/esp-idf-v6.0.2/install.sh" esp32
. "$HOME/esp/esp-idf-v6.0.2/export.sh"
cd /path/to/SurveillanceSniffer
idf.py build
```

CMake verifies the exact SDK commit `7101770dc6db2667b3c477cc31365dd1acd6db4e`. Its submodule revisions and `tools/tools.json` pin the compiler, controller/PHY libraries, and SDK dependencies. The observed Xtensa compiler is Espressif GCC 15.2.0 (`esp-15.2.0_20251204`). No SDK source is vendored in this repository.

Container alternative (requires Docker):

```sh
docker run --rm -v "$PWD:/project" -w /project espressif/idf:v6.0.2 idf.py build
```

All profiles use separate build and sdkconfig paths. Do not reuse a release sdkconfig for demo:

```sh
idf.py -B build-release -D SDKCONFIG=sdkconfig.release -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;profiles/release.defaults' build
idf.py -B build-debug -D SDKCONFIG=sdkconfig.debug -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;profiles/debug.defaults' build
idf.py -B build-test -D SDKCONFIG=sdkconfig.test -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;profiles/test.defaults' build
idf.py -B build-demo -D SDKCONFIG=sdkconfig.demo -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;profiles/demo.defaults' build
```

`debug` and `test` disable the main Wi-Fi IRAM optimization to leave room for instrumentation; release/demo retain it. `test` enables heap poisoning and stack checks; it is not a substitute for the device test checklist. `demo` starts synthetic animation without starting radio capture or writing observation/progress records.

## Generated data

```sh
python3 tools/compile_signatures.py
python3 tools/pack_assets.py
python3 tools/compile_signatures.py --check
python3 tools/pack_assets.py --check
```

The `.yaml` sources deliberately use YAML's JSON subset so the firmware build has no extra Python package dependency. The compiler validates every entry, including disabled rules. Experimental entries cannot be enabled or leak into the production header. `signatures.bin` is an archival artifact; runtime loading from SD is intentionally absent.

## Checks

Follow README for host setup, then:

```sh
python3 tools/check_static.py
ruff check tools tests/host --select F
ruff format --check tools tests/host
find main components tools tests -type f \( -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format --dry-run --Werror
cmake -S . -B build-analysis -G Ninja -DSNIFFER_HOST=ON -DSNIFFER_SANITIZE=OFF -DCMAKE_CXX_FLAGS=-fanalyzer
cmake --build build-analysis
./build-host/fuzz_mutate 60
```

The SDK's ESP32 NimBLE header unconditionally enables host privacy helpers. In this exact version, compiling only the observer role leaves `ble_sm_alg_encrypt` unresolved; enabling Central with its default GATT client provides the SDK helper. The application only calls passive discovery. Peripheral and broadcaster roles are disabled; the application has no connection or GATT operation calls. `check_static.py` guards these boundaries. Do not claim absence of transmit-capable code inside the vendor SDK.

## Passive-scanner memory budget

The default NimBLE MSYS-1, MSYS-2 and ACL data pools are limited to four blocks each because this application performs no connections, GATT requests or writes. Advertising reception keeps the SDK's eight advertising-event buffers and 30 high-priority event buffers; the Wi-Fi and raw-observation queue capacities are unchanged. Revisit these limits before adding any connecting role. When using an older generated `sdkconfig`, apply the new counts there or regenerate it from the checked-in defaults; IDF does not overwrite existing configuration choices automatically.

The eight-entry storage queue carries small tagged payloads. Up to two separately reserved state snapshots cover saves/sleep, including a snapshot currently being persisted. A queued snapshot is immutable until the worker finishes; exhausted slots or a full queue return failure, and a failed enqueue releases its reservation. Queue payloads are statically checked as trivially copyable, with no heap-owned objects. Persisted NVS record formats, storage stack allocation and SD behavior are unchanged.

The ESP32 Bluetooth controller also reserves only one connection (its minimum), matching the host setting instead of retaining the controller default of three. The Central/GATT compatibility workaround described above remains enabled, and advertising/radio receive buffers retain their existing capacities. The SDK describes each controller connection reservation as 1 KB of DRAM.

The 128-identity classifier cache stores nine independent evidence groups: exact/prefix SSID rules share a group, as do exact/prefix name rules. A zero sighting count marks an unused slot. This removes two unused evidence slots and redundant occupancy padding per identity, reducing the ESP32 Engine object from 29,072 to 26,000 bytes without reducing cache capacity or changing confidence rules. SD log stdio buffering is 512 bytes instead of 2,048; the FatFs sector cache and five-second/ten-record flush-and-fsync policy remain. These savings target SD-enabled heap headroom; current/minimum heap and export peaks still need measurement on the board. The 80,000-byte warning threshold remains unchanged.

## Development artifacts

```sh
SOURCE_DATE_EPOCH=1790035200 python3 tools/make_release.py --development --build build --output dist/0.1.0-dev
```

This packages application/bootloader/partition binaries, a flash command, SHA-256 checksums and SPDX source inventory, plus linked-archive/member hashes and SDK/runtime notices in `BINARY-EVIDENCE.zip`. The inventory identifies the SDK as a pinned package; it does not certify the transitive binary license audit. Packaging without `--development` remains blocked until the release acceptance record is approved. Nothing is pushed, published, or tagged.

The production NVS record writer/loader is isolated in `components/storage/state_store.cpp`; firmware and the host `storage_state` target compile the same implementation. The host target supplies a small NVS fake and exercises immediate writes (the pinned SDK's current behavior) as well as buffered commits. It checks complete-generation recovery at API boundaries; it does not simulate SDK flash-page internals. Persisted record layouts remain byte-for-byte compatible with the earlier firmware.

Detection events are volatile C++ objects serialized to JSON field by field. Their aligned members are grouped to keep `Detection` at 80 bytes on ESP32 (a compile-time budget prevents padding regressions). This recovers 2,608 static DRAM bytes across the engine and UI compared with the first Samsung payload-ID build, retaining all 128 cache entries, 32 history entries and Samsung metadata. No persisted NVS structure changes. Reception buffers, stacks and the 80,000-byte warning threshold are unchanged.


Idle display preferences use appearance record version 3, retaining the 12-byte record size. The loader explicitly migrates V1/V2 appearance records, ignores old padding and supplies 2/5/15-minute defaults. Other NVS record layouts are unchanged. The production persistence host tests cover legacy envelope CRCs, dirty padding, saved timeout round trips, invalid timeout/version rejection and interrupted saves. Older firmware that only understands appearance V2 cannot read a migrated V3 save: retain the pre-update NVS backup when downgrading.

## Ignore list format and backup memory

Companion V2 replaces 16 padded 16-byte ignores with 64 nine-byte entries (eight digest bytes plus category). Byte storage avoids unaligned 64-bit loads. Companion grows from 728 to 1,048 bytes; the five existing copies add 1,600 static bytes for the larger list. The frozen V1 loader validates and migrates all identities, discovery records and outfit state. Other persisted records and the partition table are unchanged. Older firmware cannot read Companion V2; retain the pre-update NVS backup for downgrades.

SD backup serialization uses an explicit fixed 624-byte little-endian format with HMAC-SHA256 and alternating files. The worker keeps only a 32-byte change digest, never a second live ignore table; unchanged successful backups skip card I/O. No new task, heap-owned list, framebuffer or storage snapshot is introduced. The validated read primitive is available for development recovery; no boot-time import or user-facing restore control is provided. See [format and recovery constraints](IGNORE_STORAGE.md).

SD history recovery is a portable incremental reader (`history_recovery.cpp`), shared with the host fault-injection target. It advances by a fixed work budget and retains reader/directory state on the existing storage task stack. Between batches the worker processes NVS and Eject, reusing one of the two snapshot slots for the newest deferred backup; replacing or cancelling it releases the old lease. There is no third saved-state copy or new stack allocation. Radio records during SD CHECK are not queued and are counted as dropped; normal card logging starts after validation. The 64-slot NVS and 624-byte SD backup formats are unchanged.
