<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Development status

Updated **2026-09-24**. This is a **0.1.0 development build**, not a completed hardware-validated release.

## Latest installed firmware

Coverage help and manufacturer/SSID inference guards are installed on the Hosyond. The application is 1,288,432 bytes, SHA-256 `a24290d8ecd957a7b4bea8dd50d145055544001ada1d819d3589db48403d8545`. Flash verification and pre/post partition/NVS/PHY comparison passed. A boot smoke check showed Wi-Fi/BLE activity and no reported radio/save errors; this does not close the RF accuracy or soak tests.

The [installation record](HARDWARE.md#coverage-help-and-inference-guards-installed-2026-09-24) and [verification](VERIFICATION.md) retain the evidence and limits.

## Implemented and pending

| Area | Implemented | Still to validate |
| --- | --- | --- |
| Hound and display | Six dogs; 33 frames each; care, outfits, compact alerts, landscapes, portrait, themes and saver | Formal readability, touch and accessibility checks across board revisions |
| Detection | 67 rules / 19 categories; bounded confidence, frame-role/OUI guards and Coverage help | Controlled product captures, false positives, capture loss and independent passive RF verification |
| Tag identity | Supported Samsung broadcast identity/state; selected-card Ignore fix; 64 saved ignores | Long-run recognition through ID rotation; AirTag rotation continuity is not implemented |
| Travel Watch | Ten-minute repeated-presence heuristic, progress and red warning; trigger seen on Hosyond | Controlled moving/stationary tests; no GPS or confirmed movement |
| Storage | Incremental SD validation, export feedback, safe eject, internal saves and authenticated ignore backups | Reader-based export/backup file checks, power interruptions, damaged/full media and long exports |
| Builds and tests | Pinned ESP-IDF 6.0.2; 15 host suites passed for latest firmware changes; release build passed | Remote CI result and final release sign-off; older four-profile evidence is dated separately |
| Memory and power | Bounded queues, diagnostics, memory reductions, manual calibration and muted audio | 80 KB minimum-heap target, measured power, speaker/battery tests and formal eight-hour soak |
| Documentation | Hound-themed Zensical site, browser lab and combined docs/lab workflow | Public hosting configuration and remote workflow execution |

## Hardware evidence

- Hosyond display colors are correct with inversion off.
- A 32 GB card mounted; a 581-record export, safe eject, remount and restored outfit were reported.
- Earlier overnight readings were about 82 KB free / 72 KB minimum, with increasing SD writes. The minimum-heap acceptance target is still unmet.
- A C3 room test received Samsung FD5A advertisements, but did not isolate a particular physical tag as their source.

Historical installation details remain in [hardware records](HARDWARE.md), [verification records](VERIFICATION.md) and the [September 23 status snapshot](history/status-2026-09-23.md).

## Before an OSS release

Complete the [release gates](RELEASE.md): retain accurate limitations, complete license/provenance review, record hardware acceptance and assemble verified artifacts. Source availability must not be presented as completed product-accuracy validation. Phone GPS, tag ringing, active probing and the planned detection expansions remain deferred or research-only; see the [roadmap](ROADMAP.md).
