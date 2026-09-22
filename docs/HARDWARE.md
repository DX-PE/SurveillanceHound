<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Hardware bring-up

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
| Minimum internal heap >=80 KB | Latest Tag Watch boot smoke: current 85,668–86,248 B; SDK-reported minimum 79,304 B. Minimum target not yet met; see below. |
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
