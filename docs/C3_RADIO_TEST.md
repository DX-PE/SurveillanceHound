<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# USB ESP32-C3 radio test

Date: 2026-09-21 local / 2026-09-22 UTC. Device: ESP32-C3 Super Mini, revision 0.4, 4 MB embedded flash, USB Serial/JTAG at `/dev/ttyACM0`. The user reported a Samsung tracking tag in the room. This was ambient, passive reception; the transmitter was not isolated or positively identified as that individual tag.

The harness in `tests/c3_radio` uses **the same** `components/radio` and `components/sniffer_core` as the display firmware, with the generated production rules. It does not build the display-board drivers. To favor tag reception it uses 100 ms Wi-Fi dwell and six-second BLE windows; display firmware defaults are 200 ms and 1.2 seconds. Logs report categories, rule IDs, RSSI and aggregate counts; they omit observed MACs, advertised names and payloads.

## Result after watchdog correction

| Elapsed | Wi-Fi received | BLE received | Samsung FD5A advertisements | Drops | Radio errors |
|---|---:|---:|---:|---:|---:|
| 10 s | 62 | 729 | 3 | 0 | 0 |
| 40 s | 169 | 3,188 | 16 | 0 | 0 |
| 50 s, stopped | 169 | 3,652 | 19 | 0 | 0 |
| 60 s, stopped | 169 | 3,652 | 19 | 0 | 0 |
| 70 s, resumed | 193 | 4,017 | 21 | 0 | 0 |
| 100 s | 319 | 6,512 | 33 | 0 | 0 |

At 100 seconds: 6,826 parsed and five rejected observations; Samsung service RSSI ranged from -85 to -70 dBm. Generic Samsung company advertisements were counted separately (406) and were **not** treated as tag detections. FD5A service data length was 22 bytes. The engine emitted `SAMSUNG_TAG`, rule `ble.samsung.smartthings`, score 60 (Medium).

The harness stopped at 45 seconds and resumed at 65 seconds. Counts remained unchanged during the settled stopped interval. The first run exposed a scheduler watchdog warning with the C3's five-second watchdog: a six-second BLE window did not reset it. The scheduler now reports progress during bounded sync/channel/BLE waits. The corrected 102-second capture contained no watchdog, panic, assertion or radio-init failures. This is a short functional test, not a soak or independent RF emission audit.

## Reproduce

Activate the project's pinned ESP-IDF 6.0.2 SDK with the `esp32c3` toolchain installed, then:

```sh
idf.py -C tests/c3_radio -B "$PWD/build-c3" build
idf.py -C tests/c3_radio -B "$PWD/build-c3" -p /dev/ttyACM0 flash monitor
```

The test automatically performs one 20-second pause, then continues receive-only scanning. The C3 currently has this test firmware installed. Do not flash the E32R40T image onto it.

The original 4 MB flash was backed up before testing, outside the repository, at:

`/home/jascha/Documents/ChatGPT/Surveillance Sniffer/hardware-backups/esp32-c3-before-hound.bin`

Backup SHA-256: `c0e705515fb42fa83b2d16823bc83633237b9ef7f235263c3cd7f536336bbf39`.

To restore, stop serial monitoring and run the following with the same SDK environment:

```sh
python -m esptool --chip esp32c3 --port /dev/ttyACM0 write-flash 0x0 "/home/jascha/Documents/ChatGPT/Surveillance Sniffer/hardware-backups/esp32-c3-before-hound.bin"
```

The backup may contain the board's previous private settings; it is not a distributable release artifact. E32R40T display/touch, SD/NVS durability, battery/audio, calibrated power and eight-hour soak tests remain pending.

Test application: 853,904 bytes; SHA-256 `56e7abde3bfc364d62a573d910bdfc4321cb7058e7b8c6d1ab4297ac742a1d0a`.
