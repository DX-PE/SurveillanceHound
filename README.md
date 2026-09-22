<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Surveillance Hound

An offline ESP32 radio-observation pet for the LCDWiki E32R40T. This is the **initial 0.1.0 development implementation**, not a hardware-validated release.

![Firmware UI rendered on the host](docs/ui-preview.png)

[Sleeping hound and new settings preview](docs/ui-features.png)

The firmware uses passive Wi-Fi management reception and passive legacy BLE scanning. It never associates, probes, advertises, injects frames, or uploads data. A radio match describes evidence, not intent or proof of surveillance.

## What works in this implementation

- Native ESP-IDF **6.0.2**, pinned to commit `7101770dc6db2667b3c477cc31365dd1acd6db4e`; C++20 portable core and native board drivers.
- ST7796 SPI display, XPT2046 three-point affine calibration, separate SD SPI bus, backlight, LED, BOOT fallback, battery ADC, optional quiet audio.
- Six original dogs (Corgi, Beagle, Shiba, Husky, Labrador, Dalmatian), 33 RLE frames each, onboarding, naming, home, observations, evidence details, detector controls, settings and diagnostics. A neon patrol scene, touch reactions, side-facing walk-and-eat snack sequences and themed alerts share one bounded tile buffer. Landscape (480 × 320) is the default, with a portrait layout and rotation lock in Settings.
- Bounded frame parsing, confidence fusion, 128-entry aggregation and meal caches, cooldowns, per-category limits, persisted recent meals and pet state.
- 67 enabled rules covering all 19 categories, including Samsung/Tile/Find My services, Flipper, Flock, vendor clues, structured Pwnagotchi, legacy Remote ID assembly and behavior heuristics. Product accuracy remains subject to field validation. See [DETECTIONS](docs/DETECTIONS.md).
- Private session HMAC identifiers, opt-in raw local MAC logging, rotated JSONL, complete-row validation, partial-tail recovery, full-history SD export, and a complete offline historical export tool.
- Start/Stop sniffing with sleeping Zs; themed food for every category; 32-entry paged history; manual UTC; battery calibration; SD-space diagnostics; confirmed log clearing and touch/display self-test.
- Deterministic generators with enforced provenance, host regression and fuzz tests, UI previews, build profiles, CI, development binary packaging and an SPDX source inventory.

## Try it without hardware

Python 3.11+, CMake 3.22+, Ninja, a C++20 compiler and OpenSSL development headers are required. On Debian/Ubuntu: `build-essential cmake ninja-build libssl-dev python3-venv`.

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements-dev.txt
cmake -S . -B build-host -G Ninja -DSNIFFER_HOST=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure
./build-host/sniffer_demo
./build-host/ui_preview preview.ppm
```

For an interactive browser preview using the firmware renderer and touch handlers, run:

```sh
python3 tools/run_local.py
```

Open **http://127.0.0.1:8765**. Tap **Stop sniffing** to put your hound to sleep; **Start sniffing** resumes reception. The sample selector previews all 19 snack motifs. In **Settings**, switch **Rotation lock** off to change **View** or use **TURN** in the top bar; switch it on to keep the chosen orientation. Rotation is manual because this board has no orientation sensor. **Settings → Alert types** controls pop-ups and sounds per category while sightings remain in the log; the confidence threshold still applies. The separate Detectors screen controls observation processing. Click the device screen, select a hound, walk through onboarding, and trigger synthetic detections. The simulator binds only to loopback and keeps all state in memory. Radio capture, SD storage and persistence require the physical board; synthetic events do not award real XP. Press Ctrl+C to stop. Use `--port 8766` to choose another port.

In a ptrace-based sandbox, use `ASAN_OPTIONS=detect_leaks=0 ctest ...`; address and undefined-behavior sanitizers remain enabled. Use the ordinary command outside that environment to retain leak checking.

## Build and flash

Activate the exact ESP-IDF SDK, then:

```sh
idf.py build
./tools/flash.sh /dev/ttyUSB0
```

See [BUILDING](docs/BUILDING.md) for SDK installation, profiles and the one-command Docker build. Start hardware bring-up on USB power. The USB ESP32-C3 radio harness received Samsung FD5A advertisements and passed its Stop/Start test. The E32R40T display/touch/storage/power tests remain pending. See [C3 radio test](docs/C3_RADIO_TEST.md).

## Export the card

```sh
python3 tools/export_logs.py /media/CARD/SURVSNIFF/LOGS ./community-export
python3 tools/validate_export.py ./community-export
```

The on-device `Export history` action streams validated stored events from all log files (4,096-file safety bound). The host command processes the full log history with disk-backed aggregation and rekeys identifiers for each export. Nothing is uploaded. See [privacy](docs/PRIVACY.md) and [export format](docs/COMMUNITY_EXPORT.md).

Saved v1 pet progress, privacy settings, recent meals and touch calibration migrate to v3 on first boot. Old cat selection slots become Husky, Labrador and Dalmatian, respectively. Simulator settings remain in memory.

The original supplied spec is retained unchanged; the Hound name, dog roster and landscape layout follow the subsequent user requests. Internal `sniffer` namespaces, build flags and on-card directory names remain stable for compatibility. New application binaries are named `surveillance_hound.bin`.

## Next hardware milestone

Follow [TEST_PLAN](docs/TEST_PLAN.md), record results in [HARDWARE](docs/HARDWARE.md), and close the open items in [STATUS](docs/STATUS.md). In particular, the eight-hour soak, 80 KB runtime heap target, measured capture loss, battery calibration, and controlled product-signature captures are still outstanding.

Firmware, tools and tests: Apache-2.0. Original documentation and art: CC-BY-4.0. Independently assembled signature facts: CC0-1.0. See [NOTICE](NOTICE), [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES.md).
