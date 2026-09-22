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

## Measurements (not yet performed)

| Measurement | Result |
|---|---|
| 50 USB cold boots | Pending board |
| Display and four-corner touch | Pending board |
| Missing/full/interrupted SD | Pending board |
| Controlled Wi-Fi/BLE capture loss | Pending lab |
| Minimum internal heap >=80 KB | Pending runtime measurement |
| Eight-hour soak | Pending board |
| Boot / screen-only / Wi-Fi / BLE / full-cycle current | Pending meter |
| Audio current and charging thermals | Pending meter |
| Three-point battery ADC calibration | Pending meter |

There is no hardware-backed claim that these tests passed. Record board revision, build hash, SD model, supply, sample sizes, measurements, failures and exact reproduction steps here when testing.

## Available USB C3

The receive-only ESP32-C3 harness was tested with the shared radio scheduler, parsers and rules. See [C3_RADIO_TEST.md](C3_RADIO_TEST.md). This does not replace any E32R40T measurements above.
