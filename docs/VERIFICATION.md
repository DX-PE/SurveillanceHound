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

## Roadmap pass: Scent Book, wardrobe, Snooze/Ignore — 2026-09-22

The updated host suite passed all six targets with ASan, UBSan and LeakSanitizer enabled outside the tracing sandbox: 206 core checks, 643 UI checks, 151 extended feature checks, 936 new companion checks, 12 Python tool tests and 100,000 deterministic fuzz inputs. The new checks cover both orientations, every category, every dog/outfit, eating/sleeping frames, locked equipment, discovery/XP unlocks, frozen ignore targets, scope and capacity of ignores, snooze expiry/resume, demo isolation, legacy state migration and invalid companion state.

The final release/debug/test/demo ESP32 profiles built with the pinned ESP-IDF 6.0.2. Static passive-radio invariants and source formatting passed. These are build/host checks; E32R40T touch, sound, NVS migration and power-cut durability, capture accuracy, and runtime stack/heap remain pending physical tests.

| Profile | Application bytes | SHA-256 |
|---|---:|---|
| release | 1,186,400 | `10c7a34f1324eb28437e2fe1ba8960a8cc9a0ab3a35a24f7fce0513334253371` |
| debug | 1,319,360 | `cb347f9ad3e0089a90b7f5efbee26e26d15f9779f1535a6ec4ba36a04239c18e` |
| test | 1,289,088 | `d50e639c37671da55666a79f463c79c31c8de428f725ac047655fbc6f544ece3` |
| demo | 1,206,256 | `90101300d7cfb3a029e044beb22150433ed9204d63f3acbe0506bf95b6a9f8b7` |

The final browser pass exercised Scent Book and wardrobe in both orientations and the Snooze/Ignore controls through the log. A portrait instruction overlap was corrected, followed by another passing sanitized UI/companion run and all four firmware rebuilds. The loopback preview was refreshed at port 8765.

## Follow Scent and Atmosphere — 2026-09-22

Implemented in that order. The eight-target host suite passes with ASan, UBSan and LeakSanitizer enabled outside the tracing sandbox: 206 core checks, 643 UI checks, 151 extended feature checks, 936 companion checks, 166 Follow Scent checks, 394 appearance checks, 12 Python tests and 100,000 deterministic parser/JSON fuzz inputs. Follow tests exercise fresh readings separately from historical extrema, bounded sampling, trend gaps, stale/lost transitions, address/radio/type isolation, demo separation, pause/resume, no extra logs/XP and navigation. Appearance checks cover all nine theme/scene combinations, compact/detailed panels, both orientations, static reduced motion, speech controls, demo isolation and save validation.

All four ESP32 firmware profiles build with the pinned ESP-IDF 6.0.2. Static passive API invariants, JavaScript syntax, Python lint, generated assets/signatures and C++ formatting checks pass. No RF scheduling, active probing or transmit APIs were added. V5 extends generation-based state with a required CRC-protected appearance record; V4 migration retains companion data and defaults appearance. NVS migration/power interruption, display performance and actual RSSI behavior still require physical E32R40T testing.

| Profile | Application bytes | SHA-256 |
|---|---:|---|
| release | 1,194,448 | `f8969b26cf5c9747f966a0805e2418adb88f3b97ecd44ae749f10edd3862e599` |
| debug | 1,328,736 | `067d04ddd7fb84791cc760577edf9e3c978b138bc49bf0ea4e7b1bb127890de4` |
| test | 1,298,368 | `62ff23796e16c79215f7856c59d3d5cd67e170d9a8ffb201d178d7e4884b2f4a` |
| demo | 1,214,272 | `e6b1bae23c519bddebb22a2c137436cc1c342ee66e4aaa960eb38875140ed967` |

The final idle-pose adjustment passed another sanitized run of the four affected UI/companion/follow/appearance targets and all four firmware rebuilds. Native rendered frames were inspected for live, stale, lost and paused follow states, every theme/background pairing, and portrait follow/settings/compact-home layouts. Browser interactions verified the Atmosphere controls, synthetic sighting → Log → Follow Scent route and signal controls. The local preview uses the updated source.

## Dog Park background — 2026-09-22

Replaced Pine Trail with an original fenced dog park, trees, agility hoop, hydrant, tennis ball and frisbee, with park-specific idle lines. Existing appearance settings remain compatible. The 394 existing appearance checks passed with ASan/UBSan (leak detection disabled under the tracing sandbox), including all themes and both orientations. Landscape and portrait frames were visually inspected, formatting passed, and the ESP32 release profile rebuilt successfully.

## Hosyond first hardware connection — 2026-09-22

