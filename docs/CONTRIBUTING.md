<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Contributing

Start with a small, reviewable change and describe the observed problem or proposed behavior. This repository is preparing for an open-source development release; [status](STATUS.md) distinguishes implemented features from validation still pending.

## Set up

- **UI and portable logic:** install the [emulator prerequisites](simulator.md), then run the [host tests](testing.md).
- **Firmware:** use the exact SDK and profile commands in [Building](BUILDING.md).
- **Documentation:** use the pinned tools and preview commands in [Documentation development](documentation.md).
- **Project structure:** consult the [code map](architecture.md).

## Changes and reviews

1. Explain the problem, reproduction steps and expected result. Include the board/controller revision and build hash when relevant.
2. Keep changes focused. Update the user guide when behavior or settings change.
3. Run the affected host tests plus generated-data, formatting and passive-operation checks. Report what ran and what still needs hardware.
4. Describe the result and evidence in the pull request. Do not present a synthetic fixture as a field accuracy measurement.

## Detection changes

Every enabled rule needs an independently understood matcher, conservative confidence, a provenance record, positive/negative synthetic fixtures, and explicit limits. Read [coverage](DETECTIONS.md), [signature research](DETECTION_RESEARCH_1_8.md) and the [roadmap](ROADMAP.md) before adding a new family. Generic module/vendor evidence must not claim a specific product or malicious intent.

The default scanner remains passive. Avoid adding network clients, active BLE/GATT operations, uploads, dynamic radio callback allocation or unbounded buffers as an incidental change. A new behavior that crosses these boundaries needs an explicit design and review.

## Reproduction data

Prefer synthetic fixtures. Do not publish surrounding devices' raw MACs, SSIDs, broadcast identities, private NVS backups, credentials or precise location in issues, screenshots or fixtures. [Privacy](PRIVACY.md) and the [fixture policy](fixtures.md) explain the data boundaries. Use sanitized exports when observation records are necessary.

## Compatibility

Persistent storage changes need migration and interrupted-save coverage. The `sniffer` namespace, `SNIFFER_HOST` build option, `SURVSNIFF` card directory and existing data schema names are retained compatibility identifiers, while the product and generated application binary are Surveillance Hound / `surveillance_hound.bin`.

## Licensing

Submit original work or properly attributed compatible material. Firmware/tools/tests use Apache-2.0; original docs/art use CC-BY-4.0; independently assembled signature facts use CC0-1.0 with separate source terms. Retain SPDX headers and update [attributions](licenses.md) when dependencies or assets change.
