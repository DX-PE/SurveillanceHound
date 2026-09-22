<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Development status — 2026-09-22

The initial software work and roadmap additions (Scent Book, wardrobe, Snooze/Ignore, Follow Scent and Atmosphere) are implemented. See [roadmap](ROADMAP.md) for the full emulator-inspired backlog. This is still a **development build**; the Hosyond display board is now connected and initial hardware bring-up has started.

| Area | Current behavior | Verification still needed |
|---|---|---|
| Build | Pinned ESP-IDF 6.0.2; release/debug/test/demo; sanitizer tests and CI definition | Remote CI execution and release sign-off |
| Pet/UI | Six dogs; landscape default, rotation lock; all 19 snack motifs; level-up/touch reactions; Start/Stop with sleeping Zs; compact alerts even without meals; distinct recent-signal totals; 32-entry paged history and evidence snapshots; Scent Book, earned outfits, Snooze/Ignore, fresh RSSI follow, themes, scenes and speech | Screen readability, touch accuracy and physical accessibility on E32R40T |
| Detection | 67 rules across 19 categories; 128-bit UUIDs; probe requests; structured uncompressed Pwnagotchi; Apple Find My distinction; bounded Remote ID assembly; corroborated security mismatch | Most product signatures have source evidence only; controlled positive/negative captures remain necessary |
| Tag Watch | Manual ten-minute repeated-tag heuristic; bounded keyed identities; compact possible-following card, slow red background flash/steady mode, ACK/Ignore/Snooze, per-tag timing/reset status and synthetic preview | User confirmed alert trigger and red flashing with dog/controls visible on Hosyond; no GPS/motion proof; rotating addresses can evade continuity; controlled moving/stationary RF tests pending |
| Storage | Complete-row validation at boot; bounded incomplete-tail repair; corrupt-log write lockout; full historical event export; free-space display; confirmed log deletion | Physical SD failures, card swaps, power interruption and long exports |
| Persistence | v1/v2/v3/v4 migration to v5; settings/pet/system/companion/appearance namespaces; CRC-protected double slots and commit generation; fail closed on invalid state | Reboot/power-cut matrix on target NVS |
| Clock/power | Manual UTC with time-quality labels; three-point battery calibration; optional voltage/estimated percentage; critical save/flush/sleep; distinct muted-by-default effects | Meter calibration, threshold characterization, audio level and current draw |
| Diagnostics | Accepted/malformed per radio, queue drops, task watermarks, byte-addressable heap/minimum/largest block, aggregate USB health and display/touch self-test | Initial heap measurements recorded in HARDWARE; minimum-heap acceptance, controlled capture loss and full soak remain pending |
| Display board bring-up | Hosyond ESP32 rev 3.1 / 4 MB attached; original flash backed up, Hound written and hash-verified; application displays; user confirms correct colors with inversion off; saved inversion toggle and frequent-controls-first Settings added; SD initialization reports no usable card | Physical toggle/persistence and touch checks; SD testing on hold; remaining acceptance tests |
| USB C3 test | Shared passive radio/parser/rules received Samsung FD5A advertisements; Stop/Start and watchdog fix verified | Does not validate the E32R40T display, storage or power path |
| Packaging | Deterministic assets/rules; source SPDX inventory; linked-archive inventory, hashes and retained SDK/runtime notices | Final license compatibility review, field accuracy review, eight-hour soak and signed release |

Phone GPS/companion work remains deferred at the user's request. No cloud, active probing, sensitive-export option, on-device rule editor or BLE extended-advertising support is introduced. Those are outside this iteration. The UI history holds the latest 32 emitted events; the SD export streams the stored archive.
