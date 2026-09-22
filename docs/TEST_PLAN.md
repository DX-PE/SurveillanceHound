<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Validation

Host CTest groups cover portable parsing, OUI/UUID/company/payload matching, confidence limits/fusion/expiry, malformed radio lengths, JSON escaping, RFC HMAC, pseudonym separation, deauth windows, legitimate multi-AP negatives, dedupe restoration, pet mechanics, demo isolation, deterministic generation/provenance failures, privacy exports, manifests and host log recovery. UI tests exercise onboarding and render all screens into guarded tiles.

`fuzz_smoke` runs 100,000 deterministic, project-generated inputs through Wi-Fi, BLE, Remote ID and JSON encoding with address/undefined-behavior sanitizers. This is a smoke campaign, not exhaustive fuzzing. `tests/fuzz/libfuzzer.cpp` is the mutation-fuzzer entrypoint; build it with Clang's `-fsanitize=fuzzer,address,undefined` and the portable core for longer campaigns. No corpus from another detector is used.

`check_static.py` rejects application calls to active scan, association, raw Wi-Fi transmit, BLE connect/advertise and HTTP APIs, and checks fixed callback enqueue paths. It complements GCC `-fanalyzer` and warnings-as-errors. It does not certify absence of every possible defect.

The optional `test` firmware profile enables runtime heap poisoning. Hardware validation is the manual procedure in `tests/device/CHECKLIST.md`; there is no mocked claim of hardware-in-loop coverage.

After firmware flashing, measure: boot under five seconds; complete scan cycle under six seconds; normal touch median under 150 ms; pet animation at least eight fps; queue drops under controlled load; minimum internal heap above the final threshold. Drive known lab advertisers and APs without connecting to discovered devices. Verify over-the-air passivity with an independent receiver. Log both missed packets and accepted observations.

Run eight uninterrupted hours with UI, Wi-Fi/BLE cycling and SD logging. Record current, resets, heap minimum, task stack minima, SD errors, queue high water and drop rate. Inspect resulting logs and validate an export after abrupt power loss. Retain build hash, test equipment, radio fixtures and limitations with the report.

## Hound display and alert controls

- Validate 480x320 landscape and 320x480 portrait on the physical ST7796, including color order and all four touch corners.
- Unlock rotation, change View, lock again, reboot: orientation and lock must persist. Calibration must work in either view without recalibration after rotation.
- Load a v1 save with XP, privacy choices, calibration and meal hashes. Upgrade must retain them and start locked in landscape.
- Settings → Alert types: disable a category, inject a qualifying lab observation and confirm it remains logged without pop-up or sound. Re-enable it and repeat; the confidence filter must still apply. iBeacon alerts start off.
