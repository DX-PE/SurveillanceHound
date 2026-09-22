<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Implementation verification

## Completion iteration — 2026-09-21 local / 2026-09-22 UTC

- All four current ESP-IDF builds pass with SDK commit `7101770dc6db2667b3c477cc31365dd1acd6db4e`. Debug/test disable the main Wi-Fi IRAM optimization to retain instrumentation headroom.
- Host ASan/UBSan suite: 206 core checks, 643 UI checks, 151 extended feature checks, 12 Python tests and 100,000 deterministic parser/JSON fuzz inputs. All five CTest targets passed in the final run.
- An additional 60-second seeded mutation run passed **2,101,847 inputs** against Wi-Fi, BLE, Remote ID, Pwnagotchi, bounded JSON, log validation and sanitized export. This is bounded mutation testing, not exhaustive or coverage-guided fuzzing. The optional libFuzzer entry point supports a longer clang run when available.
- Feature checks cover Apple subtype/length negatives, 128-bit UUIDs, Pwnagotchi fragmented IE/JSON negatives, probe requests, Remote ID validation/expiry, UTC/leap years, battery calibration, export rekeying, state migration, Start/Stop in both orientations, paging, every screen's guarded tiles and every snack category.
- Static passive-API/callback invariants, generated provenance/assets, Ruff, clang-format and JavaScript syntax checks pass.
- The committed C3 harness also builds independently from `tests/c3_radio` with the pinned RISC-V toolchain.
- USB ESP32-C3 received 33 Samsung FD5A advertisements in 100 seconds including a 20-second pause, with zero queue drops or radio errors. A scheduler watchdog issue found on the first run was corrected and retested without warnings. See [C3_RADIO_TEST.md](C3_RADIO_TEST.md).
- The browser preview at localhost:8765 was refreshed. Start/Stop displays sleeping Zs; paused sample injection is ignored. Buttons now select a fitting font size, fixing the Start Sniffing overflow. A 19-category sample selector is available. Native contact-sheet inspection also covered manual time, battery setup, clear/export settings, self-test and diagnostics.
- Development packaging includes source SPDX and linked-archive evidence: 80 archive/member inventories and 83 retained SDK/project/toolchain license files. Declarations and missing tags are recorded without claiming a completed legal compatibility conclusion.

Initial sandbox runs disabled LeakSanitizer because it cannot operate under ptrace. The final five-target CTest suite was then run outside that sandbox with leak detection enabled: all passed in 9.33 seconds. ASan and UBSan remained enabled throughout. The separate seeded mutation run used address/undefined checks without leak detection. CI configuration is updated, but a remote run has not been dispatched.

### Current application images

| Profile | Bytes | SHA-256 |
|---|---:|---|
| release | 1,168,912 | `a49aa7d2c9bee160f061632a0954eff3b0b4f35c6f5b933793ca4fa3b9cbb581` |
| debug | 1,300,896 | `bed9b2d27be45688f4beab6fabd5c869cbbb754bce849084500b7d8027f805ab` |
| test | 1,270,496 | `ede4cabb06a7f0e336c54f2f6c9ddf3d04e864988f0e3093a8cb70013d4e201d` |
| demo | 1,188,960 | `8d6c9604f5241ed355ef0164d73a8b0a8a70036caf36b884083d9aedeaa00537` |

E32R40T physical display/touch, SD recovery/eject/power loss, NVS commit interruptions, audio, battery calibration/current, capture-loss calibration, runtime heap, 50 cold boots and the eight-hour soak remain pending. No production release approval or signed tag is claimed. The records below describe earlier builds and are retained as history.

---

## Surveillance Hound redesign verification

The current Hound sources passed the following checks after the rename, dog artwork, landscape/portrait layouts, rotation lock and alert-type settings were added:

- All four ESP-IDF profiles (release, debug, test, demo) built successfully with the pinned SDK.
- ASan/UBSan host suite: 206 core checks, 12 Python tests, 100,000 fuzz inputs, and 643 UI/orientation/alert-control/save-migration/snack-animation checks.
- Both orientations render all 14 screens within guarded tile buffers; onboarding selects all six dogs and accepts the full keyboard layout.
- V1 save migration preserves progress, privacy settings, recent meals, calibration and the private meal key; upgraded saves default to locked landscape.
- Each of the 19 alert switches gates alerts while retaining observations. The confidence threshold still gates alerts; iBeacon alerts remain off by default.
- Browser verification at localhost:8765: dog scene, native touch navigation, rotation unlock/portrait layout, locked rotate button, Settings → Alert types and reset. No browser console errors observed.
- Worker protocol verified 307,200-byte RGB565 frames in both orientations, rotation-lock enforcement, per-type suppression/re-enabling, logging and demo XP isolation.
- Snack sequence: seven seconds of turn, side-walk, progressive bites and a happy finish. The compact alert stays outside the pet stage in both orientations. A later low-confidence observation cannot replace the active snack or alert; dismissing the alert keeps the meal running. Reduced animation uses a still profile.
- Static passive-radio invariants, Ruff, JavaScript syntax and generated-data checks passed. The original six-dog art has 198 frames and 196,282 RLE bytes.
- The display keeps the original 10,240-byte DMA allocation by using 480×8 landscape tiles and 320×16 portrait tiles.

