<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Hardware bring-up

[Buy the tested Hosyond 4-inch ESP32 board](https://amzn.to/4d4VI5R) (affiliate link).

Target: LCDWiki E32R40T / Hosyond 4-inch ESP32-32E, 4 MB flash, ST7796 with 480x320 landscape default and 320x480 portrait option, XPT2046 touch. No PSRAM assumption.

| Function | GPIO |
|---|---|
| TFT CS / DC / SCLK / MOSI / MISO | 15 / 2 / 14 / 13 / 12 |
| Backlight / touch CS / touch IRQ | 27 / 33 / 36 |
| SD CS / SCLK / MOSI / MISO | 5 / 18 / 23 / 19 |
| Amplifier enable (low) / DAC | 4 / 26 |
| RGB red / green / blue (low) | 22 / 16 / 17 |
| BOOT / battery ADC | 0 / 34 |
| Reserved I2C SCLK / SDA | 25 / 32 |
| Expansion input-only | 35 / 39 |

Display/touch share SPI2 with separate CS lines. SD uses SPI3 at a conservative 10 MHz. Display uses 40 MHz and a single 10,240-byte DMA tile buffer: 480x8 RGB565 in landscape and 320x16 in portrait, byte-swapped in place. Validate frequency, color order, orientation, inversion and analog panel defaults on the actual unit. Display rotation changes MADCTL and touch coordinates together. A rotation lock and the chosen orientation persist in NVS; orientation changes are manual, with no IMU or automatic gravity sensing. Touch calibration is retained in canonical portrait coordinates so either view works without recalibration. Touch uses five-sample medians and a three-point affine transform; its calibration precedes region selection on first boot so the region buttons can be used accurately.

BOOT short press cycles basic screens after onboarding. Long press mutes. Initial calibration still requires working touch. One repeating red pulse indicates board/display initialization failure. Normal scanning uses a blue slow blink; meals use green; Medium alerts pulse green and High alerts pulse red. Confirm the physical channel mapping.

Battery mode defaults off with raw ADC only. Settings provides three meter-referenced ADC/voltage calibration points. Only a valid, explicitly enabled calibration displays voltage and estimated Li-ion percentage. Low warning is 3.5 V; five consecutive samples at or below 3.3 V request state-save, SD-flush and deep sleep. These provisional thresholds need characterization with the actual cell and load. BOOT wakes from sleep. No battery runtime estimate is provided. Before enabling battery operation, measure GPIO34 at three battery voltages, verify connector polarity and protection, and characterize low/critical thresholds. Use USB power for initial bring-up. The audio amplifier stays disabled between short effects and sound defaults off.

## Acceptance measurements

| Measurement | Result |
|---|---|
| 50 USB cold boots | Pending hardware test |
| Display and four-corner touch | Pending hardware test |
| Missing/full/interrupted SD | Pending hardware test |
| Controlled Wi-Fi/BLE capture loss | Pending lab |
| Minimum internal heap >=80 KB | After the overnight memory-fix run, user reports approximately 82 KB current and 72 KB minimum. Minimum target remains unmet; largest block and live write progress are being checked. |
| Eight-hour soak | Pending hardware test |
| Boot / screen-only / Wi-Fi / BLE / full-cycle current | Pending meter |
| Audio current and charging thermals | Pending meter |
| Three-point battery ADC calibration | Pending meter |

These acceptance checks have not all passed; the limited bring-up measurements below do not replace them. Record board revision, build hash, SD model, supply, sample sizes, measurements, failures and exact reproduction steps here when testing.

## Available USB C3

The receive-only ESP32-C3 harness was tested with the shared radio scheduler, parsers and rules. See [C3_RADIO_TEST.md](C3_RADIO_TEST.md). This does not replace any E32R40T measurements above.

## First Hosyond USB bring-up — 2026-09-22

The user replaced the C3 with the Hosyond 4-inch display board. USB enumerates as a CH340 serial converter (`1a86:7523`) at `/dev/ttyUSB0`, stable path `/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0`. ROM interrogation reports ESP32-D0WD-V3 revision 3.1, dual core, 40 MHz crystal and 4 MB flash (flash ID manufacturer `c4`, device `6016`). The panel/touch revision still requires visual confirmation; USB identification alone does not identify the LCD controller.

A full 4,194,304-byte pre-Hound flash backup was saved outside the repository at `/home/jascha/Documents/ChatGPT/Surveillance Sniffer/hardware-backups/hosyond-4inch-before-hound-20260922.bin`. Its SHA-256 is `445f6fcb210442777f04028f3ad2022bd1c7231b5d9907309d8514f863978315`. Retain it locally; it includes the board's original settings. The original partition map used NVS at `0x9000/0x5000`, OTA data at `0xe000/0x2000`, app0 at `0x10000/0x300000`, SPIFFS at `0x310000/0xe0000` and coredump at `0x3f0000/0x10000`.

The current release-profile build, including Dog Park and Follow Scent, was written at the standard bootloader/partition/application offsets. Esptool verified the written hashes. Application: 1,195,552 bytes; SHA-256 `34c0bdace02b3ce2cf288749b0b4445e035caf552ecaa8c62fb916670cd56f78`. Existing NVS was not erased. The ESP-IDF 6.0.2 bootloader loaded the application successfully and firmware reached SD initialization. The initial serial capture reports `sdmmc_card_init failed (0x107)`; SD is unavailable. Display appearance, three-point touch calibration and first-run completion require the user's physical observations. This is not a completed hardware acceptance test.

To restore the complete original image if needed, stop serial monitoring and use the pinned SDK's esptool:

```sh
python -m esptool --chip esp32 --port /dev/ttyUSB0 write-flash 0x0 "/home/jascha/Documents/ChatGPT/Surveillance Sniffer/hardware-backups/hosyond-4inch-before-hound-20260922.bin"
```

### Display polarity correction

The user confirmed that the application displayed, but reported a white background and blue-colored dogs in the default dark theme. That is consistent with inverted panel colors. The board initialization was changed from ST7796 `INVON` (`0x21`) to `INVOFF` (`0x20`); RGB/BGR ordering and orientation were left unchanged for this first correction. The release build passes. Corrected application SHA-256: `baa0d87111f1593cad1aef2a9ee50705b90202a3b6ecef98435ba854005dd613` (1,195,552 bytes). Only the application was replaced, so NVS settings/calibration remain intact. The user confirmed that the corrected colors look right.

### Saved inversion control and menu organization

Settings now opens on the Hound page: Wardrobe, Atmosphere, Scent Book, Ignored scents and Snooze/Resume. More names each next group (Display, Alerts, Data, Maintenance, Tools). **Settings → More: Display → Display options → Panel inversion** applies the ST7796 polarity command immediately and saves the choice. It defaults off on this unit. Demo has a temporary inversion preference; leaving demo restores the real preference. The host simulator complements RGB565 pixels to preview the panel behavior.

Appearance records upgrade from version 1 to version 2 within the existing V5 generation format. The frozen older record is still accepted with a valid CRC and version, its cosmetic fields are retained, and inversion defaults off. Current records remain mandatory; corrupt or missing records do not silently reset preferences. A 24 KB pre-upgrade settings backup is retained outside the repo as `hardware-backups/hosyond-before-inversion-setting-nvs-20260922.bin` beside the full original backup.

SD testing is explicitly on hold at the user's request. Keep the card out until the storage test step is requested.

The combined saved-inversion/menu update was subsequently flashed and hash-verified: application 1,197,328 bytes, SHA-256 `1c9c757370659b80762740c8a06d1913753a50463ed53b67f2c21d70206d1f36`. A 25-second reset/startup capture reached SD initialization with only the expected missing-card errors and no observed panic, watchdog warning or reboot. The user still needs to exercise the new inversion toggle and persistence on the physical screen. SD tests remain paused.

### Low-heap and repeated-alert correction — 2026-09-22

The user reported approximately 70,000 B free and a persistent low-heap notice, repeated Strong/Likely count increases around two Samsung tags and an AirTag, and non-feeding alerts covering the dog. The warning previously counted all internal-memory capabilities and wrote a notice that was never cleared on recovery. Diagnostics now explicitly measures `MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT`; the warning still uses 80,000 B of current free space and clears independently of the minimum watermark.

Unused connection-data pools were reduced without shrinking advertising-event buffers or radio queues. The storage queue now has eight small messages and two immutable save snapshots rather than eight full saved-state copies. The storage task still has its original 10,240-byte stack. NVS formats, calibration, appearance, pet progress and saved ignores are retained. SD remains absent and testing remains on hold.

A five-minute USB run with application SHA-256 `49bc639f71366299d6690470a979096f3dc5d20430a873bc6ec57642bf94dc1e` measured ten aggregate snapshots: current heap 86,276–86,856 B, SDK-reported minimum 79,924 B and largest block 77,824 B. Final free stack watermarks were UI 3,864 B / radio 3,092 B / BLE 1,332 B / storage 3,156 B. Both radios received; radio errors and NVS save errors stayed zero across the periodic saves. No crash or watchdog reset appeared. Strong stayed at 2; Likely varied from 0 initially to 2–4, rather than monotonically increasing. Ambient matches are not a controlled census of the user's three physical tags.

This is a short USB-powered, no-card test, not an acceptance pass. The reported minimum remains 76 B below the existing 80,000 B gate. The last sample recorded 10,831 Wi-Fi and 12,659 BLE callbacks with 10,250 combined queue drops; no calibrated capture-loss result is claimed. High ambient-load queue pressure, minimum-heap headroom with SD, power interruption, and the eight-hour soak remain outstanding.

The earlier compact-alert application (`f40710edaaa763682922f5de1962f1a1e7801e7eb95619c2adf018e825d7bf31`, 1,200,160 B) includes a four-pixel card-height correction to hide underlying status text. Esptool verified the application write; NVS was not erased. A follow-up reboot loaded the saved state and resumed both radios. In the 135-second follow-up capture, current heap was 86,244–86,824 B, the reported minimum was 79,896 B, and the largest block was 77,824 B. Radio and save errors stayed zero through two automatic save intervals. The minimum acceptance gate remains open. Aggregate serial evidence is retained in [the measurement record](measurements/2026-09-22-heap.txt).

### Tag Watch installation — 2026-09-22

The 1,205,520-byte release application, SHA-256 `955acae092fc7ddbda2b4fe68effd4a2049113468cb5c8839329fc85155dad7d`, was written at `0x10000` and hash-verified. A private 24 KB pre-update NVS backup was saved outside the repo. NVS, bootloader and partition-table regions were not erased; saved-state loading resumed radio operation. Travel Watch starts off and requires WATCH → Start Travel Watch.

A 45-second USB boot smoke test produced two health snapshots: current heap 85,668–86,248 B, minimum 79,304 B, largest block 77,824 B; no reported radio/save errors or observed panic/watchdog reset. The second sample had 1,286 Wi-Fi callbacks, 1,511 BLE callbacks and 1,191 combined queue drops; this is uncontrolled ambient traffic, not a capture-loss measurement. The capture does not cover a full periodic save interval or an armed ten-minute watch. The minimum-heap gate remains unmet, and physical flashing/contrast/touch, moving versus stationary tags, address rotation and long soak still require testing. SD work remains on hold.

### Tag Watch troubleshooting

The user reported ordinary tag alerts while Travel Watch had been armed for over ten minutes. The existing health output did not expose individual watch-window progress, so it cannot establish whether identity changes, gaps or a timing issue prevented qualification. Inspection found that the UI sampled its clock once per frame while receiving up to sixteen asynchronous radio observations: a later packet could then be rejected by Watch/Follow as being in the future. The receive loop now refreshes the clock per observation. Watch progress shows per-identity elapsed time/minute bins/last-seen age and gap/filter/clock counters; the ten-minute/eight-bin rule is unchanged.

A serial read intended to be passive captured `POWERON_RESET` and fresh boot counters. Opening the CH340 port apparently toggled the reset circuitry despite no explicit reset command, clearing the active watch. Avoid opening serial during a field watch. The new progress UI supports diagnosis directly on the display. This reset was introduced during troubleshooting; it does not explain the user’s earlier report.

The timing/progress release (1,208,912 bytes; SHA-256 `949622ebb4df9e2ef08b3e70baf09a9cf4fb0899a753c7c5514a9a9eb3c4c164`) was subsequently flashed at `0x10000` and esptool verified the written hash. A fresh private NVS backup was saved outside the repo. NVS, calibration and saved settings were retained; no SD action was performed. No serial monitor was opened afterward, to avoid disturbing the next manually armed watch. Physical progress/alert behavior awaits user feedback; no new boot-health/heap acceptance result is claimed for this image.

### Full Home title correction — 2026-09-22

The title/layout correction was flashed to the Hosyond application partition at `0x10000` and the written-data hash verified: 1,209,456 bytes, SHA-256 `b214a45abc230cf99192ba2980a1b25d25cf06d4343f519b295ffc069d8200d8`. A fresh 24 KB NVS backup was retained privately outside the repository; calibration and saved settings were preserved. The update rebooted the board, so Travel Watch must be manually armed again. No serial monitor was opened afterward and no SD operation was performed. Display confirmation and physical Watch results remain pending.

### Tag Watch field confirmation — 2026-09-22

After the title/layout update, the user reported that the possible-following alert had triggered, then confirmed that the background flashed red while the dog and controls remained visible. This is user-observed alert/display confirmation on the Hosyond running the last installed application (`b214a45abc230cf99192ba2980a1b25d25cf06d4343f519b295ffc069d8200d8`). No firmware change, reboot or serial connection was made for this confirmation.

The triggering tag category, exact elapsed time, progress counters and motion conditions were not supplied. The cause of the earlier delayed/missing warning is still unconfirmed. Controlled moving/stationary trials, address rotation, measured timing, touch actions and long-run acceptance remain open; SD testing stays on hold.

### First microSD mount — 2026-09-22

The user resumed SD testing, inserted a 32 GB microSD card and reported **SD OK** on the Hosyond. This confirms a mounted card according to the running firmware. Card model and filesystem were not independently inspected. Log-write counters, exported-file integrity, safe eject/reinsert, heap with active logging, full-card behavior and interruption recovery remain pending. No firmware update, format operation or serial connection was performed for this step.

### SD-enabled low-heap investigation — 2026-09-22

With the 32 GB card mounted, the user reported the low-heap banner, 75,384 B free heap and a 69,332 B largest block. The minimum-heap and SD-write counters were not reported. Most reported free space was contiguous, but these readings do not establish a long-term trend. The warning is triggered below 80,000 B.

A prepared memory update keeps the 128-entry detection cache while removing unused evidence slots and occupancy padding (3,072 B saved in the ESP32 ELF), reduces the SDK controller connection reservation from three to one, and changes the log stdio buffer from 2,048 to 512 bytes. Reception buffers, confidence rules, log records, NVS formats and flush/fsync cadence are unchanged. Post-update SD logging/export and heap measurements remain pending; preparing this update did not connect to or reboot the running board.

The user confirmed SD eject before the memory update. The 1,209,456-byte release application (`b87717a1d08963c1d5c8cc5f5a2bd3594813f4b7c971f3e5d14dc102418e3843`) was then written at `0x10000` and esptool verified its written-data hash. A fresh private 24 KB NVS backup was retained outside the repository. NVS, calibration and saved settings were preserved; no card format or erase was performed. The board rebooted into the new image, with no serial monitor opened afterward. Card remount, SD writes and current/minimum/largest heap still require post-update confirmation. Travel Watch must be armed again after the reboot.

After instructions to power off, reinsert the card and boot, the user reported **82,256 B current free heap**, 6,872 B above the earlier 75,384 B reading and above the unchanged 80,000 B warning threshold. This is one user-reported snapshot; uptime, post-update SD status, minimum heap, largest block and SD-write counts were not supplied. Active logging/export and sustained minimum-heap acceptance remain open. No serial connection or firmware change was made to collect this reading.

The user then reported SD WRITES around 28–30, followed by 32. This confirms user-observed progress in the cumulative successful-record-write counter after the update. The counter increments after a successful buffered write; this observation alone does not verify flushed file contents or export integrity. Export, safe eject/reinsert and minimum-heap checks remain pending. No serial connection or board reset was performed.

### SD export file-lock correction — 2026-09-22

After SD WRITES advanced to 32 on the memory-update image, the user tapped Export History and reported **SD ERROR** in the top bar. Inspection found that export kept the current session log open for writing while attempting to open a separate reader on every log, including that active file. The pinned FatFs build has `CONFIG_FATFS_FS_LOCK=5`; its `chk_share` rejects the second open with `FR_LOCKED`. This code path explains the reported failure, but no board-side error code was captured and no serial connection was opened.

The correction flushes and closes the active log, exports the stored history on the same storage worker, then reopens the existing log at its end without creating or truncating it. Queued observations resume afterward. Export files are flushed and synced, and a failed attempt removes its completion manifest. Existing logs and NVS formats are unchanged. The release application is 1,209,824 bytes, SHA-256 `8ea3646d669ba9d69b1e19a9a42e3e7710c24498fd297aa421f9942902389627`.

The user confirmed tapping Eject SD and waiting, with no confirmation message. The existing latched SD ERROR label takes precedence over SD OFF even after an eject request; no board-side eject-completion telemetry was captured. The corrected 1,209,824-byte application (`8ea3646d669ba9d69b1e19a9a42e3e7710c24498fd297aa421f9942902389627`) was then written at `0x10000`, and esptool verified the written-data hash. A fresh private 24 KB NVS backup was retained outside the repository as `hardware-backups/hosyond-nvs-before-sd-export-20260922-134531.bin`. NVS, calibration and saved settings were preserved; no SD format or erase was performed. The board was reset to start the update, and no serial monitor was opened afterward. Physical card remount, export completion, resumed logging, minimum heap, storage stack and exported-file validation remain pending. Travel Watch must be manually armed again after reboot.

### Export completion and save-queue feedback — 2026-09-22

On the installed export-lock fix (`8ea3646d669ba9d69b1e19a9a42e3e7710c24498fd297aa421f9942902389627`), the user reported **EXPORTS: 1**, but no acknowledgement on tapping Export History. This confirms completion according to the firmware counter; the card's files have not yet been independently read or validated. The user then reported SAVE QUEUE FULL and confirmed SD WRITES kept increasing over about 30 seconds. This shows continued storage-worker progress, but no NVS completion/error telemetry was captured.

Inspection found the old queue warning was never cleared after a later successful enqueue, and an unsuccessful save still advanced the one-minute autosave timer. The prepared correction retries the latest state after one second of queue backpressure, keeping the existing two immutable snapshot slots and queue size. A separate SAVE WAITING / RETRYING status clears on queue acceptance without overwriting critical notices. Export now shows pending, complete-with-row-count or failure directly in its Settings button and blocks repeated taps while pending.

The updated release application is 1,210,560 bytes, SHA-256 `e7f3cc98251272f33a53e53cf3aaf8d57bd06b3b97937cdbe6c08a5644baa0af`. It is built and tested but not yet installed. Software eject is needed again before the update because the card is actively logging after the previous reboot. Physical feedback/retry, export-file validation, heap and long-run acceptance remain pending.

### Hound-care gameplay update prepared — 2026-09-22

The user requested hunger/mood decay with tears and lethargy when either need is low, followed by happy recovery after eating. The prepared update uses five-minute powered-on decay steps (-3 fullness/-2 mood), a low threshold of 25, and meal recovery above that threshold. The six original dog sprites now have teary hungry frames and the meal finale has bounded vertical hops. Reduced motion stays still and Stop retains the sleeping pose. The saved Pet layout remains 104 bytes in both the installed and new ESP32 ELFs; only static methods/constants were added to it. Demo needs are separate RAM values.

This update also includes the previously prepared export acknowledgement and deferred save retry. The board still runs the installed export-lock fix (`8ea3646d669ba9d69b1e19a9a42e3e7710c24498fd297aa421f9942902389627`). No serial connection or reset was made while preparing gameplay. A fresh software eject is required before installing the combined update; physical behavior, heap and saved-state persistence remain pending.


### Combined care, export/save feedback and safe-eject installation — 2026-09-22

The user requested the pending update, completion of SD handling and saved-state verification. They tapped software eject and reported SD OFF. A fresh private NVS backup, `hardware-backups/hosyond-nvs-before-sd-finish-20260922-150210.bin`, contains generation 417 with all five envelope CRCs valid: SCOUT/beagle, Trail Bandana, 474 XP, 93 meals, fullness/mood 100/100 and six ignored identities. No identity values or private keys were published.

Application 1,212,448 bytes, SHA-256 `9c925710682ac7f0a587acd31edb5044c9c46463ea63c7b001bb6b10fc071ea5`, was written at 0x10000 and esptool verified the written hash. This installs care gameplay, export acknowledgement, deferred save retry, checked safe-eject feedback and visible NVS save/error counters. Saved NVS/calibration, bootloader and partition table were preserved. The board was rebooted; no serial monitor was opened afterward. Physical export/eject/remount, card-file validation and post-reboot saved-state confirmation are in progress. Overnight soak and controlled tag tests remain planned, not completed.


Before card removal testing, an additional guard was prepared to prevent a queued Export from querying free space after eject. This guard is built/tested but not installed; the user has been asked to report export completion and SAFE TO REMOVE, leaving USB/card connected for the final application update and a post-reboot NVS comparison. Reader availability has also been requested. No physical export-file inspection, second NVS comparison or reinsertion result is claimed yet.


The user reported **COMPLETE: 581 records** after exporting on the installed combined update (`9c925710...`). This confirms the new on-screen completion feedback and the firmware-reported row count. The saved files and checksum manifest have not yet been independently read. SAFE TO REMOVE confirmation and reader availability are still pending; no reset or serial connection was made for this observation.


### Final eject guard installed and NVS readback — 2026-09-22

The user confirmed **SAFE TO REMOVE** after the 581-record export and said no card reader is available. Independent exported-file/checksum verification is deferred until a reader is available; on-screen completion is not substituted for that check.

A fresh private 24 KB NVS backup (`hardware-backups/hosyond-nvs-before-eject-guard-20260922-150705.bin`) was taken after safe eject. Compared with the pre-update backup, generation advanced 417 → 423 and boot count 17 → 18. Settings/calibration, appearance, the ignored-identity array, equipped/unlocked outfit bytes and meal key match exactly. The saved dog remains SCOUT/beagle with Trail Bandana and six ignores. XP advanced 474 → 479 and meals 93 → 94; fullness/mood are still 100/100. All five required envelope CRCs and the parsed NVS blob-chunk CRCs passed. This confirms retained state and a later saved meal after the previous reboot; it is not physical power-cut fault injection.

The guarded release (1,212,480 bytes, SHA-256 `8c7f238516184b4421d3009979428dbf9690443eed306e95a6638e66e12fa6a8`) was then written to the application partition at 0x10000 and esptool verified the written hash. NVS, calibration, bootloader and partition table were retained; no card format/erase was performed. A hard reset started the update; no serial monitor was opened afterward. Local preview was refreshed. The user was asked to perform a final safe-eject/power-off/card-reinsert/reboot check and confirm SD OK plus the saved outfit. Overnight soak and controlled outdoor tag tests remain planned for later.


On the final guarded firmware, the user performed safe eject, disconnected USB, removed/reinserted the same card, and reconnected USB. They confirmed **SD OK** and SCOUT’s Trail Bandana returned. This passes the observed same-card remount and saved-outfit power-cycle check. Post-remount log-write progress and NVS counters are being checked separately; card-file integrity remains deferred without a reader.


After the final power-cycle/reinsertion check, the user confirmed SD WRITES increased over about 30 seconds and Diagnostics showed **NVS SAVES: 1, NVS ERRORS: 0**. This confirms resumed logging and one completed post-reboot save according to firmware diagnostics. The update, visible export/eject feedback, same-card remount and saved-state checks are complete within the available hardware setup. Independent SD-file/checksum inspection remains deferred for lack of a reader; physical fault injection and the overnight soak are not claimed complete.

### Wi-Fi-window heap threshold crossing — 2026-09-22 local

On Samsung identity firmware `e6b17931d66f5a689e0ff8c6bddeb0f58303030d8824d43f1fb10e57a0de803e`, the user reported LOW HEAP coinciding with Wi-Fi windows and clearing in BLE, with free heap around 79,000 to just over 80,000 bytes. Largest-block and minimum values were not supplied. This is consistent with temporary radio allocations crossing the existing threshold; it is not by itself a leak diagnosis or a completed memory soak. No serial connection or reset was used to obtain these readings.

Inspection found alignment padding in the volatile detection record. Reordering members reduces ESP32 `Detection` from 96 to 80 bytes, engine allocation from 28,048 to 26,000 bytes and UI view from 7,968 to 7,408 bytes: 2,608 static DRAM bytes recovered. The compiler's natural alignment remains in use; no packed/unaligned access is introduced. All fields, cache/history capacities, saved-state layouts, capture buffers and the warning threshold remain unchanged. All 12 sanitizer tests and four firmware profiles pass. The candidate was subsequently installed as recorded below; live Wi-Fi/BLE heap and SD-progress checks remain pending.

The user confirmed SAFE TO REMOVE for this memory update. A fresh private 24 KiB NVS backup passed all five record CRCs and retained 15 ignored identities. Installed application `ba344a3ef1de8bddc9ef7702ad6670aa2cb45bd3b9c7fb9193c151b4d651e375` (1,214,608 bytes) at `0x10000`; esptool verified its data hash and hard-reset the board. NVS, bootloader and partition table were not written. No serial monitor was opened after the flash. Post-boot SD OK, lowest observed Wi-Fi-window heap and whether LOW HEAP recurs have been requested; no live headroom improvement is claimed yet.

### Overnight run reported — 2026-09-23 local

The user reports that the board has been running all night following installation of the memory-layout correction (`ba344a3ef1de8bddc9ef7702ad6670aa2cb45bd3b9c7fb9193c151b4d651e375`). Exact elapsed uptime and reset history have not been supplied. Requested current/minimum/largest heap, current LOW HEAP status, SD OK and SD-write progress, plus any observed restart/freeze/error. Record this as an overnight run reported by the user, with results pending, rather than a completed eight-hour acceptance pass. No serial connection, reboot or firmware change was made for this check.

The user then reported approximately **82 KB HEAP**, **72 KB MIN HEAP**, and **SD WRITES: 3,403**. Current free space is above the unchanged 80,000-byte warning threshold, while the SDK-reported minimum remains below the acceptance target. The minimum includes the period since boot; the reading does not identify when the low point occurred or establish a leak. The SD counter confirms reported writes have occurred, but a single value does not establish current progress or independently verify durable file contents. Requested a second counter reading after about 30 seconds and the largest free block. Exact uptime, reset/error history and warning recurrence remain unreported, so the formal soak is still open.

The user subsequently confirmed SD WRITES continues increasing, establishing ongoing firmware-reported logging at the morning check. They could not find the largest-block reading. In the installed UI, the SD WRITES screen is Diagnostics page 2; NEXT PAGE advances to page 3, where LARGEST HEAP BLOCK appears above NVS SAVES. The largest-block value, warning recurrence and reset/error history are still pending.


### Idle display update prepared — 2026-09-23

Prepared optional display idling: saved dim/saver/off timeouts default to 2/5/15 minutes; the selected hound and outfit bounce across a black screen. Travel Watch blocks display-off while armed. Display sleep leaves touch polling, sniffing, logging and watch timers running. All 13 sanitizer test suites and four firmware profiles pass; live backlight-off/wake, SD progress and retained state remain to be checked on target. The release candidate is 1,217,104 bytes, SHA-256 `7bb86dfccc28a22480a5126128dd451ef6248e99529f4b29dab0d98891486c0a`.

The existing overnight firmware has not been reset or accessed through serial during preparation. Fresh SAFE TO REMOVE confirmation has been requested before installation. Local simulator at port 8765 was refreshed; its synthetic clock controls can exercise the timeouts without a board restart.


### Idle display installed — 2026-09-23

After the user confirmed safe SD eject, the first backup attempt connected but failed after switching to 460800 baud, before any firmware write. A retry at 115200 baud completed a CRC-verified NVS backup, but its application write was interrupted partway through. The user requested a recheck/reload. The expected USB port was present and no flasher remained active; a fresh 24 KiB NVS backup again validated all five records and 15 saved ignores.

The complete screen-saver application was reloaded at 115200 baud: 1,217,104 bytes at `0x10000`, SHA-256 `7bb86dfccc28a22480a5126128dd451ef6248e99529f4b29dab0d98891486c0a`. Esptool reported **Hash of data verified** and completed the RTS hard reset. The successful backup is `hosyond-nvs-before-idle-display-id-20260923-171914.bin` (private, outside the repository). NVS, bootloader and partition table were not flashed. No serial monitor was opened after reboot. The new firmware migrates appearance preferences on startup; physical SD OK, saved dog/outfit, preview/wake and timeout behavior still need user confirmation.


### Frequent ordinary alerts prevented display idling — 2026-09-23

The user reported the screen never dimmed or entered the saver during testing with frequent AirTag/iBeacon alerts. Code inspection confirmed each ordinary alert and every frame of its visible card renewed the idle timestamp. Prepared a separate bounded five-second wake, no more often than every thirty seconds, without renewing the interaction timer. Following warnings still hold the display awake; armed Watch continues to block display-off. No board reset or serial access was used during preparation. The adjusted firmware still requires a fresh safe-eject confirmation and installation.


### Ordinary-alert idle correction installed — 2026-09-23

The user confirmed a fresh safe SD eject. A private 24 KiB NVS backup (`hosyond-nvs-before-alert-idle-id-20260923-185235.bin`) validated all five saved record CRCs and 15 ignored identities. The application was flashed at 115200 baud to `0x10000`: 1,217,024 bytes, SHA-256 `283f0f09d23c9968f7518b2a49f4b55bb140787027c7629a843d13c1ebd2adc8`. Esptool verified the written data hash and completed the RTS hard reset. NVS, bootloader and partition table were not flashed; no serial monitor was opened afterward.

Installed behavior: ordinary enabled alerts provide a five-second display wake at most once every thirty seconds, without renewing idle deadlines. Following warnings keep the display awake, and armed Travel Watch still prevents display-off. All 13 sanitizer suites and four build profiles passed before installation. SD OK, saved dog/outfit and actual dim/saver behavior amid frequent nearby tags remain for user confirmation; software verification is not substituted for this physical check.

## Ignore expansion and SD recovery copy

The 2026-09-23 update expands Ignore to 64 entries with explicit migration from the old 16-entry Companion record. It writes authenticated alternating SD backups after a successful internal save; the live list stays in NVS. The original NVS key remains required for recovery, and the card is never auto-imported at boot. Measure HEAP/LARGEST and storage stack during backup because the larger table adds approximately 1.6 KB across existing static copies. Device category counts and rotating-identity limitations are unchanged.

Field checks: confirm the pre-update ignore count/outfit, then Settings → Ignored scents → SD backup should report SD BACKUP SAVED with that count. Check Beagle ears in Midnight and on the bouncing saver. Without an external SD reader, independent physical backup-file validation remains deferred. See [Ignore storage](IGNORE_STORAGE.md).

Installation completed at `20260923-192007` UTC after fresh SAFE TO REMOVE confirmation. A private 24 KiB NVS readback validated all five record CRCs, Companion V1 with 16 / 16 used Ignore entries, SCOUT and the Brass Collar. Only the 1,283,744-byte application at `0x10000` was flashed at 115200 baud; esptool verified the hash and reset via RTS. Installed SHA-256: `bcb4c29fd9e06ad472ec3f72360b5bc41d3f47af99cb767abb9eb66b127b7330`. NVS/partition table/bootloader were not flashed. No serial monitor was opened after reboot; on-device 16 / 64 count, SD backup completion and dark-ear visibility await user confirmation.

## Startup queue/backup correction

After the 64-slot update, the user saw SD OK but zero SD WRITES, NVS SAVES and NVS ERRORS, with backup waiting and queue busy. The initial worker ran recovery before consuming its queue, so mounted did not mean ready. The correction adds incremental history validation, responsive NVS/eject processing, startup progress, deferred backup without extra state copies, and automatic manual-request retry. The user confirmed SAFE TO REMOVE for the update; live progress and backup success are the next acceptance checks. No serial monitoring or reset was performed before that confirmation.

Installation receipt: after the user's fresh SAFE TO REMOVE confirmation, a private 24 KiB NVS backup (`hosyond-nvs-before-storage-queue-id-20260923-193714.bin`, UTC receipt stamp) validated all five record CRCs. Companion V2 has capacity 64 with 16 used entries. Canonical ignore identities and the identity key match the pre-expansion backup; settings, appearance, SCOUT and the Brass Collar are retained. The committed generation advanced 1881 → 1891, confirming the previous worker eventually completed saves after the reported wait.

The 1,285,248-byte application at `0x10000` was installed at 115200 baud with SHA-256 `d558fd91123c228675acb722451fea36889dbe11c4020c2f87130bbb2dbe8251`. Esptool verified the written data hash and completed the RTS hard reset. NVS, bootloader and partition table were not flashed; no serial monitor was opened after reboot. The user subsequently confirms SD CHECK is shown in the top bar and the checked-record count increases. This verifies visible startup progress on the board; completed backup and resumed SD writes remain pending. Independent SD-file verification remains deferred without a card reader.


## Immediate AirTag re-alert investigation — 2026-09-23

After confirming increasing SD CHECK progress, the user reported that storage now works. They report a steady four Likely signals and an AirTag card returning within seconds/on the next scan after Ignore. No fresh radio capture or serial session was taken. Four is a recent-signal total including ignored identities, not an exact physical-tag count. Whether repeated Ignore adds entries has been requested.

A host regression reproduces replacement of the displayed alert's target by a radio event processed before the next touch. Prepared firmware retains the last displayed target in the existing action snapshot and shows a short keyed ID on the card/menu, with saved Ignore status and an Apple/Find My detail page. This is a confirmed software defect but not yet the confirmed explanation for the user's recurring signal. Hardware installation requires fresh safe eject; compare the displayed ID before Ignore and on any subsequent alert. Different IDs remain eligible; changed identities are not merged by timing/RSSI.


Installed after fresh safe-eject confirmation (UTC receipt `20260923-201443`): private 24 KiB NVS backup `hosyond-nvs-before-airtag-selection-id-20260923-201443.bin` validated all five record CRCs, Companion V2 with 17 / 64 used ignores, SCOUT and the Brass Collar. The identity key matches the previous backup. Compared with the pre-storage-correction backup, used entries increased from 16 to 17; this confirms an additional saved ignore, not which radio signal caused the user's recurring card.

Only the 1,286,384-byte application at `0x10000` was flashed at 115200 baud, SHA-256 `27857be6757d59f174dc91fa78f2ccba8fd88f601ca29ecd9e0a308c11a3f421`. Esptool verified the written hash and completed its RTS hard reset. NVS, bootloader and partition table were not flashed; no post-reboot serial monitor was opened. On-board acceptance remains: matching local ID on the card/action menu, quiet subsequent scans for that ignored identity, and comparison of any returning alert's ID/status. SD remount and backup completion after this reboot remain to be observed.


## Coverage help and inference guards installed — 2026-09-24

After the user confirmed SD was ejected, the Hosyond was identified as ESP32-D0WD-V3 revision 3.1 on the expected CH340 USB port. The tested release application (1,288,432 bytes; SHA-256 `a24290d8ecd957a7b4bea8dd50d145055544001ada1d819d3589db48403d8545`) was written at `0x10000` at 115200 baud. Esptool verified the written-data hash. This installs roadmap items 1–2: Scent Book → Coverage and transmitter-OUI inference guards, plus beacon/probe-response-only SSID matching.

A private pre-update backup of `0x8000–0xffff` includes the partition table, NVS and PHY initialization region; a separate 24 KiB NVS copy was retained outside the repository. The installed partition table matched the build before writing. A readback of the same 32 KiB after the application write was byte-for-byte identical, preserving saved settings, ignores, calibration and identity data before the new application started. The bootloader was not written. Private receipt stem: `hosyond-coverage-20260924-164702` (UTC).

The board hard-reset into the new image. A 45-second serial capture, opened without asserting another reset, received health reports at uptime 5 and 35 seconds. Current heap was 81,328 then 80,820 bytes; minimum 74,068; largest free block 73,728. Wi-Fi callbacks increased 167 → 1,162 and BLE callbacks 133 → 1,419, with zero reported radio/save errors and no observed panic, watchdog or brownout. Aggregate drops rose 116 → 1,083 in uncontrolled ambient traffic; this is not a calibrated loss measurement or long-term acceptance test. Travel Watch was off after reboot and must be manually armed again.

Physical Coverage navigation, restored appearance, SD startup completion and resumed writes still require user observation. No card format, manual SD-file changes or controlled RF validation was performed during installation.
