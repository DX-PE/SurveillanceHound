<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Implementation verification

## Zensical documentation site — 2026-09-24

- Organized all repository Markdown into 37 navigable pages, including five single-source imports for the dependency ledger, asset attribution and test worksheets. Historical status and hardware evidence remain available under Engineering records.
- Added the firmware Midnight palette, original six-dog SVG artwork, light mode, responsive tables, search and code-copy controls. The documentation footer and project notices credit Copyright 2026 Jascha Wanger (https://dx.pe).
- Pinned Zensical 0.0.65 and its Python dependencies. The GitHub documentation workflow builds a static artifact on pushes, pull requests and manual dispatch; public hosting and a remote workflow run remain pending.
- Local strict clean build passed. Source coverage and sprite checks passed; all local links, assets and fragments passed across 38 generated HTML pages (including the accessible 404 page). The link checker also rejected controlled missing-page, missing-anchor and outside-site cases. New Python tools passed Ruff 0.13.1 lint/format checks; the repository diff passed whitespace checks.
- Browser checks covered the Midnight home page, search results, guide navigation, light-mode switching and 390-pixel guide/table layout without document overflow. The existing emulator remains separate on port 8765; documentation is served on port 8000. No firmware or saved-state changes were made for this documentation work.

## Coverage help and inference guards — 2026-09-23

- Roadmap items 1–2 and the SSID frame-role prerequisite implemented; the catalogue remains 67 enabled rules / 19 categories. No saved-state format or radio callback changes.
- All 15 CTest suites passed with AddressSanitizer, UndefinedBehaviorSanitizer and LeakSanitizer enabled (outside the ptrace sandbox). Added SSID boundary cases passed in a follow-up core run.
- New regressions cover parsed beacons/probe responses versus probe requests, exact/prefix names, case/leading-junk/empty/embedded-NUL/32-byte SSIDs, duplicate/truncated IEs, all four transmitter U/L + I/G bit combinations, retained local-source SSID/payload evidence, and BLE company matching.
- Coverage navigation tests exercise both orientations and all three themes, page cycling, Back preserving the book page, guarded tile buffers, unchanged progress, continuing reception and idle dim/wake behavior.
- Native render inspection covered all five help pages and the Scent Book entry in landscape and portrait, including Midnight, Sunset and Daylight. Browser navigation verified Scent Book → Coverage at localhost:8765; intermittent browser-control timeouts limited the interactive pass, so the complete page/layout check used the same native renderer directly.
- Generated signature/assets checks, passive-API/callback invariants and clang-format checks passed.
- ESP32 release build passed using the pinned ESP-IDF 6.0.2 commit in the official `espressif/idf:v6.0.2` container (`sha256:0d8c9773d48a327233f9c1d7c654ff0bcf133ae24503ea2e97a57cfe02b8cb67`). Application: **1,288,432 bytes**, SHA-256 `a24290d8ecd957a7b4bea8dd50d145055544001ada1d819d3589db48403d8545`; 59% of the 3 MiB application partition remains free. Other firmware profiles were not rebuilt for this change.
- Installed on the Hosyond on 2026-09-24 after SD eject confirmation: esptool hash verified, partition/NVS/PHY readback unchanged, and two boot-health samples with increasing Wi-Fi/BLE callbacks and no reported radio/save errors or observed panic/watchdog. See [installation receipt](HARDWARE.md#coverage-help-and-inference-guards-installed-2026-09-24). Physical UI/SD checks and controlled RF validation remain pending.

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

The Hosyond 4-inch board replaced the C3 on USB. ROM identity confirms ESP32-D0WD-V3 revision 3.1 with 4 MB flash. A complete original flash backup and checksum were retained locally; the current 1,195,552-byte release application was flashed and its written hash verified. The bootloader loaded the application and it reached SD initialization, which returned `0x107` (no usable SD card detected). Physical display appearance, calibration and first-run setup are awaiting user observation; RF accuracy, persistence, storage and soak acceptance are still open. See [hardware bring-up](HARDWARE.md#first-hosyond-usb-bring-up-2026-09-22) for hashes and restoration details.

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

## 2026-09-22 — Tag Watch

Added the manual ten-minute tag-presence watch, bounded to sixteen keyed identities, with slow red background flashing, a steady alternative, ACK/Ignore/Snooze, and a synthetic clock-advance preview. This is a possible-following clue, not movement confirmation. The new tracker is statically bounded to 544 bytes; no NVS layout or SD behavior changed.

All nine host CTest suites passed under ASan/UBSan, including 271 Tag Watch checks and 100,000 fuzz-smoke inputs. LeakSanitizer was disabled because of the sandbox/ptrace environment. Production Samsung FD5A detections reached the warning through the normal UI event path. Generated-rule/assets checks, passive/static invariants, Ruff and C++ formatting passed. All four ESP32 profiles (release/debug/test/demo) built with the pinned ESP-IDF 6.0.2. Release image SHA-256: `955acae092fc7ddbda2b4fe68effd4a2049113468cb5c8839329fc85155dad7d`.

Browser checks exercised the preview button and acknowledgement, confirmed zero demo XP for the test sequence, and inspected warning contrast and controls. Deterministic native renders and adapter checks covered portrait/landscape, warning, review and acknowledgement; rendering tests covered all themes/scenes and tile bounds.

The Hosyond release application was flashed at `0x10000` and esptool verified its written hash. A fresh 24 KB NVS backup was retained privately outside the repository; the NVS, bootloader and partition-table regions were not erased. A 45-second boot check resumed Wi-Fi/BLE with zero reported radio/save errors and no observed panic/watchdog reset. Current heap was 85,668–86,248 B; the minimum was 79,304 B and largest block 77,824 B. This is a boot smoke test with Travel Watch initially off, not a physical warning or moving-tag validation. The existing minimum-heap acceptance gate, physical display/touch, moving/stationary RF trials, crowd capacity, address rotation and long soak remain open. SD testing is still on hold.

## 2026-09-22 — Watch timing and visible progress

The user reported ordinary tag alerts after more than ten minutes armed, without a Watch warning. Inspection found that a packet received during a UI batch could have a timestamp later than the frame-start clock, causing Watch/Follow to reject it as future data. The board now refreshes its monotonic clock before handling each parsed observation. This fixes that edge case; it does not establish that it caused the user’s earlier missing warning. No RF continuity or address-rotation evidence was captured for that run.

Watch progress now shows up to sixteen per-identity timers, occupied-minute counts, last-seen age, classification score and ACK state, with bounded gap/filter/clock counters and configuration hints. It opens on arming and from the armed Home shortcut. Controls, review, Snooze and Ignore remain separate reachable actions. The ten-minute/eight-minute-bin/two-minute-gap requirements, category/score filters, and stored data formats are unchanged. The tracker still passes its 544-byte static bound.

All nine ASan/UBSan host suites passed, including 328 Tag Watch checks. Tests cover stale-frame timestamps, expiry/filter counting, counter saturation, navigation, changing page size and guarded rendering in both orientations. Native simulator images were inspected for populated and empty progress pages. Generated-data checks, passive invariants, Ruff and C++ formatting passed; release/debug/test/demo firmware builds passed. The simulator builds with its normal unoptimized host configuration. An optional `-O3` host build reported a GCC 15 array-bounds warning in the unchanged small-array `std::sort` path in engine.cpp; no firmware build failed.

Release application: 1,208,912 bytes, SHA-256 `949622ebb4df9e2ef08b3e70baf09a9cf4fb0899a753c7c5514a9a9eb3c4c164`. Firmware write/field status is recorded in HARDWARE.md. LeakSanitizer remains disabled for the sandbox environment. The earlier short heap captures do not validate this updated runtime or an armed ten-minute watch.

## 2026-09-22 — Restore full Home title

Restored SURVEILLANCE HOUND on the Home screen: one line in landscape and two lines in portrait. Shared layout rectangles keep the Book, Watch and active Follow controls aligned with their touch targets without overlapping the title. All nine existing ASan/UBSan host suites passed (LeakSanitizer disabled under the sandbox); release/debug/test/demo firmware profiles built. Native frames were visually checked in both orientations with and without Follow, and shortcut navigation was exercised in each orientation. The local browser simulator was rebuilt and refreshed.

Release application: 1,209,456 bytes, SHA-256 `b214a45abc230cf99192ba2980a1b25d25cf06d4343f519b295ffc069d8200d8`. The Hosyond application-only flash passed esptool's written-data verification. Watch timing and qualification behavior are unchanged; physical Watch feedback remains pending.

## 2026-09-22 — User-confirmed Watch warning on Hosyond

The user reported the possible-following alert triggering on the board and confirmed that the background flashed red with the dog and controls still visible. This supplies the previously pending basic alert/display confirmation for the latest installed title/layout release. Exact qualification time, triggering identity/category, reset counters and movement were not recorded, so the earlier delay remains unexplained and controlled RF/timing acceptance remains open. Documentation only was updated; the running board was left untouched.

## 2026-09-22 — SD-enabled memory headroom

After a successful 32 GB card mount, the user reported 75,384 B free heap and a 69,332 B largest block with the low-heap banner. The warning still uses its original 80,000-byte threshold. The update compacts the 128-entry detection cache by removing unused evidence slots and redundant occupancy padding, sets the ESP32 controller connection reservation to one, and uses a 512-byte log stdio buffer. No receive-queue capacity, confidence rule, disk/NVS format or flush/fsync cadence changed.

All nine ASan/UBSan host suites passed. The core target passed again after adding pairwise corroboration/group-isolation and zero-time cache-occupancy regressions (502 core checks). The 60-second seeded mutation run passed 1,918,124 inputs; LeakSanitizer was disabled for the sandbox environment. Static passive-radio invariants and C++ formatting passed, and all four ESP32 firmware profiles built. The ELF confirms Engine shrank from 29,072 to 26,000 bytes (3,072 B). Generated firmware configurations confirm a controller limit of one. The additional controller/log-buffer savings and SD/export peaks still require hardware measurement.

Release application: 1,209,456 bytes; SHA-256 `b87717a1d08963c1d5c8cc5f5a2bd3594813f4b7c971f3e5d14dc102418e3843`. After the user confirmed SD eject, the application partition was flashed and esptool verified the written-data hash. A fresh private NVS backup was retained, and settings/calibration were preserved. The board was reset to run the update; no serial monitor was opened afterward. Post-update card mount, heap and log-write readings await user feedback.

The subsequent user-reported current free heap was **82,256 B**, an increase of 6,872 B over the pre-update reading. This snapshot is above the warning threshold but does not establish the minimum-heap or soak acceptance gate. Post-update SD status, log-write counters, export integrity and minimum/largest-heap readings remain pending; see [HARDWARE](HARDWARE.md).

The user subsequently observed SD WRITES around 28–30 and then 32, confirming progress in the displayed cumulative record-write count. Saved-file integrity, export completion and sustained heap acceptance remain unverified; no board connection or reset was needed for this check.

## 2026-09-22 — Export active-log handoff

The first physical export attempt showed SD ERROR. The current log was still open for writing when the exporter tried to read it. The pinned SDK's FatFs `chk_share` implementation and [FatFs documentation](https://elm-chan.org/fsw/ff/doc/open.html) prohibit this duplicate writer/reader open when file locking is enabled. The export path now closes the flushed writer before reading history and reopens that same existing file at EOF afterward. The portable production export implementation is shared with the new host regression target; firmware retains the same worker, queue, privacy transformations, file formats and logging buffer size.

All ten ASan/UBSan host suites passed. After adding further sync-failure coverage, the storage export target passed 57 checks. Its host file-open wrapper models FatFs's writer-exclusion rule, first demonstrates the forbidden original sequence, and then exercises the production exporter with current and prior logs. Checks cover complete history, continued appends, repeat export, existing destination preservation, malformed history, hash failure, all four artifact sync failures, log-reopen failure and missing writer. A successful three-row output passed `tools/validate_export.py`, including hashes and row metadata. This is a modeled filesystem regression, not a real SD endurance or fault-injection result. LeakSanitizer was disabled for the sandbox environment.

Static passive-radio invariants, generated-data checks, Ruff and changed-file C++ formatting passed. All four firmware profiles built against the pinned SDK:

| Profile | Application bytes | SHA-256 |
|---|---:|---|
| release | 1,209,824 | `8ea3646d669ba9d69b1e19a9a42e3e7710c24498fd297aa421f9942902389627` |
| debug | 1,346,032 | `ffae683633ba96236b8404ca5f8b4d87f4d421632b28e2b29ab1339ba4a86b72` |
| test | 1,315,632 | `d91d2c3675c03ff1c8f1a9e1570091b105b75c3676bb04406c243ada64b10b63` |
| demo | 1,229,648 | `83a730807cadd3f576cc92932374311164d077e15cfcf6e45a4d20bd24b8e5d1` |

After the user confirmed tapping Eject SD and waiting, the corrected release application was flashed at `0x10000` and esptool verified the written-data hash. A fresh private 24 KB NVS backup was retained; calibration and saved settings were preserved. The board reset into the new image, and no serial monitor was opened afterward. Physical card remount, export completion, heap/stack during export, resumed SD writes and validation of files read from the card remain open. See [HARDWARE](HARDWARE.md) for installation details and the absent eject-confirmation message.

## 2026-09-22 — Visible export feedback and deferred save retry

The user confirmed EXPORTS reached 1 on the export-lock fix and later confirmed SD WRITES continued increasing while SAVE QUEUE FULL was displayed. This provides firmware-counter confirmation of export completion and continuing log activity, not independent validation of the card's saved files or NVS writes.

Export History now displays EXPORTING..., COMPLETE: N RECORDS or a failure/demo message in its existing Settings button on both Data and Tools pages. A pending export suppresses repeat taps. Storage publishes the completed row count before incrementing the completion counter. Rejected settings/progress saves retry after one second with the latest UI-owned state; the status clears once the worker accepts the save. No additional saved-state snapshot, queue capacity or persisted format is added.

All ten ASan/UBSan host suites passed. UI/features targets passed again after shortening feedback for larger text. Tests cover immediate acknowledgement, asynchronous completion, an existing export count, repeated taps, empty exports, queue rejection/retry, demo/no-card/read-only/error states, persistence of feedback, both orientations and menu locations, and rendering confined to the existing button. Save tests exhaust both immutable snapshot slots, advance the retry clock, release capacity and verify the latest state is queued without changing the other snapshot. Recovery preserves unrelated critical notices. LeakSanitizer was disabled for the sandbox environment. Native contact sheets were visually checked in landscape and portrait; static invariants, generated data, Ruff and changed-file formatting passed.

All four final firmware profiles built with the pinned SDK:

| Profile | Application bytes | SHA-256 |
|---|---:|---|
| release | 1,210,560 | `e7f3cc98251272f33a53e53cf3aaf8d57bd06b3b97937cdbe6c08a5644baa0af` |
| debug | 1,347,104 | `c3b5fab96d419f04c2c1fe82299e201085539834b1bf27308c90e56e22791d6a` |
| test | 1,316,704 | `43de0219b31be601ed5d7df906090c3f2aa45a36fd92fa26b7dc818606b0fa1a` |
| demo | 1,230,384 | `b2fda39c2d673c3cfb8eb0beb54fd64b8eecbaeb4048d5eb707eccbaa552e5ff` |

No serial connection, reset or firmware flash was performed while preparing this update. Physical feedback/save recovery and export-file validation remain pending.

## 2026-09-22 — Hunger, mood and meal recovery

Added powered-on needs decay, rescue meals, teary/slower sad poses across all six breeds, and a bounded happy hop sequence after eating. Demo hunger/mood use independent RAM values, including petting, and the simulator provides hungry/low-mood/time-advance controls. The original 33-frame layout remains; the regenerated asset payload is 196,008 RLE bytes. The saved Pet remains 104 bytes in the installed and updated ESP32 ELFs, with no persisted field changes. A progress reset starts a fresh needs timer even after long uptime.

All ten ASan/UBSan host suites passed. Core/UI/features passed again after the reset-clock regression was added. Checks cover five-minute boundaries, saturation, long/backward clocks, hunger and mood independently, meal eligibility, rescue and timer restart, demo isolation, happy-hop timing, reduced motion and paused behavior, all breeds and guarded rendering in both orientations. Native contact sheets were visually checked for teary faces and three jump phases. Static radio invariants, generated assets/rules, Ruff and changed-file C++ formatting passed. LeakSanitizer was disabled for the sandbox environment.

The final four firmware profiles built:

| Profile | Application bytes | SHA-256 |
|---|---:|---|
| release | 1,211,264 | `9f0767adbc8d2fb9b591094ad48614773e0596ec04f0623b73fa3e9c3697daf2` |
| debug | 1,348,160 | `a174abdd6ac6f6309d9c6ac596fac32134cf01f49199bb9d55a2b0a2006c0404` |
| test | 1,317,744 | `cd0d2638d99f43e075d26ee5dfaeccc9b733e7843862bf374e93a345783d8424` |
| demo | 1,231,072 | `5caf538cdedf06e3bfa77c43d72c52fcad65313825cbfe4814bbb21217fb6884` |

The combined gameplay/export-feedback/save-retry update is prepared but not flashed. The board remains on the user-confirmed export-lock correction. Physical behavior, heap with SD, NVS persistence and card-file validation remain open.

The refreshed local browser preview was exercised through its visible controls: Hungry hound set fullness/mood to 15/60, a Flipper snack recovered them to 35/75; Low mood set 60/15, and a Pineapple snack recovered them to 74/40. The 30-minute time-advance control was then exercised to check decay through the simulator adapter. These are synthetic-only preview observations.


## 2026-09-22 — Safe eject and saved-state verification

Eject now has persistent pending/safe/error/busy/no-card/demo feedback in the Data button and a visible SD SAFE top-bar status. Success follows flush, fsync and fclose; all three failures are reported. The worker stops further card access even if close fails, retaining that failure through repeated taps. NVS SAVES and NVS ERRORS on Diagnostics page 3 distinguish completed saves from queue acceptance.

All eleven ASan/UBSan host suites passed. The updated UI and persistence targets passed again after a landscape diagnostics-spacing correction and coverage of the pinned SDK's immediate-write semantics. The production persistence target passes 1,071 checks, including twelve interruption boundaries and twelve failed calls in both immediate-write and buffered-commit models, recovery/retry, and corruption in every required namespace. Storage export/eject passes 70 checks including all three close-stage failures. These are API/filesystem models, not physical power-loss or flash-page tests. LeakSanitizer remains disabled in the sandbox. Static invariants, generated data, Ruff and changed C++ formatting pass. Native portrait/landscape contact sheets were inspected.

The installed and updated ESP32 debug type layouts match for settings/calibration, pet/meal key, companion and appearance record envelopes. A fresh private 24 KB NVS backup passed checksums for all five records in generation 417 before flashing.

| Profile | Application bytes | SHA-256 |
|---|---:|---|
| release | 1,212,448 | `9c925710682ac7f0a587acd31edb5044c9c46463ea63c7b001bb6b10fc071ea5` |
| debug | 1,349,312 | `a785d4c89b159c71f25eeecf6265cfa9fd40d3c66dc530773d67169ff3b2239d` |
| test | 1,318,896 | `3157f8802f964c144102a015c76cab3d109d2b543a797d814a0413ba414f601f` |
| demo | 1,232,256 | `1aca7391a8adbf0918c18c6474937f145a26d0d274b8c253c05b543fcd6baea7` |

After the user reported SD OFF following software eject, the combined release was written only to the application partition at 0x10000 and its written-data hash verified. NVS/calibration, bootloader and partition table were retained; no format or erase was performed. No serial monitor was opened after flashing. Physical export acknowledgement, card-file inspection, eject/remount and saved-state reboot observations are in progress; no completed hardware acceptance is claimed.


A subsequent eject-path audit found that an Export message queued behind Eject would skip export rows but still query free space. The space-query helper now exits once ejected, and the UI blocks new export requests during/after eject. The UI regression passed again, as did static invariants, formatting and all four builds. The final guarded release is **prepared, not yet flashed**, pending fresh software eject: 1,212,480 bytes, SHA-256 `8c7f238516184b4421d3009979428dbf9690443eed306e95a6638e66e12fa6a8`. The currently installed application remains `9c925710...`. Guarded debug/test/demo hashes are `07667500b8c568c5e07337c226868a90b7bb514f29b31aa3beb67ec8a91c3c3f`, `a5a5ffff8e7b11a196707be3df0f233b28eb139909c406a2c33476eb831aa087`, and `c6e8fbf8ad6643b1830a2bccc2af2df1a1bba6a9fe0a31b98a90cf751cdc86f6` respectively.


The user reported **COMPLETE: 581 records** after exporting on the installed combined update (`9c925710...`). This confirms the new on-screen completion feedback and the firmware-reported row count. The saved files and checksum manifest have not yet been independently read. SAFE TO REMOVE confirmation and reader availability are still pending; no reset or serial connection was made for this observation.


### Final eject guard installed and NVS readback — 2026-09-22

The user confirmed **SAFE TO REMOVE** after the 581-record export and said no card reader is available. Independent exported-file/checksum verification is deferred until a reader is available; on-screen completion is not substituted for that check.

A fresh private 24 KB NVS backup (`hardware-backups/hosyond-nvs-before-eject-guard-20260922-150705.bin`) was taken after safe eject. Compared with the pre-update backup, generation advanced 417 → 423 and boot count 17 → 18. Settings/calibration, appearance, the ignored-identity array, equipped/unlocked outfit bytes and meal key match exactly. The saved dog remains SCOUT/beagle with Trail Bandana and six ignores. XP advanced 474 → 479 and meals 93 → 94; fullness/mood are still 100/100. All five required envelope CRCs and the parsed NVS blob-chunk CRCs passed. This confirms retained state and a later saved meal after the previous reboot; it is not physical power-cut fault injection.

The guarded release (1,212,480 bytes, SHA-256 `8c7f238516184b4421d3009979428dbf9690443eed306e95a6638e66e12fa6a8`) was then written to the application partition at 0x10000 and esptool verified the written hash. NVS, calibration, bootloader and partition table were retained; no card format/erase was performed. A hard reset started the update; no serial monitor was opened afterward. Local preview was refreshed. The user was asked to perform a final safe-eject/power-off/card-reinsert/reboot check and confirm SD OK plus the saved outfit. Overnight soak and controlled outdoor tag tests remain planned for later.


On the final guarded firmware, the user performed safe eject, disconnected USB, removed/reinserted the same card, and reconnected USB. They confirmed **SD OK** and SCOUT’s Trail Bandana returned. This passes the observed same-card remount and saved-outfit power-cycle check. Post-remount log-write progress and NVS counters are being checked separately; card-file integrity remains deferred without a reader.


After the final power-cycle/reinsertion check, the user confirmed SD WRITES increased over about 30 seconds and Diagnostics showed **NVS SAVES: 1, NVS ERRORS: 0**. This confirms resumed logging and one completed post-reboot save according to firmware diagnostics. The update, visible export/eject feedback, same-card remount and saved-state checks are complete within the available hardware setup. Independent SD-file/checksum inspection remains deferred for lack of a reader; physical fault injection and the overnight soak are not claimed complete.

## Pending-alert Ignore regression — 2026-09-22

The user reported recurring Samsung NEW SCENT cards with Travel Watch off and no apparent change to the saved ignore entries. A host regression reproduced a narrower UI defect: a detection arriving while the frozen Quiet Alerts target was open armed a Home card, and successful Ignore left that card pending. Both Ignore paths now recheck the pending card and clear it when suppressed; ordinary Ignore also updates Watch eligibility immediately. Unrelated eligible cards remain visible. No identity algorithm, saved-state layout, rule, count or category-alert setting changed.

The regression failed against the previous UI at the pending-card assertion, then passed with the fix. Coverage includes repeated sightings of two ignored Samsung identities, idempotent Ignore, preserved observation/meal processing, another identity remaining eligible, and the Tag Watch Ignore path in both orientations. All 11 host CTest targets passed with ASan/UBSan; LeakSanitizer was disabled because of the documented ptrace restriction. The pinned ESP-IDF release build passed: **1,212,544 bytes**, SHA-256 `1df3ada8e316afa6be55c5a4c90b0882441f3d91f0f5170c728f7e6961478726`.

After the update, the user reported that the display stays at **2 Likely** with **no NEW SCENT pop-ups** during the requested Travel Watch-off check. This confirms the expected count-versus-alert behavior in the observed session. The user did not specify the elapsed duration; long-duration and address-rotation acceptance are not inferred from this check.

After the user confirmed SAFE TO REMOVE, the complete 24 KiB NVS partition was privately backed up and all five record CRCs validated. It contains 10 distinct Samsung and two Apple ignored identities; the identity key matches the previous update. The first reconnect after the backup timed out before any application write; a normal bootloader reset restored communication. Only the application at `0x10000` was then written, its data hash verified by esptool, and the board restarted. NVS, calibration and SD contents were not erased. The subsequent user check reported no recurring pop-ups, as recorded above. The multiple saved Samsung identities are consistent with address changes or other matching devices, not proof of either explanation.

## Long-running Samsung Ignore follow-up

The user subsequently reported another Samsung NEW SCENT after many hours, despite only two known tags nearby. The earlier short successful check therefore does not close the long-running report. The installed queued-card fix is unchanged; no board reset, flash or alert-setting change was performed for this follow-up.

Added and passed a regression through the production BLE parser, Samsung FD5A rule, classifier and UI in both orientations. Two ignored identities remain quiet over 24 hours of simulated uptime, including long gaps that expire classifier evidence. Changing both addresses after the old observations expire produces alerts while the recent count is still 2 Likely and the original two ignores remain saved. This verifies the mechanism and absence of a time-based ignore expiry; it does not prove the source of the user's live signals.

The same regression exercises the existing Alert Types control: Samsung category OFF clears the pending card, retains enabled detection and observation counts, suppresses ordinary alerts and Samsung Watch timers, and leaves AirTag alerts eligible. The companion suite passed under ASan/UBSan with the documented LeakSanitizer/ptrace exception. No longer physical soak, pairing-key resolution or reliable association across rotated identities is claimed.

## Samsung payload identity/state — 2026-09-22 local

All 12 ASan/UBSan CTest targets pass, including the new Samsung regression (785 assertions; 787 with two rendered evidence pages). LeakSanitizer was disabled under the ptrace sandbox. The regression covers parser boundaries, same-ID MAC changes, different IDs, legacy ignores, a saved-companion reload, 24 simulated hours of Ignore, connected-state gating, Follow Scent and Travel Watch. Both orientations of the Samsung evidence page were rendered and visually inspected.

The Samsung-seeded mutation run passed 1,952,182 inputs in 45 seconds with address/undefined sanitizers. This is bounded deterministic mutation testing, not exhaustive validation. The pinned ESP-IDF release/debug/test/demo builds all pass:

| Profile | Bytes | SHA-256 |
|---|---:|---|
| release | 1,214,576 | `e6b17931d66f5a689e0ff8c6bddeb0f58303030d8824d43f1fb10e57a0de803e` |
| debug | 1,352,336 | `da8af060d0150d67304f83e36741b7cfa26d32840d22284a27deb1cf4ef399c1` |
| test | 1,321,904 | `ff40a68d8fc8fe47688b2f5b0692f7525b03bc8e2aa81c0f5044f8541de10437` |
| demo | 1,234,416 | `1b5b780989d10750871495c972929bc138f9ace9230fdb823aa63094a98ee161` |

The release ELF allocates 28,048 bytes to the engine and 7,968 to the UI view; the engine grows by 2,048 bytes for bounded metadata/state tracking. No dynamically growing identity map or additional NVS record is introduced. Live heap, packet loss, owner-state transitions, long-run physical identity behavior and SD soak require board validation. The release image above was subsequently installed as recorded below.

### Samsung update installation

After the user confirmed SAFE TO REMOVE, read and validated a private 24 KiB NVS backup: five of five record CRCs passed, with 15 saved ignores, SCOUT and the Brass Collar present. Wrote only the application at `0x10000`; esptool verified its data hash and reset the board. Installed release: `e6b17931d66f5a689e0ff8c6bddeb0f58303030d8824d43f1fb10e57a0de803e`, 1,214,576 bytes. No NVS, partition table or bootloader write was performed. Private backup and flash receipts are retained outside the repository. The user subsequently confirmed the post-boot/SD OK check. No exact Samsung REPORTED state or IDENTITY value was supplied, so supported-payload and owner-state behavior are not yet field-verified. Live memory and long-run physical behavior remain open.

## Wi-Fi-window memory headroom correction — 2026-09-22 local

Reordered only the volatile `Detection` members and added a compile-time RAM budget. Target ELF/debug-type inspection verifies `Detection` 96 → 80 B, Engine 28,048 → 26,000 B and View 7,968 → 7,408 B (2,608 static DRAM bytes recovered). No NVS structures or JSON serialization fields change; no buffer, stack or detector capacity is reduced. The current-free-heap warning remains 80,000 B.

All 12 ASan/UBSan CTest targets pass, including Samsung identity/state, storage serialization/export, saved-state compatibility and guarded UI rendering. LeakSanitizer remains disabled in the ptrace sandbox. All four pinned ESP-IDF profiles build:

| Profile | Bytes | SHA-256 |
|---|---:|---|
| release | 1,214,608 | `ba344a3ef1de8bddc9ef7702ad6670aa2cb45bd3b9c7fb9193c151b4d651e375` |
| debug | 1,352,320 | `0990e2f82e8094bfc6150c4e4eaf40b1da5a8719880a8ba77b1e27f46b999850` |
| test | 1,321,888 | `8e0099afe89e8488c4b35972526dfe9d16f3b5e9ea7feca9b9c4ceb1b3f8d48b` |
| demo | 1,234,448 | `19f302695784dae60e165892e5952d9aca94da86305de83411f513ec4a36ac1f` |

Static checks, generated-data validation, formatting and patch whitespace checks pass. The release image above was subsequently installed after safe eject. The running board was not reset during preparation.

### Memory-layout update installation

User-confirmed safe eject preceded a private 24 KiB NVS backup (5/5 record CRCs valid, 15 saved ignores). App-only flash: `ba344a3ef1de8bddc9ef7702ad6670aa2cb45bd3b9c7fb9193c151b4d651e375`, 1,214,608 bytes, written at `0x10000`, esptool data hash verified and hard reset completed. No NVS or boot metadata write was performed. Release/debug/test/demo software checks passed before flashing; live heap recovery, SD remount and sustained warning behavior remain to be confirmed on the board.


### Idle display verification — 2026-09-23

All 13 ASan/UBSan CTest suites pass (LeakSanitizer disabled in the ptrace sandbox). New coverage includes exact timeout boundaries, Never choices, backward-clock recovery, armed Watch off suppression, awake calibration/self-test/export/eject, first-touch/BOOT consumption, selected breed/outfit rendering, guarded tiles in both orientations, DVD reflections, reduced-motion relocation and alert wake exclusions. Production NVS tests additionally read CRC-valid V1/V2 appearance envelopes with nonzero legacy padding, preserve saved state, write/reload V3 defaults, and reject malformed current/future records without rewriting NVS. Existing interrupted-save tests include custom timeout settings.

All four pinned ESP-IDF profiles build:

| Profile | Bytes | SHA-256 |
|---|---:|---|
| release | 1,217,104 | `7bb86dfccc28a22480a5126128dd451ef6248e99529f4b29dab0d98891486c0a` |
| debug | 1,355,824 | `fd5b2f9055b5b742817ebbe651c5891a634ec79d4dbe2fd6b1fa5410c8989308` |
| test | 1,325,376 | `a49d551b106554ae001f238faf655d6c552c23b1e5004b2678da79a46f0e5cbc` |
| demo | 1,236,944 | `0e16755a36773462202cd0ae8e957d86d49a1003db104b132680f564b460f961` |

Static passive-API/bounded-callback checks, generated signatures/assets validation, Ruff and C++ formatting pass. Target ELF sizes: View 7,440 bytes (+32), Engine 26,000 bytes (unchanged), DMA tile 10,240 bytes (unchanged); Appearance stays 12 bytes. Display-off uses ST7796 DISPOFF plus zero PWM duty, retaining touch polling and radio/storage tasks; wake refreshes display RAM before restoring backlight. Host simulator includes idle preview/virtual time controls and backlight simulation. Physical operation and heap measurements remain pending; this does not close the overnight soak.


### Idle display installed — 2026-09-23

After the user confirmed safe SD eject, the first backup attempt connected but failed after switching to 460800 baud, before any firmware write. A retry at 115200 baud completed a CRC-verified NVS backup, but its application write was interrupted partway through. The user requested a recheck/reload. The expected USB port was present and no flasher remained active; a fresh 24 KiB NVS backup again validated all five records and 15 saved ignores.

The complete screen-saver application was reloaded at 115200 baud: 1,217,104 bytes at `0x10000`, SHA-256 `7bb86dfccc28a22480a5126128dd451ef6248e99529f4b29dab0d98891486c0a`. Esptool reported **Hash of data verified** and completed the RTS hard reset. The successful backup is `hosyond-nvs-before-idle-display-id-20260923-171914.bin` (private, outside the repository). NVS, bootloader and partition table were not flashed. No serial monitor was opened after reboot. The new firmware migrates appearance preferences on startup; physical SD OK, saved dog/outfit, preview/wake and timeout behavior still need user confirmation.


### Ordinary-alert idle countdown correction — 2026-09-23

Ordinary eligible detections no longer reset interaction-based idle deadlines. A separate five-second display wake cannot be extended by overlapping cards and may start only once every thirty seconds; after it ends the original dim/saver/off schedule resumes. Following warnings retain their continuous awake behavior. The DVD bounce follows its original time base through temporary alert wakes. Touch/BOOT still begin a new full idle countdown. No detection, sound, ignore, SD logging or saved preference policy changes.

All 13 ASan/UBSan CTest suites pass with LeakSanitizer disabled in the sandbox. New regressions run enabled AirTag and iBeacon sightings every second for sixteen minutes each, keep card/meal timestamps active, verify timed idle stages between bounded wakes, preserve bounce continuity and count every sighting. They also cover a touch during an alert and a following warning arriving during the ordinary-alert cooldown. Static checks and C++ formatting pass.

All four pinned ESP-IDF profiles build:

| Profile | Bytes | SHA-256 |
|---|---:|---|
| release | 1,217,024 | `283f0f09d23c9968f7518b2a49f4b55bb140787027c7629a843d13c1ebd2adc8` |
| debug | 1,355,936 | `4e6e5c377ff79d5890facf5865ff7f885afd55b518650acc8ec19033fca44404` |
| test | 1,325,488 | `e945d6a568743b9cfeaf2dd9385ff043768e2b66478764eab0b6f16859ca1fa0` |
| demo | 1,236,864 | `36b5494c3e6b3a52287c5e859ac0750fb60a11791b16c456fa1199ee40b7294a` |

The adjustment adds one 8-byte volatile deadline to View; no NVS layouts or frame buffers change. Physical idle behavior with frequent real tags remains pending installation and user confirmation.


### Ordinary-alert idle correction installed — 2026-09-23

The user confirmed a fresh safe SD eject. A private 24 KiB NVS backup (`hosyond-nvs-before-alert-idle-id-20260923-185235.bin`) validated all five saved record CRCs and 15 ignored identities. The application was flashed at 115200 baud to `0x10000`: 1,217,024 bytes, SHA-256 `283f0f09d23c9968f7518b2a49f4b55bb140787027c7629a843d13c1ebd2adc8`. Esptool verified the written data hash and completed the RTS hard reset. NVS, bootloader and partition table were not flashed; no serial monitor was opened afterward.

Installed behavior: ordinary enabled alerts provide a five-second display wake at most once every thirty seconds, without renewing idle deadlines. Following warnings keep the display awake, and armed Travel Watch still prevents display-off. All 13 sanitizer suites and four build profiles passed before installation. SD OK, saved dog/outfit and actual dim/saver behavior amid frequent nearby tags remain for user confirmation; software verification is not substituted for this physical check.

## 64-entry Ignore list, SD backup and dark silhouettes — 2026-09-23

Software verification: all 14 ASan/UBSan host targets passed. Production NVS migration checks were rerun after splitting boot readers to bound stack use; they also cover old monolithic V1/V2 records and malformed record lengths. Companion V1 migration preserves the full 16-entry list, outfits and discovery records at every simulated save interruption, and all 64 entries survive subsequent save/reload. The SD backup target covers 64-entry round trips, deletion/empty snapshots, wrong keys, unchanged saves, short writes and I/O failures, every torn length and every single-byte corruption of the newest generation. These model file and API boundaries, not physical flash/FAT power-loss internals.

All four pinned-SDK profiles build:

| Profile | Bytes | SHA-256 |
| --- | ---: | --- |
| release | 1,283,744 | `bcb4c29fd9e06ad472ec3f72360b5bc41d3f47af99cb767abb9eb66b127b7330` |
| debug | 1,423,776 | `67b664c7481db5e594aabbe4f6a35e64d273d4d10ad37eb58b55fd2ff9026ffb` |
| test | 1,393,312 | `da160ff3286cc96d0011e0c6f5dc9a7d37d8ea0ee1ed3d1569d4c3905d2b657d` |
| demo | 1,303,584 | `de00a2e2fd791eb23c1befa40285809d866e3f69e9418fffc4918d48fb771a72` |

Static passive-API checks, generated-data validation, Ruff and modified C++ formatting pass. Target sizes: Companion 1,048 bytes (was 728), View 8,096 (was 7,448), State 1,816, two state snapshots 3,648, Engine 26,000 and DMA tile 10,240 unchanged. The five companion copies account for 1,600 added static bytes, plus small status/digest bookkeeping. No scan buffers or stack allocations were enlarged. Release disassembly shows `load` reserves 1,856 bytes and its separate `read_generation` helper 2,640; migration buffers return before NVS persistence begins. Field heap/stack measurement is still required.

Rendered contact-sheet review covered Beagle/Dalmatian/Husky against Midnight, Daylight and black saver backgrounds. The light rim preserves black fur and reveals ears/tails. The live local preview was rebuilt and its 64-slot used/free display and demo-only SD backup page were inspected. Full physical scene/pose/contrast acceptance remains pending.

Installation completed at `20260923-192007` UTC after fresh SAFE TO REMOVE confirmation. A private 24 KiB NVS readback validated all five record CRCs, Companion V1 with 16 / 16 used Ignore entries, SCOUT and the Brass Collar. Only the 1,283,744-byte application at `0x10000` was flashed at 115200 baud; esptool verified the hash and reset via RTS. Installed SHA-256: `bcb4c29fd9e06ad472ec3f72360b5bc41d3f47af99cb767abb9eb66b127b7330`. NVS/partition table/bootloader were not flashed. No serial monitor was opened after reboot; on-device 16 / 64 count, SD backup completion and dark-ear visibility await user confirmation.

## Responsive startup storage and backup retry — 2026-09-23

The user reported WAITING FOR BACKUP and QUEUE BUSY while SD OK was shown, with SD WRITES/NVS SAVES/NVS ERRORS all zero. The old worker published mounted status and then performed full-history recovery and a free-space census before reading any queue message. This permits exactly that stalled-looking state; no live serial trace was collected. The user later confirmed SAFE TO REMOVE before the correction, demonstrating that the old worker eventually handled Eject.

Production history recovery now processes at most sixteen lines/directory entries per step, services queued NVS saves and Eject between steps, and yields once per batch. A deferred backup reuses the existing immutable snapshot pool; later successful saves replace the retained snapshot without losing manual intent. Startup observations are counted as dropped instead of filling the control queue. The log opens after validation; the initial backup precedes the first free-space census. SD CHECK and checked-row/file counts expose progress, a manual request retries automatically after queue rejection, repeated taps coalesce, and a completion counter replaces stale QUEUED notices with the actual result. Task creation failure is explicit. Eject closes the startup reader before acknowledging safety.

All 15 ASan/UBSan host suites pass (41.40 seconds), including the new production recovery test: 4,000 rows, batch bounds, torn-tail repair, complete corruption retained, cancellation without repair, empty/missing directories and injected close/truncate/fsync errors. Snapshot replacement/release, retained backup intent, repeated taps, success feedback and export deferral are covered. Existing ignore-file authentication, migration, alert suppression and idle-display regressions pass. Static invariants, generated-data checks, Ruff and modified C++ formatting pass. A rendered portrait/landscape contact sheet verifies the checking count and retry text fit. The local preview was rebuilt.

| Profile | Bytes | SHA-256 |
| --- | ---: | --- |
| release | 1,285,248 | `d558fd91123c228675acb722451fea36889dbe11c4020c2f87130bbb2dbe8251` |
| debug | 1,425,712 | `b78884771fd5039cae974ce49f832e8e47999be7bfba8f07d91fa66b149053e4` |
| test | 1,395,280 | `cff8b90a51437b2970193b2ae7399f381af0ad3c7784eaecb0e6e108a5bfd313` |
| demo | 1,305,088 | `a2390644bf322e0ce9b70c05720e902ffe8ffd08cf86303afa488092f8602a79` |

Target View is 8,112 bytes (+16), storage Health 80 (+16), State 1,816 and two snapshots 3,648 unchanged. Engine (26,000) and DMA tile (10,240) are unchanged. The worker retains its 10,240-byte stack; release task frame is 1,040 bytes versus 704 before, with recovery step reserving 64 bytes. No new task, growing buffer or third snapshot is introduced. Physical startup duration/progress, post-check logging and SD backup success remain acceptance checks; the host tests do not reproduce card/controller latency.

Installation receipt: after the user's fresh SAFE TO REMOVE confirmation, a private 24 KiB NVS backup (`hosyond-nvs-before-storage-queue-id-20260923-193714.bin`, UTC receipt stamp) validated all five record CRCs. Companion V2 has capacity 64 with 16 used entries. Canonical ignore identities and the identity key match the pre-expansion backup; settings, appearance, SCOUT and the Brass Collar are retained. The committed generation advanced 1881 → 1891, confirming the previous worker eventually completed saves after the reported wait.

The 1,285,248-byte application at `0x10000` was installed at 115200 baud with SHA-256 `d558fd91123c228675acb722451fea36889dbe11c4020c2f87130bbb2dbe8251`. Esptool verified the written data hash and completed the RTS hard reset. NVS, bootloader and partition table were not flashed; no serial monitor was opened after reboot. The user subsequently confirms SD CHECK is shown in the top bar and the checked-record count increases. This verifies visible startup progress on the board; completed backup and resumed SD writes remain pending. Independent SD-file verification remains deferred without a card reader.


## Displayed alert selection and AirTag Ignore regression — 2026-09-23

A regression using the production BLE parser and classifier reproduces the old failure: render an Apple Find My card, process a different Find My sighting, then tap the visible Snooze / Ignore button. The old code selected the later sighting. Corrected code captures the last rendered target in the existing action snapshot before accepting touch; the menu remains frozen while later events arrive. No extra Detection copy or ignore migration is added.

All 15 ASan/UBSan suites pass (41.33 seconds). Added coverage in both orientations includes the reproduced inter-frame race, further arrivals while the menu is open, 120 repeat scans of an ignored Apple identity, unchanged count inclusion, a different eligible signal, saved-companion reload, demo isolation, changed-address/type limits, an expired/replaced backing card, and no invisible action after a rendered Home frame without an alert. Existing storage, migration, Samsung, Watch and idle-display checks pass. Static invariants and modified C++ formatting pass. Rendered landscape/portrait cards, normal/already-ignored menus and Apple detail pages were inspected for fit.

| Profile | Bytes | SHA-256 |
| --- | ---: | --- |
| release | 1,286,384 | `27857be6757d59f174dc91fa78f2ccba8fd88f601ca29ecd9e0a308c11a3f421` |
| debug | 1,427,328 | `10096e053cc89abb9c782499a6f117eda6b6cb2938c4da944482a111faea1aab` |
| test | 1,396,880 | `15bd6456739e6e506da1b138137535ad34d9f3ff6053db5f3d70a368b185d76e` |
| demo | 1,306,208 | `323c990fb3b79f34c9a3b67eb45a8b72c25fc24d17def87bdef85f3ed5578ee3` |

Target View is 8,120 bytes (+8); Engine remains 26,000, State 1,816, two storage snapshots 3,648, storage Health 80 and DMA tile 10,240. ID presentation hashes at most once per frame rather than once per rendered tile; short codes do not replace full keyed matching. No NVS format, SD format, radio buffer, detector rule, alert-category setting or heap-warning threshold changes. Installation and on-board identification of the recurring AirTag signal remain pending; rotation continuity is not implemented.


Installed after fresh safe-eject confirmation (UTC receipt `20260923-201443`): private 24 KiB NVS backup `hosyond-nvs-before-airtag-selection-id-20260923-201443.bin` validated all five record CRCs, Companion V2 with 17 / 64 used ignores, SCOUT and the Brass Collar. The identity key matches the previous backup. Compared with the pre-storage-correction backup, used entries increased from 16 to 17; this confirms an additional saved ignore, not which radio signal caused the user's recurring card.

Only the 1,286,384-byte application at `0x10000` was flashed at 115200 baud, SHA-256 `27857be6757d59f174dc91fa78f2ccba8fd88f601ca29ecd9e0a308c11a3f421`. Esptool verified the written hash and completed its RTS hard reset. NVS, bootloader and partition table were not flashed; no post-reboot serial monitor was opened. On-board acceptance remains: matching local ID on the card/action menu, quiet subsequent scans for that ignored identity, and comparison of any returning alert's ID/status. SD remount and backup completion after this reboot remain to be observed.


## Stable local emulator layout — 2026-09-24

The browser lab places the firmware display in a stationary center area, with independently scrolling hound/display and sample/signal controls on either side. Screen shortcuts sit below the display. Automatic scrolling after samples, care previews, idle previews, Tag Watch and signal readings is removed. Narrow windows stack the controls below the device.

At the normal 1280×720 browser viewport, measured canvas bounds and page scroll position remained identical after seven interaction groups: hunger, Flipper sample, category selection/send, saver preview, idle time advance, Tag Watch warning and signal/repeat controls. Lower controls were reached through the side panels without moving the center display. Responsive checks at 1024×768 and 390×844 found no horizontal page overflow; the temporary viewport override was reset. Browser error/warning logs were empty. A control inventory comparison preserves all 29 action buttons, 20 element IDs and 23 category options. JavaScript syntax and Git whitespace checks pass; the native simulator target rebuilt successfully and the local server responds on loopback port 8765. This change only edits the browser wrapper and documentation; no board update is needed.


## Zensical site and browser lab — 2026-09-24

The documentation now provides 38 navigable pages with Hound's Midnight theme and original dog artwork, preserving chronological evidence as engineering records. Five wrapper pages import their original sources. The canonical URL is `https://surveillancehound.dx.pe/`; the GitHub workflow builds the browser lab, checks native/browser parity, validates the site, and deploys the default branch through Pages.

The browser lab compiles the shared synthetic C++ simulator and firmware UI with pinned Emscripten 6.0.10. Each lab uses its own Web Worker, WebAssembly instance and RAM-only state. Native builds retain OpenSSL and device builds retain PSA; the browser uses Web Crypto for the same hashes. No radio access, USB access, persistent browser state or SD writes are introduced.

Validation passed: all 15 Debug host suites; 169 native/WebAssembly state and full RGB565 frame comparisons, including against the pre-refactor native worker; the same parity run and core/UI suites with ASan/UBSan/LeakSanitizer; Python/C++ formatting, static invariants and workflow lint; and a strict clean Zensical build with all 40 generated HTML pages checked for local links, assets and anchors. Browser checks exercised the embedded and full-window lab, hunger/meal and following-warning controls, and stable canvas bounds at 1440×900 without page movement. The session isolation test starts a second fresh WebAssembly instance.

These are software and local-browser checks. They do not establish radio accuracy, device power/memory behavior or public DNS/TLS/deployment status. The separate hardware acceptance gates remain open.
