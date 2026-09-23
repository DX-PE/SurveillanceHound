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

On the Hosyond, record byte-addressable free/minimum/largest heap after startup, through repeated detections and multiple 60-second NVS saves. Verify zero radio-init/save errors and inspect queue drops and task watermarks. Test simultaneous saves/critical sleep, SD logging/export and power interruption when card testing resumes; the two-snapshot limit must fail visibly rather than overwrite an in-flight save. SD testing has resumed with a user-confirmed 32 GB mount. Compare heap with the card absent and with active logging, then through export and safe eject. The short run does not replace the eight-hour soak or the minimum-heap acceptance gate.

## Tag Watch

Host: verify ten-minute/eight-minute-bin boundaries, packet-flood resistance, gaps, stale/future/out-of-order timestamps, supported BLE tag families versus generic iBeacon/weak clues, capacity, distinct keyed address/type/category identities, acknowledgement, confidence/detector/category gates, saved Ignore integration, Snooze expiry, pause/reset and real/demo isolation. Render warning phases across both orientations, all themes and scenes with tile guards, and verify steady red under reduced animation. Feed production Samsung FD5A classifier output through the full UI event path.

Simulator: choose each tag under Tag Watch and preview the warning; confirm the synthetic clock advances ten minutes without meals/saved XP. Review, ACK, Ignore, Snooze and Stop must stay usable. Both the Home WATCH control and first Settings page TAG WATCH control must be reachable.

Physical (pending): arm while moving through several locations with controlled known tags, compare against stationary and passing-tag runs, include address rotation and crowds above sixteen identities. Record false positives/negatives without claiming movement from reception alone. Check red/normal timing, text contrast, dog visibility, touch/Stop, steady mode, panel inversion and boot-off behavior on the Hosyond LCD. Measure incremental heap and long-run watchdog/capture behavior. No SD test is required for this RAM-only feature; separately check Watch behavior during the resumed SD logging/export tests.

Tag Watch progress regression: deliver a packet timestamp one millisecond later than the old UI-frame time; confirm a genuinely future timestamp is rejected/counts as a clock skip, then a refreshed receive-time clock accepts it. The hardware loop must refresh its monotonic clock per dequeued observation. Verify expiry/filter resets increment once, counters saturate, manual restart clears counters, status navigation works in both orientations and page removal clamps correctly. Check empty/armed/paused/high-filter/ignored/snoozed hints, multiple same-category rows, ACK state, and time/minute/last-seen readability. The counters describe lost observation windows, not confirmed physical device departures or address rotations.

Do not reconnect a serial monitor during a user’s active Travel Watch test: opening the CH340 port caused a hardware reset during troubleshooting even without an explicit reset command. Opening serial before arming and keeping it open is required if a controlled run needs serial evidence; otherwise use the on-screen progress.

SD memory regression: preserve all 128 cached identities, independent clue corroboration, exact/prefix grouping, zero-time observations, expiry and cache eviction after compacting the evidence table. Rebuild all ESP32 profiles with the controller connection limit of one applied to generated configurations as well as sdkconfig.defaults. On the board, collect current/minimum/largest heap, radio errors, SD writes/free space and storage stack headroom during logging and export. Verify the exported checksum manifest and saved log rows after safe eject before claiming storage acceptance.

SD export regression: the host `storage_export` target runs the production file-export path with a wrapper modeling FatFs writer/reader exclusion. It must reject the former simultaneous-open sequence, include the flushed current session plus older logs, reopen at EOF and retain all records after subsequent writes/exports. Inject malformed records, hash and artifact sync failures, existing output directories, failed log reopen and an absent writer; no failed attempt may retain a completion manifest or overwrite an earlier export. On hardware, confirm EXPORTS increments, SD WRITES resumes, SD ERROR stays absent, and heap/stack remain adequate. Safely eject and validate the actual export with `tools/validate_export.py` before claiming SD export acceptance.

Export/save feedback regression: both Data and Tools must show immediate export progress followed by a persistent row-count success or an explicit failure. Tap repeatedly while pending, reject a queue submission, test zero rows and a prior completed export, then retry. Demo must never claim real files were written. Repeat in portrait and landscape without covering navigation or adjacent controls. Exhaust the two save snapshot slots during slow storage, verify retry after one second with the latest settings/progress, and ensure the waiting message clears on queue acceptance while unrelated critical notices remain. On the board, verify subsequent NVS persistence across a controlled reboot; queue acceptance alone is not a persistence check.

Hound-care regression: verify five-minute boundaries, preserved partial intervals, zero/100 saturation, long uptime and backward-clock rebasing; a meal must reset the needs timer and rescue low stats without altering XP/cooldown rules. Non-meals, demo events and disallowed low-confidence clues must not feed saved state. Resetting progress at long uptime must start a fresh needs timer. Check each breed with low fullness alone and low mood alone, both orientations, paused sleep and reduced animation. Confirm visible tears/slow movement, eaten-food recovery and bounded hops that keep the name and controls clear. Demo decay, petting and snacks must not change saved needs or XP. On hardware, verify saved needs after a controlled reboot and measure heap with SD logging during animations.


