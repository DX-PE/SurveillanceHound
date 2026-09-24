<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Emulator and lab guide

[**Open the browser lab**](lab.md) to try Hound immediately. It uses the same C++ simulator and firmware UI as the native lab, compiled to WebAssembly. Every browser tab has an independent, temporary session. Use **Open the full-window lab** for the wider layout and screen recording. A current browser with JavaScript, WebAssembly, Web Workers and Web Crypto is required; use HTTPS or localhost.

## Run the native local lab

Requirements: Python 3.11+, CMake 3.22+, Ninja, a C++20 compiler and OpenSSL development headers. Debian/Ubuntu packages: `build-essential cmake ninja-build libssl-dev python3-venv`.

From the repository root:

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements-dev.txt
python3 tools/run_local.py
```

Open [http://127.0.0.1:8765/](http://127.0.0.1:8765/). The script builds the native firmware UI and serves it on loopback. Click the device screen, choose a hound, try onboarding, or use the sample selector to preview any category's snack. Start sniffing before sending samples if the hound is asleep.

On desktop, the device stays in a fixed center area with hound/care/idle controls on the left, sample/signal/Tag Watch controls on the right, and screen shortcuts underneath. Scroll either side panel to reach its remaining controls without moving the device. Choosing samples, previews or signal readings never scrolls the page automatically. Narrow windows stack the controls below the device.

The browser preview uses **synthetic events and RAM-only state**, even when a C3 is connected over USB. It does not display live USB radio data, write SD logs or award saved XP. Demo samples fill a separate Scent Book and award session-only wardrobe XP. Repeating a sample category uses the same synthetic device, so Ignore can be tested. Restarting the native simulator or reloading the browser lab resets its session. A subtle light rim keeps dark ears and tails visible in Midnight/Sunset and on the black saver, without changing fur colors; Daylight keeps its original outline. The Bouncing hound panel previews the saver and advances the virtual clock to test idle timeouts. Press Ctrl+C to stop, or use `--port 8766` for another port.

## Trigger a possible-following alert

1. Start sniffing if the hound is sleeping.
2. In the controls beside or below the device, find **Tag Watch**.
3. Choose AirTag / Find My, Samsung Tag, Tile or Google Tag.
4. Select **Preview red warning**.

The preview advances the virtual clock through ten minutes of eligible sightings. The compact warning and red background appear without waiting ten real minutes. The selected category must have its detector and alerts enabled and its demo identity must not be ignored. [Travel Watch](tag-watch.md) explains the live rules.

## Screen recording

On wide windows, scroll either control panel independently while keeping the device in the same capture region. Sample, signal and preview buttons do not scroll the page automatically. Smaller windows stack the controls under the display. Use **Reset test session** to clear the emulator's temporary state, or **Try first boot** to rehearse onboarding.

## Other previews

- **Care for your hound:** choose Hungry hound or Low mood, then send a Flipper or Pineapple sample.
- **Bouncing hound:** preview the selected dog and outfit, or advance idle time by 2, 5 or 15 minutes.
- **Follow a signal:** select a logged sample on the device, open Follow Scent and send individual or repeated RSSI readings.
- **Explore:** use the shortcuts below the display for Settings, Wardrobe, Scent Book, Detectors and the other screens.

See the [control reference](controls.md), [hound care](hound-care.md) and [Follow Scent](follow-scent.md).
