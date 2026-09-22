<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Surveillance Hound

An offline ESP32 radio-observation pet for the **LCDWiki E32R40T**: six dogs, passive Wi-Fi/Bluetooth clues, and local logs. This is a **0.1.0 development build**. The ESP32-C3 radio test has passed; the display board's physical acceptance tests remain pending.

![Firmware UI rendered on the host](docs/ui-preview.png)

[Sleeping hound and settings preview](docs/ui-features.png)

The application uses passive Wi-Fi management reception and passive legacy BLE scanning. It does not associate, probe, advertise, inject frames, or upload observations. A match describes radio evidence, not intent or proof of surveillance.

## Current features

- **Six original dogs:** Corgi, Beagle, Shiba, Husky, Labrador and Dalmatian, with 33 animation frames each, naming, pet progress, touch reactions and level-up celebrations.
- **Landscape by default:** 480 × 320, with a 320 × 480 portrait layout and persistent rotation lock. Rotation is manual; the board has no orientation sensor.
- **Start/Stop sniffing:** the home-screen button pauses reception and puts the hound to sleep with floating Zs. Start wakes the hound and resumes reception. Both button labels fit within the display.
- **Snacks for all 19 categories:** the hound turns sideways, walks up, takes progressive bites and finishes happy. Reduced-animation mode is available.
- **67 enabled rules:** Flock, Axon, glasses, skimmer modules, Raven, Apple Find My/AirTag, Tile, Samsung Tag, Google Tag, Drone, ALPR, Camera, Ring, iBeacon, Flipper, Pwnagotchi, Pineapple, deauth bursts and possible evil twins. Vendor, name and generic-service clues retain conservative confidence limits; see [detection coverage and limitations](docs/DETECTIONS.md).
- **Bounded detection:** 128-entry observation and meal caches, confidence fusion, cooldowns, probe-request parsing, 128-bit BLE UUIDs, structured uncompressed Pwnagotchi advertisements, legacy Remote ID assembly and corroborated Wi-Fi security differences.
- **Evidence and controls:** 32 recent events paged four at a time, frozen evidence details, separate detector and alert switches, and volatile Remote ID details when received.
- **Local storage and privacy:** private HMAC identifiers by default, opt-in raw local MAC logging, rotated JSONL, complete-row validation, incomplete-tail recovery, corrupt-log write lockout, full-history export and confirmed log deletion.
- **Device settings:** manual UTC, three-point battery calibration, optional voltage/estimated percentage, critical save/flush/sleep, brightness and muted-by-default audio effects.
- **Diagnostics:** per-radio accepted/malformed counts, queue drops, task watermarks, heap, SD space, raw ADC and display/touch self-test.

Firmware uses native **ESP-IDF 6.0.2**, pinned to `7101770dc6db2667b3c477cc31365dd1acd6db4e`, a C++20 portable core, and project-owned ST7796 display/XPT2046 touch drivers. Release, debug, test and demo build profiles are included.

## Run the local simulator

Requirements: Python 3.11+, CMake 3.22+, Ninja, a C++20 compiler and OpenSSL development headers. Debian/Ubuntu packages: `build-essential cmake ninja-build libssl-dev python3-venv`.