| Profile | Application bytes | SHA-256 |
|---|---:|---|
| release | 1,127,376 | `f10baff42afe77756ce861bf715bc37217d853a4b9645aaed349c0d10271822e` |
| debug | 1,251,584 | `668762e68916ffb0f5360cd419a37ba8f83e0d52bd69de8e9e7c6ea8ffb1ee62` |
| test | 1,220,880 | `8ad5dd9c96cd1a272e1f37aa949ee0631ae880000bb38e21796565fe4b652ce8` |
| demo | 1,147,232 | `fb21e714bfd32882bac5bc2ae22004f9c623eeed4a55ed14828374b6a57f2071` |

Physical ST7796 orientation, touch corner mapping and reboot persistence still need testing on an E32R40T. The browser simulator keeps state only in RAM and does not exercise radio hardware or SD writes.


## Original implementation record

The measurements and artifacts below describe the earlier Sniffer build, before the Hound redesign.


Date: 2026-09-21 (local development session).

## Executed successfully

- ESP-IDF 6.0.2 at `7101770dc6db2667b3c477cc31365dd1acd6db4e`; target ESP32; Espressif GCC 15.2.0.
- Separate release, debug, test and demo builds, each with its own sdkconfig.
- 206 C++ core checks, including all 19 synthetic category paths and required negative regression classes.
- 12 Python tool tests, also rerun with the exact requirements-dev.txt environment.
- UI onboarding/keyboard checks and 13 guarded tiled-screen renders.
- 100,000 deterministic parser/JSON fuzz smoke inputs under AddressSanitizer and UndefinedBehaviorSanitizer.
- Deterministic signature and asset checks: 5 enabled rules / 20 catalog entries; 150 original frames / 78,524 RLE bytes.
- Provenance validation, passive-API/callback/SPDX static invariants, Ruff checks/format and clang-format checks.
- GCC `-fanalyzer` compiled the portable runtime and UI with no runtime-source warnings. It reported four uninitialized-value diagnostics in the standard-library vector allocator used only by the synthetic test builder; ASan/UBSan and the value-initialized fixture checks passed. These test-harness diagnostics are not claimed to be a clean full-analysis run.
- Host UI contact sheet inspected; fixed a palette/member naming collision found by the visual check.
- Local development bundle with bootloader, partition table, application, notices, checksums and SPDX source inventory.

The sandbox runs under ptrace, which makes LeakSanitizer fail before reporting results. Local sanitizer runs used `ASAN_OPTIONS=detect_leaks=0`; address and undefined-behavior checks stayed enabled. Leak detection still needs an ordinary CI/host run. CI is configured but has not been dispatched to a remote runner.

## Built application images

| Profile | Bytes | SHA-256 |
|---|---:|---|
| release | 1002384 | `9fa653ec80ec3e7265f5abd3ae9260b06687711db45c78ec2cf4b914625cb975` |
| debug | 1123120 | `99e343f025e0d2981e263fd08c5a65477abfb528d588defc48115be3d86daf6e` |
| test | 1092336 | `641ac228e50c1fa6247d42d8dd02638c3ab5360ade01785e3a8dc6b65f80615c` |
| demo | 1022288 | `d9941de45ed17663907992e3714ab7b58a9f6c2b7541fdb69bc12ccfcbef65f1` |

Release link-time memory: IRAM 120,963 / 131,072 bytes; static DRAM 116,299 / 124,580 bytes. This is **not a measurement of runtime free heap**. The 80 KB runtime target and allocation headroom with both radios running remain hardware gates. Application partition size is 3 MB; the image fits.

## Not executed / not certified

No physical E32R40T was connected. Display initialization, touch accuracy, SD durability/eject, radio passivity and packet loss, audio, ADC calibration, current draw, 50 cold boots and the eight-hour soak remain untested. Fifteen vendor families have no verified live rule. Full specification completion and the transitive binary license audit remain open. See STATUS.md and RELEASE.md.
