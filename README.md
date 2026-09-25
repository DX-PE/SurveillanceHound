<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Surveillance Hound

An offline ESP32 radio-observation companion: six pixel dogs, passive Wi-Fi/Bluetooth clues and local logs. **[v0.1.0-alpha.1 prerelease](https://github.com/DX-PE/SurveillanceHound/releases/tag/v0.1.0-alpha.1).** A match describes radio evidence, not intent or proof of surveillance.

![Surveillance Hound firmware UI](docs/ui-preview.png)

## Documentation

Read the [documentation](https://surveillancehound.dx.pe/) or [try the Hound in your browser](https://surveillancehound.dx.pe/lab/). The site uses **Zensical**, Hound's Midnight colors and original pixel art.

- [Install from your browser](https://surveillancehound.dx.pe/flash/)
- [Try the Hound](https://surveillancehound.dx.pe/lab/), [getting started](https://surveillancehound.dx.pe/getting-started/) and [emulator guide](https://surveillancehound.dx.pe/simulator/)
- [Controls](https://surveillancehound.dx.pe/controls/), [hound care](https://surveillancehound.dx.pe/hound-care/) and [appearance](https://surveillancehound.dx.pe/appearance/)
- [Alerts and ignores](https://surveillancehound.dx.pe/alerts/), [Travel Watch](https://surveillancehound.dx.pe/tag-watch/) and [Follow Scent](https://surveillancehound.dx.pe/follow-scent/)
- [Detection coverage](https://surveillancehound.dx.pe/DETECTIONS/), [storage](https://surveillancehound.dx.pe/storage/) and [privacy](https://surveillancehound.dx.pe/PRIVACY/)
- [Build firmware](https://surveillancehound.dx.pe/BUILDING/), [run tests](https://surveillancehound.dx.pe/testing/) and [contribute](https://surveillancehound.dx.pe/CONTRIBUTING/)
- [Development status](https://surveillancehound.dx.pe/STATUS/), [roadmap](https://surveillancehound.dx.pe/ROADMAP/) and [release gates](https://surveillancehound.dx.pe/RELEASE/)

Preview the complete documentation and browser lab locally (Docker supplies the pinned WebAssembly compiler):

```sh
python3 -m venv .venv-docs
. .venv-docs/bin/activate
python -m pip install -r requirements-docs.txt
python tools/build_web.py --docker
python tools/build_flasher.py
npm ci --prefix tools/flasher --ignore-scripts
npm run build --prefix tools/flasher
zensical serve
```

Open <http://127.0.0.1:8000/>. See [documentation development](https://surveillancehound.dx.pe/documentation/) for the strict build and GitHub workflow.

## Try the emulator

Requires Python 3.11+, CMake 3.22+, Ninja, a C++20 compiler and OpenSSL development headers.

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements-dev.txt
python3 tools/run_local.py
```

Open <http://127.0.0.1:8765/>. It runs the firmware UI with synthetic events and RAM-only progress. It does not capture USB radio data or write SD logs.

## Hardware

[Buy the tested Hosyond 4-inch ESP32 board](https://amzn.to/4d4VI5R) (affiliate link).

The display target is the LCDWiki E32R40T / tested Hosyond 4-inch ESP32 board with ST7796 display and XPT2046 touch. Firmware uses **ESP-IDF 6.0.2**, pinned to `7101770dc6db2667b3c477cc31365dd1acd6db4e`.

After activating the pinned SDK in a checkout without spaces:

```sh
idf.py build
./tools/flash.sh /dev/ttyUSB0
```

Install the initial release with the [browser flasher](https://surveillancehound.dx.pe/flash/) (full installation resets saved board data), or build from source. Additional board support is coming soon.

See [Building](https://surveillancehound.dx.pe/BUILDING/) and [getting started](https://surveillancehound.dx.pe/getting-started/) before flashing. An ESP32-C3 has a separate [radio-only harness](https://surveillancehound.dx.pe/C3_RADIO_TEST/).

## Status and scope

67 rules cover 19 categories. Hound listens for selected 2.4 GHz Wi-Fi management frames and legacy BLE advertisements; it does not associate, probe, advertise, inject or upload. Quiet scans are not an all-clear, and Travel Watch detects repeated nearby presence without proving movement.

Initial display/SD checks and host regressions are documented. Controlled RF accuracy, durability/power tests, formal soak and release/license acceptance remain open. The [status page](https://surveillancehound.dx.pe/STATUS/) is the current summary; dated records preserve earlier evidence.

## License

Copyright 2026 Jascha Wanger ([https://dx.pe](https://dx.pe)).

Firmware, tools and tests: Apache-2.0. Original docs/art: CC-BY-4.0. Independently assembled signature facts: CC0-1.0, with separate source terms. See [LICENSE](LICENSE), [NOTICE](NOTICE), [dependency ledger](https://surveillancehound.dx.pe/licenses/) and [provenance](data/provenance.yaml).
