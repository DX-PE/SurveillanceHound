<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Getting started

Surveillance Hound can run as a browser emulator before you have a board. For radio reception, use the supported ESP32 display hardware and build the firmware.

## Try it without hardware

Open [Try the Hound](lab.md) for the browser lab, with synthetic sightings and temporary progress. No installation is needed to use a hosted lab. The [emulator guide](simulator.md) also covers the native development server at `http://127.0.0.1:8765/`.

## Display board

[Buy the tested Hosyond 4-inch ESP32 board](https://amzn.to/4d4VI5R) (affiliate link).

Use the pinned SDK and a repository path without spaces. After activating ESP-IDF:

```sh
idf.py build
./tools/flash.sh /dev/ttyUSB0
```

Replace the serial port with the display board's port. See [BUILDING](BUILDING.md) for SDK installation, the four profiles, Docker, and development packaging. Start bring-up on USB power. The tested Hosyond uses **Panel inversion: Off**. Initial display, card export/eject and saved-state checks are recorded in [hardware measurements](HARDWARE.md); full touch, SD/NVS durability, audio and battery validation remain open.

### First use

1. Power the board over USB and complete the on-screen region, privacy and touch setup.
2. Choose a dog and name. Landscape and rotation lock are the defaults.
3. Start sniffing. Open **LOG** for evidence and **SCAN** for detector switches.
4. Use **SET → Alert types** to quiet categories while keeping their logs, or **Ignore** to quiet one supported identity.
5. Start **Travel Watch** manually when wanted; it starts off after each reboot.

A quiet scan does not establish that an area is free of cameras or trackers. Read [coverage and limits](DETECTIONS.md) before interpreting detections.

### microSD

A card is optional; settings, progress and ignores use internal NVS. The tested card is a 32 GB FAT32 microSD. Hound does not format cards. Insert with the board powered off, then wait for **SD CHECK** to finish and **SD OK** to appear. Before removal, use **Settings → Data → Eject SD** and wait for **SAFE TO REMOVE**. [Storage](storage.md) covers export and error feedback.

## ESP32-C3 Super Mini

The C3 has a separate **radio-only test harness** using the shared scheduler, parsers and rules. It is not the display firmware, and it is not connected to the browser preview.

With the pinned SDK and its `esp32c3` toolchain installed, run from the repository root:

```sh
idf.py -C tests/c3_radio -B "$PWD/build-c3" build
idf.py -C tests/c3_radio -B "$PWD/build-c3" -p /dev/ttyACM0 flash monitor
```

Use the C3's actual serial port and preserve its existing firmware before flashing if needed. The harness automatically pauses at 45 seconds, resumes at 65 seconds, then continues passive scanning. It reports aggregate counts, rule matches and RSSI without printing observed addresses, names or raw payloads.

The recorded 100-second room test received **33 Samsung FD5A advertisements**, with zero queue drops or radio errors. Reception stopped and resumed correctly after a watchdog timing fix. A Samsung tag was reported in the room; the transmitter was not isolated as that specific tag. See the [C3 test report and original-firmware restore instructions](C3_RADIO_TEST.md).

## Development status

Follow [TEST_PLAN](TEST_PLAN.md), record measurements in [HARDWARE](HARDWARE.md), and track readiness in [STATUS](STATUS.md). Formal display/touch checks, SD/NVS power interruptions, 50 cold boots, the eight-hour soak, the 80 KB runtime heap target, measured capture loss, battery calibration/current draw and controlled product-signature captures remain outstanding.

This hardware observes 2.4 GHz Wi-Fi and legacy BLE only. It does not cover 5/6 GHz Wi-Fi, BLE extended advertising or coded PHY. Phone GPS/companion work is deferred. No cloud service, active probing, sensitive-export path or on-device rule editor is included.
