<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# External signature review

For the independent primary-source review of roadmap items 1–8, use the [2026-09-23 coding handoff](DETECTION_RESEARCH_1_8.md). The historical findings below retain their original scope and do not describe new enabled rules.

## External reference review

**Historical review:** the later authorized implementation adds 67 enabled rules. See [DETECTIONS.md](DETECTIONS.md) and current provenance for shipped behavior; the findings below describe the earlier review.

Reviewed 2026-09-21 (local date), at the user's request. The source name, URL and revision are omitted from this published review. The original consultation involved GPL-3.0 source; this record does not claim independent primary-source verification.

## Findings

The external source inspection identified these candidates. Grades below are the reference's own labels, not our verification:

| Family | Candidate evidence | Limits |
|---|---|---|
| Flock | 29 MAC prefixes; only `B4:1E:52` graded high. Also `flock-` SSIDs and Bluetooth names. | Shared module prefixes are low. Vendor identity alone does not establish a camera model. |
| Axon | `00:25:DF` graded high; `E4:05:40` and `28:24:FF` low. SSIDs `AB2-`, `AB3-`, `AB4-`, `AXON-`. | Names can be changed or imitated. |
| AirTag | Raw advertisement searches for `1E FF 4C 00` or `4C 00 12 19`. | The broad first pattern also catches Apple accessories such as AirPods. No AD-boundary or full-length validation occurs inside this helper. |

Other candidates include Tile, Samsung SmartTag, Raven, glasses, Remote ID, Ring/cameras, Flipper and Pineapple.

The inspected external documentation was partly stale: it describes AirTag subtype checks superseded by the current byte searches, and its Axon OUI grades differ from the implementation. Its Flock section identifies generic module vendors as a source of false positives.

## Independent checks and implementation implications

The primary research paper [Track You, section 2.2](https://petsymposium.org/2023/files/papers/issue4/popets-2023-0102.pdf) documents Apple manufacturer ID `0x004C`, Find My type `0x12`, unpaired type `0x07`, and a status byte distinguishing device classes. A future Hound parser should validate complete AD structures, then label generic Find My traffic separately from an AirTag-specific classification. Synthetic positive and negative cases, including other Apple accessories, are needed before enabling it.

IEEE's public MA-L CSV and text endpoints returned HTTP 418 during this review, so vendor registrations were not independently verified. Searches of Axon's public documentation did not establish the listed SSID prefixes. These remain candidates requiring direct evidence and hardware captures; source comments are not validation.

Hound currently has disabled synthetic placeholders for Flock, Axon and AirTag in `data/signatures/catalog.yaml`. This review changed documentation only. It did not enable detection, change alerts, import third-party implementation code, rebuild firmware or restart the local simulator.