## SD and saved-progress checks — 2026-09-22

The combined hound-care, export/save feedback and safe-eject update is installed on Hosyond (release `8c7f238516184b4421d3009979428dbf9690443eed306e95a6638e66e12fa6a8`). Before updating, the saved state had SCOUT/beagle, Trail Bandana, 474 XP, 93 meals, fullness/mood 100/100 and six ignores. NVS was backed up privately; all five selected-generation record CRCs validated.

1. Verify normal boot, dog/outfit/settings/ignores and SD OK. Export history: observe pending then completed row count. Verify SD WRITES advances after exporting.
2. Change a visible preference (for example brightness from 100 to 90) and note needs/outfit/ignore count. On Diagnostics page 3, NVS SAVES must advance and NVS ERRORS remain zero. Wait for the save before the controlled reboot.
3. Eject SD and wait for SAFE TO REMOVE. Record failures rather than removing during EJECTING. Once safe, read the card on a computer. Validate every complete export with tools/validate_export.py; older incomplete exports without a manifest must not be reported as successful. Retain aggregate counts/checksum results, never private raw rows, in repository test evidence.
4. Power off, reinsert the card, and restart. Verify SD OK, continued log writes, and the noted settings/progress/outfit/ignores. Needs can change during powered-on time; no offline decay is applied. Travel Watch and temporary snooze intentionally clear on reboot.
5. Interrupted-save host coverage executes the production NVS loader/writer with both immediate-write (pinned SDK) and buffered-commit models. Cuts at all twelve set/commit boundaries and failures before those calls must recover a whole old/new generation. Physical power-cut/flash-page fault injection remains a separate acceptance test; these host models do not emulate flash hardware.

After these checks, prepare the eight-hour overnight run with the card mounted, normal scanning enabled and stable USB power. Do not open serial during a manually armed Watch; this adapter has reset the board on open. Record start/end uptime, heap/minimum/largest, SD writes/exports/errors, NVS saves/errors and UI responsiveness. Controlled moving/stationary tag tests are planned for the following day.

Current physical results: 581-record completion and SAFE TO REMOVE confirmed. Private NVS readback verifies retained preferences/calibration/outfit/ignores and new progress saved after reboot. Safe eject/power-off/reinsertion/reboot returned SD OK and the saved outfit according to the user. After remount, the user confirmed increasing SD WRITES, NVS SAVES: 1 and NVS ERRORS: 0. No reader is available, so independent card-file validation remains deferred rather than passed. The final guarded firmware is installed and hash-verified.

## Ignore with detections arriving during review

Host: queue a second Samsung sighting while its Quiet Alerts menu is open, then Ignore. The pending Home card must clear immediately, including when the ignore already exists. Repeat both ignored identities every 30 seconds with Travel Watch off; check that observations and eligible meal animations continue while ordinary alerts stay quiet. A different, unignored identity must still alert, even if it arrives during review. Run in both orientations, and verify the Tag Watch Ignore path also removes a queued ordinary card.

Hardware: with Travel Watch off and both nearby Samsung identities ignored, observe for at least five minutes and confirm no NEW SCENT card returns for those identities. Counts may still show the observed signals. If an alert recurs, record its category, whether Ignore adds a slot or reports a full list, and whether it returns immediately or after a later sighting. Address rotation is a separate limitation; do not silently mute all Samsung devices or merge identities to hide a failure.

Long-run Ignore regression: feed two real-parser Samsung FD5A identities through the classifier/UI at simulated hourly checkpoints through 24 hours, with both orientations and Ignore saved in the active collection. They must remain quiet without a timer expiry. Replace the addresses after the 90-second count window; verify alerts are eligible again while counts return to two, without changing the old ignores. Exercise Samsung Alert Types OFF and verify continued observations/counts, no Samsung Watch qualification, and unaffected AirTag alert eligibility. This is simulated time and identity substitution, not proof that two captured broadcasts belong to the same physical tag.

## Samsung identity/state acceptance

- [ ] Check both physical Samsung tags in Log → More: record supported broadcast-ID vs radio-address fallback, and reported state without copying raw identifiers.
- [ ] Confirm existing saved settings/outfit/ignores survive the app-only update. A current payload ID may need its own Ignore if an older address entry no longer matches.
- [ ] Ignore a supported current ID; confirm no card/sound through a MAC change while that broadcast ID is unchanged. Distinguish a changed broadcast ID from an Ignore failure during the hours-long run.
- [ ] Leave a second offline tag unignored; confirm it can still alert while the ignored/owner-connected tag stays quiet. Check connected→offline and offline→connected changes, ordinary pending cards and Travel Watch resets.
- [ ] Confirm counts/logging/earned meals continue for owner-connected reports. Check private logs/exports contain no raw Samsung broadcast IDs.
- [ ] Check heap/largest block, capture drops and SD writes with both tags and the AirTag. Complete the separate overnight soak and controlled rotation tests before closing roadmap item 23.


