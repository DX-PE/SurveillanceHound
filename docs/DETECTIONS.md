<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Detection coverage and limits

The development pack has **67 enabled rules covering all 19 categories**. Rules describe observable clues, with conservative labels and confidence caps. Source review is separate from a measured product-identification accuracy. All are spoofable.

| Category | Clues used | Important limit |
|---|---|---|
| FLOCK | Documented B4:1E:52 vendor prefix, Flock name/SSID forms, external-battery name and 128-bit service | Name or vendor evidence alone does not identify a camera model; GATT services may not be advertised |
| AXON | AB2/AB3/AB4/AXON name prefixes and vendor prefix | Names and addresses can be changed |
| GLASSES | Meta-assigned FD5F service | Meta service clue; not proof of glasses |
| SKIMMER | HC-03/05/06, RN42, BT04-A module names | Low only; ordinary hobby modules are common |
| RAVEN | 3100–3500 service family | Low only; generic UUIDs and non-advertised GATT services cause ambiguity/misses |
| AIRTAG | Exact Apple Find My manufacturer layout, subtype 12 | Find My beacon, accessory unknown; deliberately excludes subtype 07 AirPods traffic |
| TILE | FEED/FEEC services | Tile service clue, model unknown |
| SAMSUNG_TAG | FD5A service | Samsung finding-service clue; confirmed receiving in the C3 room test, not isolated proof of one tag |
| GOOGLE_TAG | FEAA Find Hub frame layout; separate FE2C Fast Pair clue | Find Hub protocol match; Fast Pair alone remains Low and can be headphones |
| DRONE | Valid legacy Remote ID Basic ID, Location or System messages | Unauthenticated broadcast claims; unsupported transports/messages are ignored |
| ALPR | Motorola/Genetec vendor prefixes | Low only; vendor hardware is not proof of ALPR |
| CAMERA | Camera-vendor prefixes | Vendor clue; exact product and intent unknown |
| RING | Ring vendor prefixes | Vendor clue; exact model unknown |
| IBEACON | Exact iBeacon manufacturer layout and plausible signed TX power | Generic beacon protocol; alerts/meals off by default |
| FLIPPER | 3081/3082/3083 serial services and assigned company 0E29 | Spoofable service/company evidence |
| PWNAGOTCHI | Signature transmitter plus bounded IE-222 JSON name/version/pwnd_tot | Compact, uncompressed advertisements only; oversized, compressed and streamed forms unsupported |
| PINEAPPLE | Pineapple_ SSID prefix | Name clue only; renamed devices missed |
| DEAUTH | 20 deauth/disassociation frames from one source/channel within 10 seconds | Legitimate maintenance can cause bursts |
| EVIL_TWIN | Same raw SSID on concurrent open/protected APs; stronger score requires two vendor/PMF/IE/channel-RSSI differences | Mixed deployments can be legitimate; never proof of an attack |

`data/signatures.yaml` and `data/provenance.yaml` provide every exact value, source URL, score and review limitation. external detector reference facts were consulted with the user's authorization; implementation, fixtures and rule descriptions are project-owned. Primary specifications and original research were used where available. Reference-derived vendor mappings still need independent registry/field corroboration before a final release.

Names alone are capped Medium; generic or locally administered vendor-prefix evidence is capped Low. Same-kind evidence does not stack. Evidence expires after 30 seconds; repeats do not add confidence; contradictions subtract 20. The fusion ceiling remains the highest applicable rule cap.

## Bounds and identity

- Wi-Fi callback: at most 512 bytes; BLE callback: at most 31 bytes. Oversized frames are counted and dropped.
- Capture queue: 16 entries; processing budget: 16 per UI iteration. 128 cached observations, 24 AP summaries, 16 burst sources.
- Remote ID cache: eight transmitting addresses, separately expiring Basic/Location/System parts after 60 seconds. Evidence details show a volatile snapshot; serials/coordinates never enter logs or exports.
- Latest 32 emitted events are paged four at a time. Opening details freezes the selected evidence, even as new events arrive.
- Updates are limited to one per identity per 30 seconds except first sightings/grade increases, plus a global 100 ms limit.
- Meal identity uses category, radio, address and address type, omitting changing rule sets. No identity is inferred from RSSI or joined across rotating addresses. Pwnagotchi's shared signature transmitter is replaced with its transmitted source identity.
- 128 meal entries; 30-minute source cooldown (10 minutes for deauth), 30 seconds per category and five seconds globally. Latest 32 are persisted; restore uses a conservative 30 powered-on-minute hold.

Hardware covers 2.4 GHz Wi-Fi and legacy BLE. It misses 5/6 GHz, extended advertising, coded PHY, non-advertising devices and packets outside the current time/channel slice. No RSSI-to-distance estimate is made.
