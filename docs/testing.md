<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Host tests and validation

Install the [host dependencies](simulator.md), then run from the repository root:

```sh
cmake -S . -B build-host -G Ninja -DSNIFFER_HOST=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure
./build-host/fuzz_mutate 60
./build-host/ui_preview preview.ppm
```

Roadmap items 1–2 (Coverage help and manufacturer/SSID inference guards) pass all 15 current host suites and the ESP32 release build; installed on the Hosyond on 2026-09-24 with verified saved-data preservation and a successful boot/radio smoke check. See the latest entry in [VERIFICATION](VERIFICATION.md).

The earlier recorded verification includes successful release/debug/test/demo firmware builds, 206 core checks, 643 UI checks, 151 extended feature checks, 12 Python tests, 100,000 deterministic fuzz inputs, and 2,101,847 additional seeded mutation inputs. The final host test suite passed with address, undefined-behavior and leak checks enabled. See [VERIFICATION](VERIFICATION.md) for exact scope and build hashes; the GitHub Actions workflow is provided, but no remote CI result is claimed.

In a ptrace-based sandbox, LeakSanitizer may require `ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-host --output-on-failure`; retain leak detection on an ordinary host. Development packaging includes checksums, a source SPDX inventory, linked-archive evidence and retained dependency notices. Final license compatibility review remains a release gate.

For target-board checks, use the [test plan](TEST_PLAN.md) and [device worksheet](device-checklist.md). Record measurements separately from pass/fail conclusions.
