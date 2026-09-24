<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Code map and runtime

Hound separates reception, portable detection/game logic, rendering and storage so most behavior can be tested without hardware.

| Path | Responsibility |
| --- | --- |
| `main/app_main.cpp` | Coordinates bounded observation processing, UI and storage requests |
| `components/radio/` | Passive Wi-Fi/BLE reception and time slicing |
| `components/sniffer_core/` | Parsers, matchers, confidence, identities, pet state and tracking |
| `components/ui/` | Shared device UI, layouts, dialogue and drawing |
| `components/storage/` | NVS generations, SD logging, validation, backup and export |
| `data/signatures/` and `data/provenance.yaml` | Reviewed rule inputs and source evidence |
| `assets/pets/` and `tools/pack_assets.py` | Original pixel geometry and reproducible asset generation |
| `generated/` | Generated rules/assets; update their source inputs instead |
| `tools/simulator/` and `tools/simulator.cpp` | Shared controls and synthetic session, used by native and WebAssembly adapters |
| `tools/simulator_browser.cpp` and `tools/simulator_crypto.cpp` | Browser exports and Web Crypto bridge; runs in one Web Worker per lab |
| `tools/build_web.py` and `tools/test_web.mjs` | Pinned browser build and native/browser state + pixel comparison |
| `tests/host/` | Portable regression tests, including production storage code with fakes |
| `tests/c3_radio/` | Radio-only C3 test harness |
| `docs/` and `zensical.toml` | Documentation source and site configuration |

## Observation flow

Radio callbacks copy bounded input into a fixed queue. Parsers normalize management frames and advertisements. The engine matches enabled rules, fuses independent evidence and applies update cooldowns. A detection may update the log, contribute to discovery statistics, pass an alert gate or earn a meal; these decisions are separate.

Follow Scent consumes fresh reception samples for its selected identity. Travel Watch maintains a bounded repeated-presence window. Neither supplies location or proves movement. Storage serializes allowed fields and handles NVS/SD operations on its worker.

See [Building](BUILDING.md) for concrete memory budgets, [coverage](DETECTIONS.md) for detector bounds, and [privacy](PRIVACY.md) for retained data.
