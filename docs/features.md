<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Features

- **Six original dogs:** Corgi, Beagle, Shiba, Husky, Labrador and Dalmatian, with 33 animation frames each, naming, pet progress, touch reactions and level-up celebrations.
- **Landscape by default:** 480 × 320, with a 320 × 480 portrait layout and persistent rotation lock. Rotation is manual; the board has no orientation sensor.
- **Bouncing hound screen saver:** selected dog and outfit follow a DVD-style diagonal bounce on black. Defaults: dim after 2 idle minutes, saver after 5, display/backlight off after 15. Each timeout supports Never or 1/2/5/10/15/30/60 minutes. Travel Watch always blocks screen-off. Touch/BOOT reset the idle timer; ordinary alerts briefly wake the display for up to 5 seconds, at most once every 30 seconds, without resetting that timer. Active following warnings stay visible. Sniffing/logging continue throughout.
- **Start/Stop sniffing:** the home-screen button pauses reception and puts the hound to sleep with floating Zs. Start wakes the hound and resumes reception. Both button labels fit within the display.
- **Hound care:** fullness and mood decline between meals; hungry or sad dogs slow down and cry, then recover with happy hops after an earned snack.
- **Snacks for all 19 categories:** the hound turns sideways, walks up, takes progressive bites and finishes happy. Reduced-animation mode is available.
- **67 enabled rules:** Flock, Axon, glasses, skimmer modules, Raven, Apple Find My/AirTag, Tile, Samsung Tag, Google Tag, Drone, ALPR, Camera, Ring, iBeacon, Flipper, Pwnagotchi, Pineapple, deauth bursts and possible evil twins. Vendor, name and generic-service clues retain conservative confidence limits; see [detection coverage and limitations](DETECTIONS.md).
- **Bounded detection:** 128-entry observation and meal caches, confidence fusion, cooldowns, probe-request parsing, 128-bit BLE UUIDs, structured uncompressed Pwnagotchi advertisements, legacy Remote ID assembly and corroborated Wi-Fi security differences.
- **Evidence and controls:** 32 recent events paged four at a time, frozen evidence details, separate detector and alert switches, and volatile Remote ID details when received.
- **Follow Scent:** select a logged sighting for fresh RSSI, a 30-second chart, reception trend and waiting/lost/paused states. Raw address and samples remain in RAM; this does not estimate direction or distance.
- **Atmosphere:** Midnight, Sunset and Daylight themes; Signal Grid, Dog Park and Rooftops scenes; optional speech bubbles, varied petting reactions and a compact patrol panel. Reduced animation stays available.
- **Scent Book:** illustrated field-guide cards for all 19 categories, persistent observation totals, confidence and manual first/last times, plus recent matching evidence. Coverage help explains what Hound can hear and why a quiet scan is not an all-clear.
- **Dog wardrobe:** five original accessories alongside the classic look, with previews, XP/discovery unlocks and saved equipment for every dog.
- **Snooze/Ignore:** 5/15/60-minute global alert snooze, early resume, and 64 saved device/category ignores with individual removal. Pop-ups and sounds are quieted; logs, meals and discoveries continue.
- **Local storage and privacy:** private HMAC identifiers by default, opt-in raw local MAC logging, rotated JSONL, complete-row validation, incomplete-tail recovery, corrupt-log write lockout, full-history export and confirmed log deletion.
- **Device settings:** manual UTC, three-point battery calibration, optional voltage/estimated percentage, critical save/flush/sleep, brightness, saved panel inversion and muted-by-default audio effects.
- **Diagnostics:** per-radio accepted/malformed counts, queue drops, task watermarks, heap, SD space, raw ADC and display/touch self-test.

Firmware uses native **ESP-IDF 6.0.2**, pinned to `7101770dc6db2667b3c477cc31365dd1acd6db4e`, a C++20 portable core, and project-owned ST7796 display/XPT2046 touch drivers. Release, debug, test and demo build profiles are included.
