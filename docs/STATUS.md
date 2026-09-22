<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Development status — 2026-09-21

The remaining software work requested for this iteration is implemented. This is still a **development build**; the E32R40T acceptance tests require the arriving hardware.

| Area | Current behavior | Verification still needed |
|---|---|---|
| Build | Pinned ESP-IDF 6.0.2; release/debug/test/demo; sanitizer tests and CI definition | Remote CI execution and release sign-off |
| Pet/UI | Six dogs; landscape default, rotation lock; all 19 snack motifs; level-up/touch reactions; Start/Stop with sleeping Zs; 32-entry paged history and evidence snapshots | Screen readability, touch accuracy and physical accessibility on E32R40T |
| Detection | 67 rules across 19 categories; 128-bit UUIDs; probe requests; structured uncompressed Pwnagotchi; Apple Find My distinction; bounded Remote ID assembly; corroborated security mismatch | Most product signatures have source evidence only; controlled positive/negative captures remain necessary |
| Storage | Complete-row validation at boot; bounded incomplete-tail repair; corrupt-log write lockout; full historical event export; free-space display; confirmed log deletion | Physical SD failures, card swaps, power interruption and long exports |
| Persistence | v1/v2 migration to v3; settings/pet/system namespaces; CRC-protected double slots and commit generation; fail closed on invalid state | Reboot/power-cut matrix on target NVS |
| Clock/power | Manual UTC with time-quality labels; three-point battery calibration; optional voltage/estimated percentage; critical save/flush/sleep; distinct muted-by-default effects | Meter calibration, threshold characterization, audio level and current draw |
| Diagnostics | Accepted/malformed per radio, queue drops, task watermarks, SD space, raw ADC, touch coordinates and display/touch self-test | Controlled capture-loss and runtime heap measurements on the target |
| USB C3 test | Shared passive radio/parser/rules received Samsung FD5A advertisements; Stop/Start and watchdog fix verified | Does not validate the E32R40T display, storage or power path |
| Packaging | Deterministic assets/rules; source SPDX inventory; linked-archive inventory, hashes and retained SDK/runtime notices | Final license compatibility review, field accuracy review, eight-hour soak and signed release |

Phone GPS/companion work remains deferred at the user's request. No cloud, active probing, sensitive-export option, on-device rule editor or BLE extended-advertising support is introduced. Those are outside this iteration. The UI history holds the latest 32 emitted events; the SD export streams the stored archive.
