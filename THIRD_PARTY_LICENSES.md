<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Dependency and source ledger

No third-party detector code, assets or fixtures are incorporated. No external display, UI or touch library is used. Runtime dependencies are provided by the exact ESP-IDF SDK checkout.

| Dependency/source | Pin | License/review | Purpose |
|---|---|---|---|
| ESP-IDF | 6.0.2 / 7101770dc6db2667b3c477cc31365dd1acd6db4e | Apache-2.0 umbrella; individual components retain notices | Native SDK |
| ESP32 Wi-Fi / PHY / coexistence / BT libraries | SDK submodule commits | Apache-2.0 license files inspected in pinned checkout | Required radio hardware |
| Apache NimBLE, Espressif fork | SDK submodule commit | Apache-2.0 | Passive legacy BLE reception |
| FreeRTOS | SDK version | MIT | Scheduling and queues |
| Mbed TLS / TF-PSA-Crypto | SDK submodule commit | Apache-2.0 OR GPL-2.0-or-later; use Apache-2.0 option | HMAC/SHA on device |
| FatFs | SDK version | Permissive upstream license; retain SDK notice | SD FAT32 |
| lwIP / WPA supplicant / TLSF | SDK pins | BSD-family / permissive component notices | SDK transitive support |
| GCC and libstdc++ runtime | SDK toolchain 15.2.0 | Build compiler is GPL; linked runtime uses GCC Runtime Library Exception. No compiler code copied | Toolchain/runtime exception review required at release |
| OpenSSL | Host system library, tested 3.5.5 | Apache-2.0 for 3.x | Host crypto backend only |
| Python / CMake / Ninja | Host tooling | PSF / BSD-3-Clause / Apache-2.0 | Build tooling, not firmware |
| jsonschema / specifications / referencing | requirements-dev.txt exact versions | MIT | Host schema validation only |
| attrs / rpds-py / typing_extensions | requirements-dev.txt exact versions | MIT / MIT / PSF-2.0 | Schema test dependencies |
| clang-format | 20.1.8 | Apache-2.0 WITH LLVM-exception | Formatter only |
| Ruff | 0.13.1 | MIT | Python static/format checks |
| Zensical | 0.0.65; documentation dependencies in requirements-docs.txt | MIT; upstream dependencies retain their notices | Static documentation build only; not firmware |
| Emscripten / LLVM browser runtime | 6.0.10; compiler image digest in tools/build_web.py | Emscripten MIT / University of Illinois; LLVM Apache-2.0 WITH LLVM-exception and retained upstream terms | Browser simulator only; generated JS includes Emscripten attribution |
| musl / libc++ / libc++abi / compiler-rt / libunwind | Bundled with pinned Emscripten | Upstream license texts copied into lab-app/LICENSES.txt | WebAssembly runtime; not ESP32 firmware |
| ESP Web Tools | 10.4.0, pinned CDN entry module with SHA-384 integrity | Apache-2.0; upstream bundle retains dependency notices | Browser USB installer only, loaded from jsDelivr; not firmware or part of the passive radio runtime |
| Web Crypto | Browser implementation | Platform API; no crypto library bundled | Browser SHA-256 and HMAC bridge; native/device backends unchanged |
| Google Fast Pair / Find Hub documentation | URLs and retrieval date in provenance | CC-BY-4.0; factual fields independently normalized | Signature facts |
| STMicroelectronics beacon documentation | URL/date in provenance | Public factual protocol layout; no code/text copied | iBeacon facts |
| OpenDroneID header | Upstream master consulted 2026-09-21, not linked | Apache-2.0 | Experimental message format review |

The SDK is an upstream distribution with its own mixed component notices. This ledger records the initial review, not a claim that every transitive file has been audited. No GPL/AGPL runtime dependency without an applicable permissive option or explicit runtime exception is intentionally selected. The compiler's build-only GPL license is distinct from licensing the application. Final binary/license compatibility review and archival of all required notices remain release gates.

## Current build evidence

`tools/audit_binary.py` reads the linker map's included members, hashes the actual linked archives, records component source SPDX declarations, and archives SDK/toolchain license files. `make_release.py` includes this as `BINARY-EVIDENCE.zip`. Missing source tags remain `NOASSERTION`; component umbrella licenses are not silently promoted into legal conclusions. This improves traceability but does not certify compatibility of every selected object. GCC/libstdc++ runtime-exception review and the SDK's individual notices remain relevant.

External GPL-3.0 detector source was consulted with explicit user authorization for signature facts; no implementation or assets are incorporated. The current catalog includes re-expressed factual values from that consultation. Bluetooth SIG assigned-number facts, Flock research (MIT) and original protocol declarations supplement it. See the rule-level provenance for the exact distinction and review limitations.