The Hosyond 4-inch board replaced the C3 on USB. ROM identity confirms ESP32-D0WD-V3 revision 3.1 with 4 MB flash. A complete original flash backup and checksum were retained locally; the current 1,195,552-byte release application was flashed and its written hash verified. The bootloader loaded the application and it reached SD initialization, which returned `0x107` (no usable SD card detected). Physical display appearance, calibration and first-run setup are awaiting user observation; RF accuracy, persistence, storage and soak acceptance are still open. See [hardware bring-up](HARDWARE.md#first-hosyond-usb-bring-up--2026-09-22) for hashes and restoration details.

The user subsequently reported that the UI displayed with a white background and blue dogs. A first hardware correction disables panel inversion (`0x20`, replacing `0x21`). The corrected release application builds successfully; the user confirmed that the corrected colors look right. This application-only update preserves NVS/calibration.

## Saved panel inversion and Settings order — 2026-09-22

The Settings entry page now prioritizes Wardrobe, Atmosphere, Scent Book, Ignored scents and Snooze/Resume. The next-page button names its destination; SET and BOOT entry return to the first page. Display options provides brightness, immediate saved inversion, animation and a color/touch test. Normal polarity remains the default. Demo inversion stays separate from real preferences and restores the real setting when leaving demo.

The five affected host targets pass with ASan/UBSan: 643 UI, 151 extended feature, 936 companion, 166 Follow Scent and 424 appearance checks. New checks cover appearance-record migration, validation, inversion/save requests, demo restoration and menu entry/navigation. Native simulator protocol checks verify exact RGB565 complement outside the changed toggle row and exact full-frame restoration after toggling off, in both orientations. Daily Settings and Display options were visually inspected; formatting and passive API checks pass. SD work is on hold at the user's request.

All four updated ESP32 profiles build. The combined release image was flashed to the Hosyond with esptool verification and booted through saved-state loading; the short serial capture shows only the missing-card initialization errors, with no observed panic or reboot. No SD test was performed. The browser preview was refreshed to the same source. Physical toggle/save/reboot checks and longer acceptance testing remain open.

| Profile | Application bytes | SHA-256 |
|---|---:|---|
| release | 1,197,328 | `1c9c757370659b80762740c8a06d1913753a50463ed53b67f2c21d70206d1f36` |
| debug | 1,331,824 | `14321849f0a774e16911e35c691687aed3b10e6e8d59401c79d0feae702cdba8` |
| test | 1,301,424 | `b499d2127baa8a0ad35e8bccf42a27edafeba46d7eeb4ad9f69e52299d882ee0` |
| demo | 1,217,152 | `6350b0942bc795841a7c1dcdc08420e2c49771b699f1e3a56517076ec136921a` |

## Low heap, repeated counts and compact alerts — 2026-09-22

Changed the home totals from cumulative emitted events to distinct recent radio/address/type identities using the existing 128-entry evidence cache. A 90-second window, confidence-bucket updates and category overlap suppression prevent repeat inflation. Mute/snooze/ignore preserve observed-signal counts. Non-feeding alerts now share the compact card with feeding alerts; text is limited to the available card area and the dog/Start–Stop control remain visible.

Reduced unused NimBLE connection-data pools while preserving advertising reception buffers and the raw-observation queue. The eight storage messages carry small trivially copyable variants, with two separately reserved state snapshots and explicit failure on exhaustion. No NVS format or SD feature changed. Diagnostics now uses byte-addressable internal heap and displays the largest block; recovery clears the low-heap banner and unrelated notices keep priority. USB aggregate health includes NVS save errors and no observed identifiers.

All eight host CTest targets passed with ASan/UBSan (`detect_leaks=0` because of the sandbox's ptrace limitation), including 100 repeated three-signal cycles, confidence/category/identity/expiry/capacity cases, snapshot lifecycle and queue-byte-copy checks, heap warning recovery, and pixel comparisons for feeding/non-feeding alert layouts in both orientations. The UI and feature targets passed again after the final four-pixel card-height adjustment. Native firmware renders were inspected in landscape and portrait. Passive-radio static invariants and all four pinned ESP-IDF firmware profiles passed.

| Final profile | Application bytes | SHA-256 |
|---|---:|---|
| release | 1,200,160 | `f40710edaaa763682922f5de1962f1a1e7801e7eb95619c2adf018e825d7bf31` |
| debug | 1,334,864 | `e0689c1042db3e46b98f622d559cfdc9a03ac778090fd2e4df01d7bc1097d8f3` |
| test | 1,304,448 | `01f98905c9be7324c7e7b21e1aa82018f771ae6f6adde5e73d06d229ae61a1a9` |
| demo | 1,219,984 | `7eb4d6e8d04507a5d0058b69117dbe257cb65de158b82d3b9c597b309608e36a` |

The five-minute no-card hardware run and its exact application hash are recorded in [HARDWARE](HARDWARE.md); it preceded only the final card-height adjustment. The minimum-heap acceptance gate, controlled capture-loss test, SD work and long soak remain open.

The final release image was then hash-verified on the Hosyond and rebooted with the existing NVS state. A 135-second follow-up recorded 86,244–86,824 B current heap, a 79,896 B minimum, a 77,824 B largest block, and zero radio/save errors through two save intervals. See the aggregate [serial record](measurements/2026-09-22-heap.txt). The local preview was rebuilt and restarted at `http://127.0.0.1:8765/`.
