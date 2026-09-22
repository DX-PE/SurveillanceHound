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

## Development artifacts

```sh
SOURCE_DATE_EPOCH=1790035200 python3 tools/make_release.py --development --build build --output dist/0.1.0-dev
```

This packages application/bootloader/partition binaries, a flash command, SHA-256 checksums and SPDX source inventory, plus linked-archive/member hashes and SDK/runtime notices in `BINARY-EVIDENCE.zip`. The inventory identifies the SDK as a pinned package; it does not certify the transitive binary license audit. Packaging without `--development` remains blocked until the release acceptance record is approved. Nothing is pushed, published, or tagged.
