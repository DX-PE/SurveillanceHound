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

## Scent Book, wardrobe and quiet alerts

Host coverage: both orientations, all categories, six dogs and six outfit choices; locked/equipped gear, XP/discovery milestones, synthetic isolation, frozen ignore targets, 5/15/60-minute expiry, early resume, full ignore list, per-device/category matching and restoration.

On the display board, verify V1/V2/V3 upgrade and V4 save/reboot, including power cuts between each namespace write and the generation commit. Missing/corrupt V4 companion data must disable scanning rather than lose ignores. Confirm saved outfits, discovery totals and ignores survive reboot; snooze clears. Repeat a known signal and an unrelated signal in the same category: only the selected identity should be quiet. Verify audio and LED behavior, log writes and feeding during snooze. Check all accessory frames, both orientations and touch targets on the real LCD. Measure heap and stack headroom with the enlarged state/storage queue.

## Follow Scent and Atmosphere

Host checks cover selection without seeding historical RSSI, identity/radio/address-type/demo isolation, invalid and out-of-order readings, one-second history bins and bounded floods, stale/lost expiry, gaps, pause/resume, future timestamps, no added XP/log entries, navigation in both orientations and guarded renders. Appearance checks exercise all theme/scene/panel combinations, reduced-motion stability, speech frequency/off, petting reactions, demo preference isolation, legacy defaults and invalid saved values.

On E32R40T, verify V4 → V5 migration and V5 appearance save/reboot; cut power between each namespace write and the generation commit. V5 must reject missing or corrupt appearance/companion records rather than silently resetting them. Test real repeated BLE/Wi-Fi observations below the log emission interval, target absence, radio-window gaps, pause/resume and rotating addresses. Inspect the three themes/scenes in both orientations and measure frame time, free heap and task stacks. These tests do not infer distance or direction from RSSI.

## Panel inversion and Settings order

Host appearance checks cover upgrade of the old appearance record, invalid inversion values, immediate panel-command/save requests, demo isolation and restoration, and both orientations. Verify that entering Settings via SET or BOOT returns to the first page, with Wardrobe first, and that all six page groups and their submenus remain reachable. On hardware, toggle inversion on and back off, reboot after each saved value, and verify the selected polarity returns with calibration, theme, progress and privacy settings intact. Missing/corrupt records must still fail closed.

## Memory, recent-count and repeat-alert regressions

Host checks repeat two Samsung-category identities and one Apple-category identity through 100 update cycles: totals stay 2 Strong / 1 Likely. Check confidence promotion/demotion, overlapping categories on one identity, address type/radio separation, disabled detectors, alert mute, 90-second expiry, backwards time and cache capacity. These fixture counts do not imply that ambient hardware observations identify exactly three products.

Render feeding and non-feeding alerts in both orientations, compare the dog and sniff-control pixels against the same frame without an alert, then tap the compact Snooze/Ignore button. Check long evidence text remains inside the card. Validate low-heap recovery and priority of existing critical notices. Exercise storage snapshot exhaustion/reuse, independent copies and FreeRTOS-style byte copies of each payload kind.

On the Hosyond, record byte-addressable free/minimum/largest heap after startup, through repeated detections and multiple 60-second NVS saves. Verify zero radio-init/save errors and inspect queue drops and task watermarks. Test simultaneous saves/critical sleep, SD logging/export and power interruption when card testing resumes; the two-snapshot limit must fail visibly rather than overwrite an in-flight save. SD testing remains on hold; leave the card out for this memory check. The short run does not replace the eight-hour soak or the minimum-heap acceptance gate.
