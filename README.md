<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Surveillance Hound

An offline ESP32 radio-observation pet for the **LCDWiki E32R40T**: six dogs, passive Wi-Fi/Bluetooth clues, and local logs. This is a **0.1.0 development build**. The ESP32-C3 radio test has passed; the display board's physical acceptance tests remain pending.

![Firmware UI rendered on the host](docs/ui-preview.png)

[Sleeping hound and settings preview](docs/ui-features.png)

The application uses passive Wi-Fi management reception and passive legacy BLE scanning. It does not associate, probe, advertise, inject frames, or upload observations. A match describes radio evidence, not intent or proof of surveillance.

## Current features

- **Six original dogs:** Corgi, Beagle, Shiba, Husky, Labrador and Dalmatian, with 33 animation frames each, naming, pet progress, touch reactions and level-up celebrations.
- **Landscape by default:** 480 × 320, with a 320 × 480 portrait layout and persistent rotation lock. Rotation is manual; the board has no orientation sensor.
- **Start/Stop sniffing:** the home-screen button pauses reception and puts the hound to sleep with floating Zs. Start wakes the hound and resumes reception. Both button labels fit within the display.
- **Snacks for all 19 categories:** the hound turns sideways, walks up, takes progressive bites and finishes happy. Reduced-animation mode is available.
- **67 enabled rules:** Flock, Axon, glasses, skimmer modules, Raven, Apple Find My/AirTag, Tile, Samsung Tag, Google Tag, Drone, ALPR, Camera, Ring, iBeacon, Flipper, Pwnagotchi, Pineapple, deauth bursts and possible evil twins. Vendor, name and generic-service clues retain conservative confidence limits; see [detection coverage and limitations](docs/DETECTIONS.md).
- **Bounded detection:** 128-entry observation and meal caches, confidence fusion, cooldowns, probe-request parsing, 128-bit BLE UUIDs, structured uncompressed Pwnagotchi advertisements, legacy Remote ID assembly and corroborated Wi-Fi security differences.
- **Evidence and controls:** 32 recent events paged four at a time, frozen evidence details, separate detector and alert switches, and volatile Remote ID details when received.
- **Follow Scent:** select a logged sighting for fresh RSSI, a 30-second chart, reception trend and waiting/lost/paused states. Raw address and samples remain in RAM; this does not estimate direction or distance.
- **Atmosphere:** Midnight, Sunset and Daylight themes; Signal Grid, Dog Park and Rooftops scenes; optional speech bubbles, varied petting reactions and a compact patrol panel. Reduced animation stays available.
- **Scent Book:** illustrated field-guide cards for all 19 categories, persistent observation totals, confidence and manual first/last times, plus recent matching evidence.
- **Dog wardrobe:** five original accessories alongside the classic look, with previews, XP/discovery unlocks and saved equipment for every dog.
- **Snooze/Ignore:** 5/15/60-minute global alert snooze, early resume, and 16 saved device/category ignores with individual removal. Pop-ups and sounds are quieted; logs, meals and discoveries continue.
- **Local storage and privacy:** private HMAC identifiers by default, opt-in raw local MAC logging, rotated JSONL, complete-row validation, incomplete-tail recovery, corrupt-log write lockout, full-history export and confirmed log deletion.
- **Device settings:** manual UTC, three-point battery calibration, optional voltage/estimated percentage, critical save/flush/sleep, brightness, saved panel inversion and muted-by-default audio effects.
- **Diagnostics:** per-radio accepted/malformed counts, queue drops, task watermarks, heap, SD space, raw ADC and display/touch self-test.

Firmware uses native **ESP-IDF 6.0.2**, pinned to `7101770dc6db2667b3c477cc31365dd1acd6db4e`, a C++20 portable core, and project-owned ST7796 display/XPT2046 touch drivers. Release, debug, test and demo build profiles are included.

## Run the local simulator

Requirements: Python 3.11+, CMake 3.22+, Ninja, a C++20 compiler and OpenSSL development headers. Debian/Ubuntu packages: `build-essential cmake ninja-build libssl-dev python3-venv`.