From the repository root:

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements-dev.txt
python3 tools/run_local.py
```

Open [http://127.0.0.1:8765/](http://127.0.0.1:8765/). The script builds the native firmware UI and serves it on loopback. Click the device screen, choose a hound, try onboarding, or use the sample selector to preview any category's snack. Start sniffing before sending samples if the hound is asleep.

The browser preview uses **synthetic events and RAM-only state**, even when a C3 is connected over USB. It does not display live USB radio data, write SD logs or award saved XP. Restarting the simulator resets the session. Press Ctrl+C to stop, or use `--port 8766` for another port.

| Control | Behavior |
|---|---|
| Home → Start/Stop sniffing | Pause/resume scanning; show the sleeping hound while stopped |
| Settings → Rotation lock / View | Unlock, choose landscape or portrait, then lock the chosen orientation |
| Settings → Alert types | Toggle pop-ups and sounds per category while retaining sightings; the confidence threshold still applies |
| Detectors | Enable or disable observation processing for each category |
| Settings → More, page 5 | Set UTC, calibrate battery, clear SD logs, run self-test or export history |

Manual UTC is marked as manual in records and resets after power loss. Battery mode defaults off; voltage and estimated percentage require a valid, explicitly enabled calibration against a meter. Power thresholds still require validation with the actual board and battery.

## Build and flash the display board

Use the pinned SDK and a repository path without spaces. After activating ESP-IDF:

```sh
idf.py build
./tools/flash.sh /dev/ttyUSB0
```

Replace the serial port with the display board's port. See [BUILDING](docs/BUILDING.md) for SDK installation, the four profiles, Docker, and development packaging. Start bring-up on USB power. The E32R40T display, touch, SD/NVS durability, audio and battery behavior still need physical validation.

## Test an ESP32-C3 Super Mini

The C3 has a separate **radio-only test harness** using the shared scheduler, parsers and rules. It is not the display firmware, and it is not connected to the browser preview.

With the pinned SDK and its `esp32c3` toolchain installed, run from the repository root:

```sh
idf.py -C tests/c3_radio -B "$PWD/build-c3" build
idf.py -C tests/c3_radio -B "$PWD/build-c3" -p /dev/ttyACM0 flash monitor
```

Use the C3's actual serial port and preserve its existing firmware before flashing if needed. The harness automatically pauses at 45 seconds, resumes at 65 seconds, then continues passive scanning. It reports aggregate counts, rule matches and RSSI without printing observed addresses, names or raw payloads.

The recorded 100-second room test received **33 Samsung FD5A advertisements**, with zero queue drops or radio errors. Reception stopped and resumed correctly after a watchdog timing fix. A Samsung tag was reported in the room; the transmitter was not isolated as that specific tag. See the [C3 test report and original-firmware restore instructions](docs/C3_RADIO_TEST.md).

## Test and verify

After installing the host dependencies above:

```sh
cmake -S . -B build-host -G Ninja -DSNIFFER_HOST=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure
./build-host/fuzz_mutate 60
./build-host/ui_preview preview.ppm
```

The recorded verification includes successful release/debug/test/demo firmware builds, 206 core checks, 643 UI checks, 151 extended feature checks, 12 Python tests, 100,000 deterministic fuzz inputs, and 2,101,847 additional seeded mutation inputs. The final host test suite passed with address, undefined-behavior and leak checks enabled. See [VERIFICATION](docs/VERIFICATION.md) for exact scope and build hashes; the GitHub Actions workflow is provided, but no remote CI result is claimed.

In a ptrace-based sandbox, LeakSanitizer may require `ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-host --output-on-failure`; retain leak detection on an ordinary host. Development packaging includes checksums, a source SPDX inventory, linked-archive evidence and retained dependency notices. Final license compatibility review remains a release gate.

## Export and saved state

**Settings → Export history** streams validated stored events from the SD card, retaining repeated updates, with a 4,096-file safety bound. It rekeys session IDs and observation identifiers and writes a checksum manifest only for a completed export.

For an aggregated historical export, eject the card in Settings and run:

```sh
python3 tools/export_logs.py /media/CARD/SURVSNIFF/LOGS ./community-export
python3 tools/validate_export.py ./community-export
```

Nothing is uploaded. Raw MACs, SSIDs, payloads and Remote ID serials/coordinates are excluded from sanitized exports. The device collects no GPS location. See [privacy](docs/PRIVACY.md) and [export format](docs/COMMUNITY_EXPORT.md).

Version 1 and 2 saves migrate to version 3, preserving pet progress, privacy settings, recent meals and touch calibration. Old cat selection slots become Husky, Labrador and Dalmatian. Settings, pet and system state use CRC-protected generation slots in separate NVS namespaces. Simulator state is not persisted.

## Remaining hardware work

Follow [TEST_PLAN](docs/TEST_PLAN.md), record measurements in [HARDWARE](docs/HARDWARE.md), and track readiness in [STATUS](docs/STATUS.md). Display/touch bring-up, SD/NVS power interruptions, 50 cold boots, the eight-hour soak, the 80 KB runtime heap target, measured capture loss, battery calibration/current draw and controlled product-signature captures remain outstanding.

This hardware observes 2.4 GHz Wi-Fi and legacy BLE only. It does not cover 5/6 GHz Wi-Fi, BLE extended advertising or coded PHY. Phone GPS/companion work is deferred. No cloud service, active probing, sensitive-export path or on-device rule editor is included.

The supplied specification is retained with external project names generalized. The Hound name, all-dog roster and landscape layout reflect the later project requirements. Internal `sniffer` namespaces, build flags and on-card directory names remain stable for compatibility; display-board binaries are named `surveillance_hound.bin`.

Firmware, tools and tests: Apache-2.0. Original documentation and art: CC-BY-4.0. Independently assembled signature facts: CC0-1.0, with sources and their terms recorded separately. An external detector reference was consulted for visual ideas and, with user authorization, signature facts; its implementation and assets are not incorporated. See [NOTICE](NOTICE), [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES.md) and [rule provenance](data/provenance.yaml).
