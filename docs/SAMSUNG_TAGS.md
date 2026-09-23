<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Samsung tag identity and reported connection state

Hound now recognizes the documented Samsung FD5A finding advertisement. An exact match of its eight-byte broadcast ID can preserve an identity across changing Bluetooth addresses. This improves Ignore and repeated-presence tracking, but does not establish permanent physical-device identity or ownership. The broadcast ID can also rotate.

## Supported wire format

`parse_samsung_tag` accepts exactly 22 service-data bytes: the little-endian UUID `5A FD`, followed by the documented 20-byte payload. Offsets below exclude the UUID:

| Payload field | Interpretation |
|---|---|
| Byte 0, bits 7–3 | Version 1, advertisement type 0: `(byte & 0xF8) == 0x10` |
| Byte 0, bits 2–0 | 1: recently separated; 2: offline; 3: long offline; 5: connected; all other values: unknown |
| Bytes 4–11 | Eight-byte broadcast/privacy identifier |
| Other bytes | Not retained in parsed metadata or used to infer ownership |

All-zero/all-FF identifiers, other versions/types and incomplete/longer service data fall back to the existing FD5A service clue. A malformed BLE AD envelope remains a parser failure. Unknown states never imply an owner connection. The parser does not validate an authentication tag or connect to the device.

Protocol evidence: [AirGuard SamsungTracker.kt](https://github.com/seemoo-lab/AirGuard/blob/main/app/src/main/java/de/seemoo/at_tracking_detection/database/models/device/types/SamsungTracker.kt) and [SmartTag research parser](https://github.com/pogen300/galaxy_smarttag_research/blob/main/other_scripts/parse_tag_ble.py), inspected 2026-09-22. This is an independent bounded parser of the documented fields; no AirGuard implementation code is incorporated. Unlike broad fallback state handling, only the explicit state value 5 suppresses alerts. [AirGuard’s FAQ](https://airguard.seemoo.de/en/docs/faq/) also describes the limitation of changing Samsung identities. Protocol evidence is not a substitute for captures of the user's actual tag models/firmware.

## Behavior and compatibility

- Same supported broadcast ID: one detection-cache identity across MAC/address-type changes, shared meal cooldown, saved Ignore, session log pseudonym, Follow Scent and Travel Watch history.
- Different supported IDs: separate identities, even if they use the same MAC. No timing/RSSI-based merging and no claim that two nearby signals imply exactly two physical tags.
- Owner-connected report: ordinary card and sound eligibility are suppressed; the matching pending card and Travel Watch window clear. Logging, recent counts, collection and normal earned meals continue. Offline or unknown reports restore normal eligibility. State changes can emit before the ordinary 30-second repeat interval, still within the global event-rate limit.
- Unknown/unsupported format: existing service-only score and normal alert policy; no inherited connected-state suppression. All 67 rules and 19 categories retain their current confidence scores.
- Ignore stores a domain-separated keyed digest in the shared 64-entry list. Old address ignores are also checked. The list never evicts silently. The later Companion V2 expansion migrates the earlier 16-entry saves and supports authenticated SD backups; see [Ignore storage](IGNORE_STORAGE.md). Older ignores cannot be upgraded without observing and associating their current identity; a user may need to Ignore a current sighting once more. Do not delete all saved ignores as part of this update.
- Broadcast identifiers stay in bounded volatile parser/detection/UI state. Private log identifiers are session-keyed pseudonyms; exports do not receive raw Samsung IDs. Research mode retains its existing raw-MAC behavior. No GPS, GATT connection, cloud service or active scan is added.

In **Log → Samsung sighting → More**, the evidence snapshot shows the reported state, broadcast-ID/radio-address basis and current Ignore status. The Ignore screen explains that the current identifier can change. A connection report is an unauthenticated broadcast claim, not a determination that the tag is yours.

## Validation and remaining hardware checks

The Samsung regression covers every header byte and state, all relevant lengths, invalid identifiers, UUID-only fallback, same-ID MAC changes, different IDs sharing a MAC, pseudonym/key isolation, meal cooldowns, legacy ignores, saved-companion reload, demo isolation, 24 simulated hours of Ignore, Travel Watch continuity/reset, Follow Scent, and unrelated pending alerts. General parser mutation testing includes synthetic structured Samsung service data and BLE envelopes. No real tag identifiers are checked into fixtures.

On the Hosyond, inspect each Samsung tag’s **More** page after installation and record whether the format is supported and what state it reports. Ignore any currently unignored broadcast ID, then run with the two tags for several hours; use the evidence page to distinguish unsupported packets, a changed broadcast ID and a UI suppression failure. Check a deliberately unignored offline tag still alerts. Owner-connected/offline RF transitions, long-run rotation, live heap/drop rates and the SD soak remain physical acceptance work. A future changed privacy ID may still produce NEW SCENT; this implementation does not claim permanent owned-tag enrollment.