From the repository root:

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements-dev.txt
python3 tools/run_local.py
```

Open [http://127.0.0.1:8765/](http://127.0.0.1:8765/). The script builds the native firmware UI and serves it on loopback. Click the device screen, choose a hound, try onboarding, or use the sample selector to preview any category's snack. Start sniffing before sending samples if the hound is asleep.

The browser preview uses **synthetic events and RAM-only state**, even when a C3 is connected over USB. It does not display live USB radio data, write SD logs or award saved XP. Demo samples fill a separate Scent Book and award session-only wardrobe XP. Repeating a sample category uses the same synthetic device, so Ignore can be tested. Restarting the simulator resets the session. Press Ctrl+C to stop, or use `--port 8766` for another port.

| Control | Behavior |
|---|---|
| Home → Start/Stop sniffing | Pause/resume scanning; show the sleeping hound while stopped |
| Settings → More: Display → Rotation lock / View | Unlock, choose landscape or portrait, then lock the chosen orientation |
| Settings → More: Display → Alert types | Toggle pop-ups and sounds per category while retaining sightings; the confidence threshold still applies |
| Detectors | Enable or disable observation processing for each category |
| Home → BOOK | Browse all 19 field-guide cards, statistics and recent evidence |
| Home → hound name / Settings → Dog wardrobe | Preview accessories, see requirements and equip unlocked gear |
| Alert or Log → details → Snooze / Ignore | Quiet all alerts for 5/15/60 minutes or ignore the selected device/category |
| Settings → first page | Manage ignored scents, resume alerts, open Scent Book, wardrobe or Atmosphere |
| Settings → More: Display → Display options | Brightness, immediate saved Panel inversion toggle, animation and color/touch test |
| Log → sighting → Follow Scent | Follow reception from that radio identity; Home → Follow returns to it |
| Settings → Atmosphere | Cycle theme, background, speech frequency and compact panel |
| Settings → Tools (page 6) | Set UTC, calibrate battery, clear SD logs, run self-test or export history |

Manual UTC is marked as manual in records and resets after power loss. Battery mode defaults off; voltage and estimated percentage require a valid, explicitly enabled calibration against a meter. Power thresholds still require validation with the actual board and battery.

## Build and flash the display board

Use the pinned SDK and a repository path without spaces. After activating ESP-IDF:

```sh
idf.py build
./tools/flash.sh /dev/ttyUSB0
```

Replace the serial port with the display board's port. See [BUILDING](docs/BUILDING.md) for SDK installation, the four profiles, Docker, and development packaging. Start bring-up on USB power. The connected Hosyond displays Hound with correct colors after disabling inversion. Full touch, SD/NVS durability, audio and battery validation remain open; SD testing is on hold.

## Test an ESP32-C3 Super Mini

The C3 has a separate **radio-only test harness** using the shared scheduler, parsers and rules. It is not the display firmware, and it is not connected to the browser preview.

With the pinned SDK and its `esp32c3` toolchain installed, run from the repository root:

```sh
idf.py -C tests/c3_radio -B "$PWD/build-c3" build
idf.py -C tests/c3_radio -B "$PWD/build-c3" -p /dev/ttyACM0 flash monitor
```

Use the C3's actual serial port and preserve its existing firmware before flashing if needed. The harness automatically pauses at 45 seconds, resumes at 65 seconds, then continues passive scanning. It reports aggregate counts, rule matches and RSSI without printing observed addresses, names or raw payloads.

The recorded 100-second room test received **33 Samsung FD5A advertisements**, with zero queue drops or radio errors. Reception stopped and resumed correctly after a watchdog timing fix. A Samsung tag was reported in the room; the transmitter was not isolated as that specific tag. See the [C3 test report and original-firmware restore instructions](docs/C3_RADIO_TEST.md).

## Test and verify

After installing the host dependencies above:

```sh
cmake -S . -B build-host -G Ninja -DSNIFFER_HOST=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure
./build-host/fuzz_mutate 60
./build-host/ui_preview preview.ppm
```

The recorded verification includes successful release/debug/test/demo firmware builds, 206 core checks, 643 UI checks, 151 extended feature checks, 12 Python tests, 100,000 deterministic fuzz inputs, and 2,101,847 additional seeded mutation inputs. The final host test suite passed with address, undefined-behavior and leak checks enabled. See [VERIFICATION](docs/VERIFICATION.md) for exact scope and build hashes; the GitHub Actions workflow is provided, but no remote CI result is claimed.

In a ptrace-based sandbox, LeakSanitizer may require `ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-host --output-on-failure`; retain leak detection on an ordinary host. Development packaging includes checksums, a source SPDX inventory, linked-archive evidence and retained dependency notices. Final license compatibility review remains a release gate.

## Export and saved state

**Settings → Export history** streams validated stored events from the SD card, retaining repeated updates, with a 4,096-file safety bound. It rekeys session IDs and observation identifiers and writes a checksum manifest only for a completed export.

For an aggregated historical export, eject the card in Settings and run:

```sh
python3 tools/export_logs.py /media/CARD/SURVSNIFF/LOGS ./community-export
python3 tools/validate_export.py ./community-export
```

Nothing is uploaded. Raw MACs, SSIDs, payloads and Remote ID serials/coordinates are excluded from sanitized exports. The device collects no GPS location. See [privacy](docs/PRIVACY.md) and [export format](docs/COMMUNITY_EXPORT.md).

Version 1, 2, 3 and 4 saves migrate to version 5, preserving pet progress, privacy settings, recent meals and touch calibration. Old cat selection slots become Husky, Labrador and Dalmatian. Settings, pet, system, companion and appearance state use CRC-protected generation slots in separate NVS namespaces. V4 and V5 commits require the companion record; V5 also requires the appearance record. Interrupted or corrupt state never silently discards ignores or preferences. Older saves start with Midnight / Signal Grid / occasional speech / detailed panel. The appearance record upgrades from version 1 to version 2 without changing the V5 generation scheme; existing theme/scene/speech/panel preferences are retained and inversion defaults off. Simulator state is not persisted.

## Scent Book, outfits and quiet alerts

Scent Book counts emitted observations, including repeat updates, rather than unique devices. First/last times are manual UTC when available; unset times remain unknown. Statistics start when this feature is installed or progress is reset. Existing saves retain XP and earn eligible XP outfits, without inventing historical sightings. Evidence pages use the latest 32 RAM log entries; they may be empty after reboot even when totals remain saved.

| Outfit | Unlock |
|---|---|
| Classic | Available immediately |
| Trail bandana | 25 XP |
| Brass collar | 100 XP |
| Raincoat | Discover 5 categories |
| Detective cap | 300 XP |
| Sunglasses | Discover 10 categories |

Unlocks stay earned until **Reset progress**, which also clears the Scent Book and equipped outfit. Reset keeps observation logs and ignored devices. Snooze uses powered-on time, expires automatically, and resets on reboot. Stop/Start does not cancel snooze; Settings offers early resume. Ignore persists a keyed digest for the selected device and category, never a raw address. Rotating/changing addresses can alert again; unrelated devices in that category remain eligible. The 16-entry list never silently evicts an entry when full.

All emulator-inspired ideas, selected work and future acceptance criteria are recorded in the [roadmap](docs/ROADMAP.md).

## Recent signals and quiet alerts

The home **Strong / Likely** totals count distinct radio/address/type identities seen within the last **90 seconds**, using the highest current confidence when several categories match one identity. Repeated updates refresh that identity instead of adding another count. The totals reuse the 128-entry detection cache, refresh every two seconds, and expire while sniffing is stopped. They are recent signals, not a verified inventory of physical tags: address rotation can temporarily add an identity, and Apple Find My clues also occur on compatible products other than AirTags. Disabled detector categories are excluded. Sound mute, snooze and per-device Ignore silence alerts but do not hide observed signals; Scent Book and logs continue counting emitted observations. Demo samples never add to the real totals.

Home alerts always use a small card beside the dog in landscape or below it in portrait, including repeats that earn no meal. The dog animation, **Snooze / Ignore** and **Start / Stop sniffing** remain accessible. Eating starts only when a detection earns a meal; ordinary repeat tags have a 30-minute feeding cooldown, conservatively restored after reboot. Repeat alerts can therefore appear without another eating animation. For an immediate animation check, enable **Settings → Data → Demo mode** with sniffing running; synthetic Flipper/Pineapple meals occur every eight seconds without changing saved progress. Full evidence stays in the Log.

**Diagnostics** reports byte-addressable internal heap, its boot minimum and the largest allocatable block (page 3). The 80,000-byte warning tracks current free space, clears when it recovers, and no longer overwrites other notices. The USB serial console prints aggregate memory, stack, radio/drop, save-error and watch-progress statistics every 30 seconds; it does not print discovered identifiers or payloads. See [hardware measurements](docs/HARDWARE.md) for the short run and remaining acceptance limits.

## Tag Watch: possible-following warning

Tap **WATCH** on Home, or **TAG WATCH** on the first Settings page, then **Start Travel Watch** when heading out. It starts off after reboot. This is a repeated-presence heuristic: without GPS or motion input, Hound cannot confirm that a tag moved with you or identify its owner. Your own tags and stationary nearby tags can qualify. Apple Find My classifications also cover devices other than AirTags.

AirTag/Find My, Samsung, Tile and Google tag classifications qualify only when the same keyed radio/address/type/category identity is seen over **at least ten minutes**, in **at least eight distinct one-minute intervals**, with **no gap over two minutes**. Only BLE sightings scoring at least 50 and passing the current confidence, detector and alert-category settings count. Generic iBeacon and weak Fast Pair clues do not qualify. Old engine timestamps and packet counts cannot accelerate the watch; time starts with the first eligible sighting after arming.

Starting the watch opens **Watch progress**. While armed, the Home **WATCH** button returns to this page; **Controls** opens the watch options. Each row shows its category, a session slot number, classification score, observed time toward **10:00**, occupied minute bins toward **8**, and seconds since its last qualifying sighting. The timer advances on sightings rather than simply counting time since arming. Gap/filter reset counters explain discarded windows; clock skips record rejected future timestamps. New or rotating identities build separate rows. **Settings → Alerts: High** excludes the current Samsung (60) and Apple Find My (75) signatures; use **Med/High** when testing those families. Acknowledged entries show **ACK**.

A qualifying tag shows **Possible following**, its category and observed duration in the compact Home card. The background alternates red/normal once every two seconds across regular screens, keeping the hound and controls visible. Tap **Review tag** or the top **Tag alert / Review** indicator. **Acknowledge** silences that identity while it remains present; **Ignore this tag** uses the existing saved ignore list; **Snooze** suppresses all alerts temporarily. Sound mute does not disable this visual warning. **Red warning: Steady**, or the existing **Animation: Reduced** setting, replaces flashing with steady red. Watch and flash choices are session-only; reduced animation and real-device ignores retain their existing persistence.

Stop/Start sniffing clears the observation window. Ending the watch, rebooting, calibration or switching demo mode also clears it. Warnings clear after two minutes without an eligible sighting; repeated sightings must build a fresh window afterward. Sixteen keyed identities fit in a bounded RAM table, with no new NVS or SD records. A full table keeps existing present identities; it cannot cover every tag in a crowd. Rotating addresses remain separate, which can prevent a warning; Hound does not correlate them. This feature is not a substitute for phone unwanted-tracker alerts.

In the local lab, use **Tag Watch → Preview red warning** and choose a tag. This sends synthetic sightings and advances the simulator clock by ten minutes using the same production rule. It changes no saved progress, creates no meals, and never injects real radio data. Disabled categories, confidence settings and ignored demo identities still apply. On the Hosyond board, the user confirmed that the possible-following alert triggered and the red background flashed with the dog and controls still visible. Controlled RF accuracy, timing and broader hardware acceptance checks remain pending.

## Settings order and panel inversion

Settings always opens on **Hound**, with **Wardrobe, Atmosphere, Scent Book, Ignored scents and Snooze/Resume alerts**. The More button names the next page: **Display → Alerts → Data → Maintenance → Tools**, then back to Hound. Returning from a submenu keeps its parent page; tapping the main SET tab or cycling to Settings with BOOT returns to Hound.

**Settings → More: Display → Display options → Panel inversion** changes the LCD color polarity immediately and saves it on the real device. Keep it **Off** on the tested Hosyond board; enabling it turns the dark background light and changes the dog colors. Its value is restored at boot. Demo inversion is temporary and the real setting is restored when leaving demo. The simulator previews the same color reversal without altering the underlying theme.

## Follow Scent and atmosphere

Open **Log**, select a sighting and tap **Follow Scent**. Selection waits for a new, valid reading from the same address, address type and radio. Live reception is at most five seconds old; after five seconds it shows the last reading and waits for a repeat, and after 15 seconds it shows **Signal lost**. The 30-second graph uses one latest reading per second, leaves gaps between sparse readings, and fades as data ages out. Trend compares recent samples; the dog reacts to the current reception level. Walls, antennas, channel hopping and advertisement intervals affect RSSI, so neither the graph nor the dog gives direction or exact distance.

**Back to Hound** keeps the selected scent; tap **Follow** to reopen it. **Stop following** clears it without stopping scanning. Stop/Start scanning keeps the target but clears readings, waiting for a fresh sample after resuming. Selecting a new scent or changing demo mode resets the follow session; reboot clears it. Address rotation requires a new selection. A disabled detector suspends updates for its selected category. Follow readings bypass alert throttling but do not independently add log entries, discoveries or meals.

In the browser, send a synthetic sighting first, select it through the device Log, and use **Follow a signal** below the device. Change reception, send one reading or enable **Repeat reading every second**. Disable repeat to test waiting/lost states. These readings are synthetic and do not use the USB board.

**Settings → Atmosphere** cycles themes, scenes, speech (**Off / Occasional / Chatty**) and the patrol panel (**Detailed / Compact**). Tap **Preview on Hound** to return home. Petting triggers alternating replies and happy/head-tilt poses; the sleeping dog and important scanning/alert status remain visible with speech off. **Settings → More: Display → Animation: Reduced** freezes scene movement and dog animation. Real-device appearance is saved; demo appearance has its own temporary copy and cannot alter saved preferences. Reset progress keeps appearance choices.

## Remaining hardware work

Follow [TEST_PLAN](docs/TEST_PLAN.md), record measurements in [HARDWARE](docs/HARDWARE.md), and track readiness in [STATUS](docs/STATUS.md). Formal display/touch checks, SD/NVS power interruptions, 50 cold boots, the eight-hour soak, the 80 KB runtime heap target, measured capture loss, battery calibration/current draw and controlled product-signature captures remain outstanding.

This hardware observes 2.4 GHz Wi-Fi and legacy BLE only. It does not cover 5/6 GHz Wi-Fi, BLE extended advertising or coded PHY. Phone GPS/companion work is deferred. No cloud service, active probing, sensitive-export path or on-device rule editor is included.

The supplied specification is retained with external project names generalized. The Hound name, all-dog roster and landscape layout reflect the later project requirements. Internal `sniffer` namespaces, build flags and on-card directory names remain stable for compatibility; display-board binaries are named `surveillance_hound.bin`.

Firmware, tools and tests: Apache-2.0. Original documentation and art: CC-BY-4.0. Independently assembled signature facts: CC0-1.0, with sources and their terms recorded separately. An external detector reference was consulted for visual ideas and, with user authorization, signature facts; its implementation and assets are not incorporated. See [NOTICE](NOTICE), [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES.md) and [rule provenance](data/provenance.yaml).