## Idle screen saver acceptance

1. After safe eject and installing the idle-display build, confirm the saved hound/outfit, inversion, ignored devices and SD OK. Settings → Display → Display options → Screen saver / Timeouts should initially show 2/5/15 minutes.
2. Preview: the chosen dog/outfit bounces off all edges against black; touch once returns to the prior screen without activating its hidden control. Check portrait as well as landscape. Reduced animation should move to a new position every 15 seconds.
3. With Travel Watch off and no eligible alerts, observe dim at 2 idle minutes, saver at 5 and true backlight/display off at 15. Touch or BOOT once should wake at the saved brightness; normal actions should work on the next press. Change a timeout and confirm it survives safe reboot.
4. Arm Travel Watch, wait beyond screen-off timeout, and confirm the saver remains visible. An enabled ordinary alert should wake the dashboard for at most five seconds and then return to its prior idle schedule. A following warning must remain readable while active, even within the ordinary-alert cooldown. Ignored/snoozed detections should not wake it. Check true display-off alert wake with Watch off as well.
5. Confirm SD WRITES and radio counts continue increasing across display idle states. Compare current/minimum/largest heap; no new frame allocation is expected. Calibration/self-test and active export/eject must remain visible. These checks are pending on hardware.

6. Generate repeated enabled AirTag/iBeacon sightings, including overlapping cards more often than every five seconds. Without touching the screen, confirm the dim/saver/off deadlines still advance and each ordinary wake ends within five seconds, with no new ordinary wake until thirty seconds after its start. Sightings, sounds and logging remain subject to the existing alert/ignore settings. Tap during a visible card to verify a fresh full idle countdown; check ignored/snoozed events remain quiet. The adjustment is installed; these physical checks remain pending.

## Expanded ignores and SD backup acceptance

Host checks: fill all 64 entries; reject a 65th without eviction; remove entries on the final page; preserve a full list after save/reboot; migrate a real-layout 16-entry V1 envelope at every interrupted NVS write boundary; reject invalid legacy records without rewriting them. SD tests cover round trips, wrong/zero keys, per-board filenames, unchanged saves, deletion snapshots, short writes, open/read/flush/sync/close failures, all truncated lengths, every corrupted byte, trailing data, and two damaged copies.

On Hosyond, after safe eject and app-only upgrade, verify the previous ignore count, SCOUT/outfit and settings, 64-slot capacity, and no fresh alerts for unchanged ignored identities. Open Ignored scents → SD backup and confirm SD BACKUP SAVED with the expected count. Add/remove an entry and verify the backup count follows; Eject SD must finish before card removal and later internal saves must report SD EJECTED with no card access. Repeat without a card and in demo. Reboot normally and check the current internal list wins over older SD state. Measure HEAP/LARGEST/storage stack during backup and radio transitions; threshold stays 80,000 bytes. No reader is currently available, so independent physical file recovery remains deferred.

Visually compare Beagle, Dalmatian and Husky on Midnight, Sunset, Daylight, all scenes and the black saver. Check dark ears/tails, selected outfit, walking/eating/crying poses, portrait/landscape, saver corners and Travel Watch red background. Rim must move with the dog and keep the original fur colors.

### SD startup responsiveness regression

Validate a 4,000-row historical file plus torn tail incrementally; each call may consume at most its configured line/directory-entry budget, allowing the caller to process controls. Confirm complete-row corruption is retained, short tails repaired, cancellation closes every reader without repair, and close/truncate/fsync failures cannot return SAFE TO REMOVE. Exercise deferred snapshots replacing older state without a third allocation, retained manual-backup intent through queue rejection, repeated-tap coalescing, final success acknowledgement, demo isolation and export deferral during SD CHECK.

On the board, a large card should show SD CHECK plus advancing record/file counts, NVS SAVES should advance while checking, and Eject must finish without waiting for the whole history. After checking, confirm SD BACKUP SAVED and increasing SD WRITES. If counts do not advance, capture the checking count, NVS count/errors and heap; mounting alone is not evidence of worker progress. Verify the 16 saved ignores and SCOUT/Brass Collar survive the correction. Independent card-file validation still needs a reader.


### Displayed alert selection / immediate AirTag repeats

- Host: draw one production-classified Find My sighting, ingest another before touch, and confirm Ignore targets the displayed ID in both orientations. Further arrivals during the action menu must not change its target. Verify repeats of an ignored identity remain quiet while another identity can alert and both still count. Verify saved-list reload, demo isolation, already-ignored feedback, expiry between render/touch, and no hidden button after a Home frame without a card.
- Board: compare the local ID on the AirTag alert with the ID on its Ignore menu; Ignore that sighting. If a card returns next scan, compare its ID and Log → More → Ignore status. Check whether used Ignore entries increase. This distinguishes changed/other radio identities from suppression failure; a short-code match alone is not conclusive because it is truncated.
- Confirm saved settings, Ignore entries, SD startup/backup and logging remain functional; measure heap/largest block with normal radio/SD operation. AirTag/Find My address rotation remains an explicit continuity limit.
