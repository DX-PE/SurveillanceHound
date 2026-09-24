<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Detection expansion: research and coding handoff for items 1–8

Reviewed **2026-09-23**, against commit `37c33927a074d22a2a41e53b3ce6d11647a695c3`. This is the implementation handoff for [roadmap items 1–8](ROADMAP.md). The research itself added no enabled rules or hardware validation. The live baseline remains **67 rules / 19 categories**. This independent vendor/standards review does not retroactively verify the historical signature catalogue.

## Implementation status (2026-09-23)

Items **1–2** and the shared SSID frame-role prerequisite are implemented. Coverage has five help pages reachable from every Scent Book page; Back preserves the page, and help does not affect discoveries, scanning or display idle policy. OUI inference requires a globally administered individual transmitter address. SSID rules require beacon/probe-response frames; payload identifiers and BLE matching remain independent. All 15 host suites and the ESP32 release build pass; host tests use address, undefined-behavior and leak checks. The 67 rules / 19 categories and persistent state formats are unchanged. Installed on the Hosyond on 2026-09-24; the application hash and saved-data preservation were verified, with a successful boot/radio smoke check. Physical UI/SD checks and controlled RF testing remain pending.

The baseline findings below describe the reviewed commit; the implemented sections note the current behavior. Candidate rules in items 3–8 remain research work.

## Readiness and recommended order

“Documented” means a source describes the field; it does not mean Hound has received it from hardware. **No new candidate in this report has a controlled RF capture.** Firmware versions are unknown unless explicitly stated. Scores below are proposed Hound policy, not measured accuracy or vendor claims.

| Roadmap item | Research result | Next coding session |
|---|---|---|
| 1. Coverage help | Hardware limits and receiver boundaries established | Implemented: five pages, navigation and UI regression checks |
| 2. Local-address inference | IEEE semantics established; baseline matcher only lowered confidence | Implemented: reject local/group transmitter-OUI evidence; preserve other evidence |
| 3. Camera vendors | Registry downloads blocked; useful product/setup documentation found | Ready: audit existing vendor-only confidence. New OUI mappings remain blocked; Blink setup name is a separate capture candidate |
| 4. Dashcam names | Exact model prefix for BlackVue; family prefix for Thinkware; indexed model names for 70mai; gaps for Viofo/Garmin/Nextbase | Add disabled fixtures/candidates for documented names after the SSID frame-role guard; enable individually after captures |
| 5. O.MG / Hak5 | O.MG exact setup SSID documented; Key Croc documentation describes client networking | O.MG matcher is straightforward, but needs a truthful category and capture. Do not add a speculative Key Croc AP rule |
| 6. Fleet / retail | Cradlepoint examples and analytics capabilities documented; neither proves police use nor tracking enabled | Design neutral informational presentation first; no new alerting vendor rules |
| 7. Low-cost cameras | SmartLife is a general pairing clue; proposed camera-app strings remain unverified as radio names | Keep broad names out of camera alerts; acquire model-specific evidence |
| 8. Generic modules | HC-08 default documented; HM-10 manual says `HMSoft`; name may be unavailable to passive scanning | Informational design and passive captures first; no generic skimmer alarm |

Suggested small patches: **A:** item 1; **B:** item 2 plus the SSID frame-role guard below; **C:** item 3 confidence audit; **D:** item 4 disabled candidates/tests. Prepare item 5 after category design, then items 6–8 as opt-in informational work. Documentation research is complete for this pass; unresolved evidence is an explicit follow-up, not permission to ship guesses.

## Constraints discovered at the reviewed commit

| Area | Baseline behavior | Consequence for implementation |
|---|---|---|
| Radio receiver (`components/radio/radio.cpp`), parser (`components/sniffer_core/parsers.cpp`) | Wi-Fi management frames; BLE discovery is passive and accepts at most 31 advertisement bytes | Existing connected-camera traffic may be missed. A BLE local name supplied only in a scan response cannot be assumed observable. Do not enable active scanning to rescue these rules |
| Matcher (`components/sniffer_core/engine.cpp`), `matches()` | SSID exact/prefix matching checks radio, but not management subtype | A phone probing for a saved camera network could match that camera rule. Frame-role guard now implemented before expanding SSID rules |
| Same matcher | OUI matches the parsed transmitter address, including local addresses; `cap()` only limits local OUI confidence to 49 | Item 2 now rejects that evidence rather than merely lowering its score |
| Compiler (`tools/compile_signatures.py`) | Literal, case-sensitive exact/prefix names; 24-bit OUIs only; no regex or suffix constraints | Do not write wildcard strings into `matcher.value`, assume a model number is the broadcast name, or broaden a 28/36-bit allocation to 24 bits |
| Engine scoring | Evidence kinds are grouped; generic clues cannot provide the independent-evidence bonus | Mark vendor/module-only evidence generic; repeated weak sightings must not become Strong |
| Alert gate (`components/sniffer_core/include/sniffer/core.hpp`), `should_alert()` | Threshold defaults to 50; UI supports 20/50/80; `generic` does not silence alerts | Score 30 is quiet at the default threshold but alerts at 20. Informational candidates should have score/cap 19, or an explicit separately tested quiet policy |
| Engine/game and Companion (`components/sniffer_core/include/sniffer/companion.hpp`) | Recent counts exclude scores below 20; meals need 50, or 20 with low-confidence feeding. `Companion::observe()` has no score gate | Score 19 avoids ordinary alerts/counts/meals, but can still contribute a discovery and outfit unlock if routed through the existing collection path. Define informational discovery policy explicitly |
| Category/persistence/UI | 19 fixed categories; category arrays occur in saved state; UI and simulator contain fixed lists/page counts | Dashcams fit `CAMERA`. O.MG is not a Pineapple, a fleet router is not an Axon device, and a UART module is not a confirmed skimmer. Do not force these into misleading categories to avoid migration work |

### Shared prerequisite: match an advertised SSID to its sender

Implemented in `matches()`: require `Radio::Wifi` **and subtype 8 (beacon) or 5 (probe response)** for `Kind::SsidExact` and `Kind::SsidPrefix`. Preserve subtype 4 parsing for future passive request/response correlation. A directed probe request names a network being sought, not a product hosted by the phone. This guard is for SSID product evidence; it must not disable unrelated protocol/behavior detection.

Tests: the same synthetic SSID in a beacon and probe response matches; in a probe request it does not. Exercise both exact and prefix kinds through `parse_wifi()` and the engine, not only fabricated `Observation` fields. Update existing direct-observation fixtures to state their subtype. Test leading junk, case changes, embedded NUL, empty SSID, 32-byte boundary, duplicate SSID IEs and truncated frames. Prefix matching intentionally accepts arbitrary suffixes; it is not validation of serial-number syntax and remains spoofable.

## 1. Explain coverage in the UI

The original ESP32 supports 2.4 GHz 802.11b/g/n and Bluetooth including BLE. Hound's implementation uses passive BLE advertisements and selected Wi-Fi management frames; Classic Bluetooth inquiry and arbitrary radio interception are not current features. Hardware capability must not be confused with implemented reception. [Espressif datasheet v5.3][esp32]

Implemented a **Coverage** entry reachable from the Scent Book, with a short multi-page explanation and Back control. Keep it separate from the 19 discovery categories: viewing help must not create a discovery or alter persistence. Research copy below; the shipped wording is split across five pages to fit both orientations:

- **What Hound hears:** “Hound listens for recognizable 2.4 GHz Wi-Fi and Bluetooth LE broadcasts. Nearby devices can be missed between scan windows.”
- **What a clue means:** “A name or manufacturer clue does not prove a camera is recording, who owns it, or that someone is tracking you.”
- **What stays unseen:** “Silent listeners, wired-only devices and cellular-only equipment may give Hound nothing to hear. It cannot receive 5/6 GHz Wi-Fi or other radio systems.”
- **Quiet does not mean clear:** “No matching signals observed. This is not a complete device inventory or a safety guarantee.”

Detailed help should name silent PAX-style counters, cellular-only trackers/ALPR, cellular interception equipment, acoustic-only sensors, and optical/IR/radar/RFID/LoRa systems as outside these detectors unless they separately emit a supported identifiable broadcast. Radio frequency overlap alone is insufficient. Travel Watch reports repeated nearby presence; it still has no location/motion proof.

Touch points: UI implementation (`components/ui/ui.cpp`), UI types/layout (`components/ui/include/ui.hpp`), field guide (`components/ui/include/scent_guide.hpp`), simulator navigation only if useful, and the coverage sections in [DETECTIONS.md](DETECTIONS.md). Check “ALL QUIET. GOOD COMPANY.” and empty-log text in context; keep pet personality, but make the coverage limitation accessible without an alarm.

Acceptance: landscape and portrait; all themes; Back restores the correct page; controls fit; help never interrupts scanning or resets idle timers without user interaction; no new persistent field, connection or category required.

## 2. Stop manufacturer inference from locally administered Wi-Fi addresses

IEEE defines the U/L bit as bit 1 of the first displayed MAC octet. A set bit denotes local administration, not necessarily randomization. The I/G bit is a separate bit 0. A protocol OUI or CID within an information element is not a transmitter address. [IEEE EUI/OUI/CID guidelines][ieee-guidelines]

Implemented predicate for `Kind::Oui`: Wi-Fi, `(o.address[0] & 0x03) == 0`, then existing prefix comparison. This rejects both local administration and group transmitter addresses, with all four bit combinations tested. Do not clear either bit to manufacture a vendor match. Retain the cap defense, but it should no longer be the only protection.

Apply this only to transmitter-MAC manufacturer inference. Local addresses may still carry a valid SSID or protocol signature. Leave BLE address-type handling, BLE company IDs, service UUIDs, payload OUIs/CIDs, Pwnagotchi decoding and behavior rules intact.

Acceptance fixtures:

1. A synthetic globally administered prefix matches its OUI rule; an independently matching locally administered prefix does not. Testing only an address whose bytes no longer match the rule would miss the bug.
2. A local Wi-Fi address with a matching advertised SSID still matches that name rule.
3. The same local address with valid vendor-payload evidence still matches that payload rule, including an identifier whose own first byte has bit 1 set.
4. Existing BLE public/random address fixtures and supported tag ignores remain unchanged.
5. Local OUI alone produces no detection, alert, meal or discovery. Name/protocol evidence retains its own confidence without an OUI bonus.

Primary files: `components/sniffer_core/engine.cpp`, `tests/host/core_tests.cpp`; review live local-prefix entries and their provenance rather than silently repurposing them. No change to the radio callbacks or saved-state format is needed.

## 3. Camera-vendor clues and the existing confidence audit

The official IEEE [MA-L CSV][ieee-mal], [MA-L text][ieee-text], [MA-M][ieee-mam] and [MA-S][ieee-mas] endpoints returned **HTTP 418** on 2026-09-23. No new allocation is independently verified in this report. Do not substitute an unsourced MAC-lookup page or infer a camera brand from a module supplier.

Before adding an OUI: save registry name, exact allocation and bit length, retrieval date and snapshot hash; record whether it identifies the product vendor, an OEM, a hub or a module supplier. Verify a known unit's *radio transmitter* during the relevant operating state. An Ethernet-label MAC or app device ID is not automatically the Wi-Fi address. MA-M/MA-S require a real masked-prefix matcher; truncation would attribute other allocations to the vendor.

| Candidate | Primary evidence found / useful distinction | Disposition |
|---|---|---|
| Dahua | Web 3.0 manual v3.0.1, printed p.90, describes optional AP mode, disabled by default, with a serial-derived `_SD` name and band choice. This is not a universal Dahua SSID. [Manual][dahua] | OUI pending registry + captures; do not invent `Dahua-` or match every `_SD` suffix |
| Lorex | W482CAD troubleshooting describes camera-to-router Wi-Fi. It supplies no vendor allocation or identifying camera AP pattern. [Support][lorex] | Pending; do not assume Dahua and Lorex prefixes are interchangeable |
| Reolink | E1/TrackMix/Video Doorbell setup can provision router credentials by QR code or Ethernet. [Setup guide][reolink] | Pending; “uses Wi-Fi” is not proof it advertises a branded AP |
| Uniview | IPC322LB-AF28WK-G specifies 802.11b/g/n. [Product specification][uniview] | Radio capability established for that model; OUI/pattern unverified |
| Hanwha / Wisenet | XNF-9013RV manual, printed p.16, describes an installer Wi-Fi dongle and several third-party dongle suppliers. [Manual][hanwha] | Capture the actual dongle/role; its radio vendor need not be Hanwha |
| Arlo | SmartHubs/base stations advertise a network to synchronize cameras. [Support][arlo] | A hub clue is not one detected camera, a count of its cameras or recording status; exact pattern still needed |
| Eufy | Indoor Cam S350 supports both 2.4 and 5 GHz. [Model support][eufy] | Capture by model/band; do not generalize older “all Eufy is 2.4 GHz” guidance or use every Anker allocation as a camera |
| Blink | Support documents temporary `BLINK-` + four serial characters during setup for cameras **and Sync Modules**. [Support][blink] | Capture candidate: `wifi_ssid_prefix` / `BLINK-`; label “Blink setup network”; cannot distinguish camera from hub |
| Google / Nest | Official specifications list differing Wi-Fi/BLE capabilities across models. [Specifications][nest] | No dedicated passive product signature verified here; include speakers, displays and thermostats as negative controls |

**Existing issue to address:** the seven `wifi.camera.*` OUI entries (`00408C`, `B8A44F`, `E0A700`, `701AD5`, `C056E3`, `4419B6`, `2CAA8E`) and the fourteen `wifi.ring.*` OUI entries currently use score 60 / Medium / non-generic. This is an audit inventory, **not registry validation**. Proposed vendor-only policy: score 30, cap Low, `generic=true`, labels that identify a vendor clue rather than a proven camera/doorbell. This changes alert behavior and must be reviewed explicitly, including matching provenance fields. Do not quietly preserve Medium just because it shipped previously.

At threshold 50 these proposed OUI clues are quiet; at threshold 20 the user has requested weak alerts. For very broad vendors spanning unrelated products, defer the rule or route it to informational context rather than `CAMERA`. Two weak vendor/name clues must not be promoted into a product assertion. Regression tests should verify the desired combined score/cap, repeated sightings and manufacturer overlap.

Blink's setup candidate can use `CAMERA` only with a clear camera-system/hub limitation in the field guide and visible rule label. Suggested initial score 45 / Low until model captures support a more specific interpretation. Hidden SSIDs, a camera already connected to a router, Wi-Fi disabled, and 5 GHz-only traffic are expected coverage gaps. Ordinary data-frame capture remains roadmap item 18, outside this session.

## 4. Dashcam SSIDs

Use a `CAMERA` rule with a visible model/family label such as “Possible BlackVue dashcam network”; explain “network name match; recording unknown.” Proposed score 60 / Medium for a validated product-specific name. Do not add a separate persisted dashcam category just for these first rules.

| Candidate key | Documented field and scope | Literal matcher proposal | Evidence readiness |
|---|---|---|---|
| `dashcam.blackvue.590xplus` | DR590X Plus instructions show `BlackVue590XPlus-******`; user can change SSID. [Manual][blackvue] | `wifi_ssid_prefix`: `BlackVue590XPlus-` | Document directly read; capture actual suffix and firmware before enablement |
| `dashcam.thinkware.link` | F200/F200 PRO/F790/F800/M1 connection article shows `Thinkware_xx`, example `Thinkware_68`. [Support][thinkware] | `wifi_ssid_prefix`: `Thinkware_` | Document directly read; family-level label only, not individual model identification |
| `dashcam.70mai.m310` | Indexed manufacturer M310 manual p.04 gives `70mai_M310_XXXX`. [Manual][70mai-m310] | `wifi_ssid_prefix`: `70mai_M310_` | Search-index evidence only; direct URL returned 404. Reacquire manual + capture |
| `dashcam.70mai.m300` | Indexed M300 manual p.03 gives `70mai_M300_XXXX`. [Manual][70mai-m300] | `wifi_ssid_prefix`: `70mai_M300_` | Same access limitation; separate model rule, not broad `70mai` |
| `dashcam.70mai.m500` | Indexed M500 manual p.03 gives `70mai_M500_XXXX`. [Manual][70mai-m500] | `wifi_ssid_prefix`: `70mai_M500_` | Same access limitation; original and CDN URLs returned 404 |
| `dashcam.70mai.m200` | Indexed Dash Cam 3 manual gives `70mai_M200_XXXX`. [Manual][70mai-m200] | `wifi_ssid_prefix`: `70mai_M200_` | Same access limitation; product name “3” is not the SSID model component |
| `dashcam.viofo` | Support article updated 2026-05-21 describes VIOFO + model naming, without fixing delimiters/suffix; some models support 5 GHz. [Support][viofo] | None yet | Need exact model manual/label and captured SSID; do not guess `VIOFO_A229` |
| `dashcam.garmin` | Garmin describes a built-in network used with its phone app and separate infrastructure Wi-Fi on some models. [Support][garmin] | None yet | No exact radio name established; never treat all `Garmin` products as cameras |
| `dashcam.nextbase` | Indexed RIDE manual R01 p.38 shows `RIDE` + suffix, for a BikeCam. [Manual][nextbase] | None for production | Full PDF retrieval failed; broad `RIDE` is ambiguous and does not establish 422GW/622GW names |

`XXXX`, `xx` and asterisks above reproduce placeholder notation, **not literal matcher bytes or proven suffix alphabets**. Prefix rules alone cannot enforce lengths. Do not add case-insensitive matching or a broad brand substring as a shortcut. Modern models/bands are not covered merely because an older manual has a similar brand name.

BlackVue permits automatic Wi-Fi timeout; Thinkware's cited connection page explicitly says recording is paused while connected. These differences make “dashcam nearby” distinct from “recording you.” [BlackVue settings][blackvue-settings], [Thinkware connection guide][thinkware]

Acceptance: positive beacon/probe-response fixtures; a searching phone as negative; router SSID `MyBlackVue590XPlus-...` must not match; lower-case spelling must not match unless separately documented; harmless AP spoofing the exact name must retain only the name-based confidence. Capture hotspot on/off, renamed SSID, connected/disconnected phone, parking/recording modes, and 2.4/5 GHz separately. Repeats must retain existing cooldown/Ignore/Snooze behavior.

## 5. O.MG and other Hak5 gear

The O.MG creator's quick start directly documents **SSID `O.MG` after flashing**, and configurable Wi-Fi settings. Candidate `tool.omg.setup`: `wifi_ssid_exact`, value `O.MG`, proposed 60 / Medium; label “O.MG-style setup network.” This applies to the documented setup/AP state, not every cable/firmware/mode. `O.MG-123`, `O.MG_Cable` and `My O.MG` must not match an exact rule without separate evidence. An ordinary AP can imitate the name. [Creator quick start][omg]

Key Croc's documented `WIFI_SSID` is the network **it connects to**, not proof of a default AP called “Key Croc.” The official configuration page was available through its search index but direct fetch failed. No default Key Croc broadcast signature was established. Other Hak5 devices remain model-by-model research; neither a USB function nor an Ethernet interface establishes an identifying Wi-Fi broadcast. [Hak5 configuration][keycroc]

**Category dependency:** do not put O.MG in `PINEAPPLE` merely to reuse an animation. Recommended separate follow-up: append a neutral `RADIO_TOOL` category, with an original dog snack, alert toggle and honest name-only field guide. This is a design recommendation, not an implemented feature. Its coding change must migrate category-sized saved arrays, preserve existing numeric IDs/ignores, and update compiler/schema, export names, UI pagination and simulator lists. If that work is outside the small-change session, keep the O.MG candidate disabled.

Audit existing `Pineapple_` separately against an exact model/revision; this research does not newly validate the existing management-name rule or justify expanding it to all Hak5 gear. No association, network access, attack execution or probing is required for a name detector.

## 6. Fleet and retail infrastructure

| Candidate | Evidence and interpretation | Next step |
|---|---|---|
| Cradlepoint IBR900/IBR1700 | Official NetCloud CLI examples contain `IBR900-485` and `IBR1700-1ee`. These are examples, not proof of factory defaults for every revision. [CLI guide][cradlepoint] | Candidate prefixes `IBR900-` / `IBR1700-`, only after a model/default-state capture. Informational label “Possible fleet router” |
| Sierra Wireless / Semtech AirLink | RX55 AirLink OS 6.1 supports AP and client configurations; the configuration page does not establish a universal default SSID. [RX55 guide][airlink] | Obtain exact model and radio-equipped SKU, factory/manual revision and capture. Do not generalize cellular product names into Wi-Fi signatures |
| Getac / Panasonic Toughbook | Getac F110 G7 specifies Intel AX211; Toughbook 55 specifications also list Intel radios. [Getac][getac], [Panasonic][panasonic] | Board brand and Wi-Fi interface vendor may differ. No OUI/name rule approved; include ordinary Intel-equipped laptops as negative controls |
| Meraki | MR documentation describes Wi-Fi/BLE location analytics; collection is configurable. [Analytics][meraki], [settings][meraki-settings] | Recognizing an AP vendor does not expose its analytics configuration |
| Aruba | Aruba documents AP-supported retail presence analytics. [Retail white paper, p.17][aruba] | Capability alone is insufficient to label an observed AP “tracking active” |

Choose neutral informational presentation before adding these clues. Proposed score/cap 19 and no pop-ups, sound, screen wake, Travel Watch escalation, meals, XP or discovery-based outfit unlocks. Do not map any of them to `AXON`, `ALPR` or another security-device category. A new bounded “Radio clues” view/category needs its own resource and persistence plan; until then defer these rules.

A silent receiver has no required detectable advertisement. Dense venues are mandatory negative controls. A single router with several BSSIDs is not a count of police vehicles or retail tracking systems. Keep raw addresses out of saved research/export evidence; preserve the existing keyed-identity policy.

## 7. Low-cost camera setup networks

No model-specific primary-source/capture pair was established for the proposed `HDWiFiCam`, `V380`, `Yoosee`, `LookCam`, `IPC-`, `HIPC` or `A9` strings. App names, packaging names and actual broadcast SSIDs are different fields. Searches surfaced reseller/manual aggregators, but those are not adopted as verified manufacturer signatures here. Do not fill these gaps with guessed prefixes.

| Candidate group | Finding / decision |
|---|---|
| `HDWiFiCam`, `V380`, `Yoosee`, `LookCam` | Hold. Obtain an identified hardware model, manufacturer-authored manual and passive capture. Check whether the AP uses a UID rather than the app name. Don't ship a broad app-name match |
| `IPC-`, `HIPC`, `A9` | Hold. Short/generic strings lack sufficient product binding; `A9` is particularly ambiguous. A suffix or substring match would increase false positives |
| `SmartLife-`, `SmartLife_` | Tuya documents pairing names across general IoT products; hyphen/underscore variants appear in different APIs. These establish setup context, not camera type. [AP pairing][tuya-ap], [MCU protocol][tuya-mcu] |
| `Tuya`, `ESP_`, `ESP8266` | No camera assertion. Bare brand/chip strings are not a validated camera pattern; no universal ESP SSID default is established in this review |
| Espressif, Beken, Realtek allocations | Broad module suppliers; allocations still require registry verification. Never enable a camera alarm from module-vendor evidence alone |

If generic setup context is implemented later, share one informational candidate between items 7 and 8. The compiler prohibits duplicate evidence-kind/value pairs; do not classify the same `ESP_` name once as camera and once as skimmer. Require a specific additional product signature for a camera label. Signal strength, repetition or being in a rental property is not that signature.

Capture checklist: actual model/FCC label if present, firmware, factory setup, AP mode, normal client mode, Ethernet mode where available, Wi-Fi disabled, and several non-camera smart plugs/bulbs/relays using the same IoT platform. A negative sweep must never say the room is camera-free.

## 8. Generic module clues for skimmer investigations

The HC-08 manufacturer manual specifies software **V3.1**, hardware **V2.0**, dated **2017-07-07**; its command table and name section document default `HC-08` and configurable names. It describes BLE and a default service `FFE0`. Candidate `module.hc08`: `ble_name_exact` / `HC-08`, **informational 19**, only after proving that the name is in an advertisement Hound receives passively. [Manufacturer manual, printed pp.4–6][hc08]

The manufacturer-authored HM Bluetooth module manual V545 (2017-01), mirrored by Cornell, documents default **`HMSoft`**, not the hardware model string `HM-10`, in its name command section (PDF p.29 / printed p.41). Candidate `module.hmsoft`: `ble_name_exact` / `HMSoft`, informational 19, with the same capture prerequisite. A mirrored manual is evidence for that revision, not every clone or current firmware. [Datasheet][hm10]

| Remaining candidate | Decision |
|---|---|
| `JDY-xx` | Unverified family proposal. `xx` is not a matcher; identify exact model, firmware, actual local name and BLE versus Classic transport |
| `AT-09`, `MLT-BT05` | No manufacturer-controlled default-name evidence verified in this pass. Do not equate module labels with advertised names; keep disabled |
| `ESP_xxxxxx`, `ESP8266` | Shared generic Wi-Fi context with item 7, never standalone skimmer evidence |
| Service `0xFFE0` alone | A generic UART service clue, not a skimmer/product signature. A GATT service being present does not prove its UUID is advertised; no connection to discover it |
| Existing `HC-03`, `HC-05`, `HC-06`, `RN42`, `BT04-A` name rules | Audit transport and passive observability. HC-05 manufacturer documentation describes Classic SPP; the present BLE callback cannot discover a Classic-only module merely because a matching string is in the catalogue. [HC-05 manual][hc05] |

The new informational path must show “Generic radio module — purpose unknown,” remain quiet even at threshold 20, and avoid discovery/game rewards. A module might be a sensor, toy or legitimate serial adapter. Hound has no ATM/pump/location context and must not manufacture an intent score from RSSI. Review the existing SKIMMER field-guide/labels and advertised coverage before extending this category. Do not enable Classic inquiry, active BLE scanning or GATT access as part of this roadmap block.

## Coding checklist and release gates

### Evidence record for each candidate

Keep research candidates in documentation until approved for a disabled experimental rule. Do not drop a draft JSON file into `data/signatures/`: the compiler loads every `*.yaml` there. For a promoted rule, update catalogue (`data/signatures/catalog.yaml`) and provenance (`data/provenance.yaml`) together, using the compiler's exact schema. `experimental=true` must have `enabled_by_default=false`; “disabled” is not a substitute for the source record.

Record these fields before enablement:

| Record field | Required content |
|---|---|
| Candidate / rule ID | Stable unique ID, roadmap item and category decision |
| Match | Radio, frame/AD type, exact byte field, case-sensitive literal, prefix versus exact, score/cap/generic flag |
| Product | Manufacturer, model/SKU, hardware revision, firmware; write “unknown” where not established |
| State | AP/client/setup/idle/recording/connected mode; band/channel; whether name/UUID is in ADV or scan response |
| Documentation | Direct primary URL, owner, title, revision/date, page/section, retrieval date, access status; source licensing facts, not an assumed open-source license |
| Capture evidence | Capture date, known device, receiver/settings, passive reception result, positive and negative fixture IDs, redaction method, original-file hash in the private validation record |
| Interpretation | What the field establishes; expected missed states; spoofing and OEM ambiguity; no intent/recording/location claim |
| Validation | Host tests, exact build/rule-pack hash, on-device observations, heap/queue/storage changes; explicit enabled/disabled decision |

Public fixtures must remain synthetic or carefully minimized/anonymized, consistent with [fixture policy](fixtures.md). A synthesized `O.MG` beacon demonstrates parser behavior, not that a particular cable emits it. Do not upload surrounding devices' addresses, network names, credentials or private captures as part of this work.

### Implementation and test map

| Change | Files / tests to examine | Done when |
|---|---|---|
| Coverage entry | `components/ui/ui.cpp`, `components/ui/include/ui.hpp`, `components/ui/include/scent_guide.hpp`; `tests/host/ui_tests.cpp`, `feature_tests.cpp` | Help reachable in both orientations, no discovery/category migration, no clipped controls |
| OUI/SSID inference | `components/sniffer_core/engine.cpp`, `parsers.cpp`; `tests/host/core_tests.cpp`, `tests/fuzz/` | Local-only OUI evidence rejected; SSID requests cannot impersonate hosts; payload/BLE behavior retained |
| Existing camera confidence | Catalogue + matching provenance + field guide + `docs/DETECTIONS.md` | Vendor-only entries Low/generic as decided; threshold 20/50 behavior explicit; no accidental corroboration boost |
| Disabled name candidates | Catalogue/provenance only after source record ready; `tests/host/core_tests.cpp`, `test_tools.py` | Exact literals, source linkage, compiler validation, production pack unchanged while experimental |
| Truthful labels | `Rule::label`, log/detail rendering, Scent Book copy | A label stored in the rule actually reaches the UI; category title never overstates product/intent |
| New informational/category path | Compiler/schema, `core.hpp`, `companion.hpp`, UI/simulator, `components/storage/`, JSON/export code | Separate design reviewed; numeric IDs preserved, saved records migrated, policy tested for counts/discoveries/alerts/feeding; no unbounded lists or new background task |

For engine/catalogue changes, run the existing host checks described in [host test guide](testing.md) and [BUILDING.md](BUILDING.md). Commands are:

```sh
cmake -S . -B build-host -G Ninja -DSNIFFER_HOST=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

Keep sanitizers enabled as configured by the project. Compiler/provenance checks must pass and generated output must be deterministic. Include regression coverage for these user-visible behaviors:

- Monitor/detector toggles, alert thresholds 20/50/80, Snooze, sound mute and per-device Ignore; queued alerts cannot bypass a newly added Ignore.
- Samsung continuity and AirTag/Find My identity handling; a new camera/vendor clue must not change tag deduplication or Travel Watch eligibility.
- Repeat sightings do not increase confidence, repeatedly feed the dog or inflate physical-device claims. Informational-only additions do not create outfit discoveries through `Companion::observe()`.
- Screen dim/saver/off policy remains unchanged; quiet clues do not keep the screen awake. Demo observations remain isolated from saved stats/ignores.
- Stronger product evidence can supersede a vendor clue for the same existing identity without double-counting, while separate same-brand devices remain separate.

Before enabling a new hardware rule, test on the Hosyond with SD logging: record free heap, minimum heap, largest free block, radio queue drops, SD writes/backlog and UI responsiveness through BLE/Wi-Fi windows. Compare against a same-build baseline with that rule disabled, not an invented universal heap target. Do not resize caches/queues or add allocations merely to hold candidate names. A synthetic test pass is necessary but does not close the RF-validation gate.

### Open evidence work, in priority order

1. Capture a documented BlackVue or Thinkware model in 2.4 GHz AP mode and a phone probing for its remembered SSID. Record firmware and negatives; these are the clearest first dashcam candidates.
2. Reacquire the 70mai manuals from a working official endpoint, then capture exact named models. Obtain exact Viofo/Garmin/Nextbase model patterns before adding those families.
3. Verify `O.MG` from an identified cable/firmware in setup mode, once its category is designed. No Key Croc default AP claim until primary evidence exists.
4. Obtain a current IEEE allocation snapshot for all item 3 vendors; separately capture Blink setup and at least one non-camera hub. Audit the existing seven camera and fourteen Ring prefixes against that snapshot.
5. Capture HC-08 and HMSoft in genuinely passive mode with no nearby active scanner, then with a separate scanner to distinguish scan-response-only names. Include benign UART devices and connected/sleeping states.
6. Identify exact low-cost camera models and neutral fleet/retail examples. Avoid enabling the broad rejected patterns while waiting for evidence.

The research did not change the enabled 67-rule pack. The items 1–2 application was subsequently installed on the Hosyond on 2026-09-24 after SD eject confirmation; see the [hardware receipt](HARDWARE.md#coverage-help-and-inference-guards-installed-2026-09-24). Controlled RF acceptance remains separate from the successful boot smoke check.

## Sources and access notes

All sources were reviewed on **2026-09-23**. Live support pages without an explicit revision should be treated as mutable; pin a revision or record a snapshot hash in the rule evidence when promoting a candidate. Search-index-only/manual-access failures are called out in the relevant row rather than presented as a successful full-document read. The IEEE allocation failures are a verification blocker, not evidence that a vendor has no allocation. The original author of the HM manual is the primary source; Cornell hosts a copy. No third-party detector implementation was used for this report.

[esp32]: https://documentation.espressif.com/esp32_datasheet_en.html
[ieee-guidelines]: https://standards.ieee.org/wp-content/uploads/import/documents/tutorials/eui.pdf
[ieee-mal]: https://standards-oui.ieee.org/oui/oui.csv
[ieee-text]: https://standards-oui.ieee.org/oui/oui.txt
[ieee-mam]: https://standards-oui.ieee.org/oui28/mam.csv
[ieee-mas]: https://standards-oui.ieee.org/oui36/oui36.csv
[dahua]: https://materialfile.dahuasecurity.com/uploads/cpq/DOR/PUM0003423/202411/Dahua_Network_Speed_Dome_PTZ_Camera_Web_3.0_User_27s_Manual_V3.0.1.pdf
[lorex]: https://www.lorex.com/blogs/help/w482cad-series-2k-wi-fi-camera-troubleshooting-guide
[reolink]: https://support.reolink.com/articles/8247771140377-How-to-Initially-Set-up-WiFi-Cameras-via-Reolink-Client-With-LAN-Port-and-Voice-Prompt-New-Client/
[uniview]: https://global.uniview.com/Products/Network_Cameras/Easy_Series/EasyBasic_Series/IPC322LB-AF28WK-G/
[hanwha]: https://www.hanwhavision.com/wp-content/uploads/2022/12/Manuals_XNF-9013RV_220802_EN.pdf
[arlo]: https://www.arlo.com/en_gb/support/faq/connectivity-and-integrations/why-does-my-arlo-base-station-broadcast-its-ssid-and-is-that-a-security-risk
[eufy]: https://service.eufy.com/article-description/Wi-Fi-Connection-Issues-During-Setup1
[blink]: https://support.blinkforhome.com/en_GB/what-is-the-blink-wifi-connection
[nest]: https://support.google.com/googlehome/answer/9259110?hl=en
[blackvue]: https://manual.blackvue.com/docs/dr590x-2ch/playing-and-managing-videos/new-using-your-smartphone-android-ios-dr590x-plus-series/
[blackvue-settings]: https://manual.blackvue.com/docs/dr590x-2ch/changing-settings/new-changing-settings-using-your-smartphone-android-ios-dr590x-plus-series/
[thinkware]: https://support.thinkware.com/hc/en-us/articles/7913264076435--Thinkware-Dashcam-Link-Android-How-to-Connect-via-Wi-Fi
[70mai-m310]: https://www.70mai.com/instructions/70maiDashCam_M310_UserManual.pdf
[70mai-m300]: https://www.70mai.com/instructions/70maiDashCam_M300_UserManual.pdf
[70mai-m500]: https://www.70mai.com/instructions/70maiDashCam_M500_UserManual.pdf
[70mai-m200]: https://www.70mai.com/instructions/70maiDashCam_3_UserManual.pdf
[viofo]: https://support.viofo.com/support/solutions/articles/19000127932-how-to-use-viofo-app-
[garmin]: https://support.garmin.com/en-IE/?faq=Dm5QA79XXQ2DniThOGVMk8
[nextbase]: https://cdn.nextbase.com/nextbase-en-au/2018/07/NBDVR-RIDE-Instruction-Manual-English-R01.pdf
[omg]: https://o.mg.lol/setup/OMGCable/
[keycroc]: https://docs.hak5.org/key-croc/configuration
[cradlepoint]: https://docs.cradlepoint.com/r/Cradlepoint-Wi-Fi-Collection/Command-Line-Interface-Commands-for-Captive-Wi-Fi
[airlink]: https://source.sierrawireless.com/airlinkos/RX55-6.1/reference/hardwareinterfaces/howto/wificonfig/
[getac]: https://www.getac.com/content/dam/getac/product-spec-data-pdf/intl/Getac_F110_G7_WW_Product.pdf
[panasonic]: https://eu.connect.panasonic.com/gb/en/toughbook/toughbook-55-series
[meraki]: https://documentation.meraki.com/Wireless/Operate_and_Maintain/User_Guides/Monitoring_and_Reporting/Location_Analytics
[meraki-settings]: https://documentation.meraki.com/Platform_Management/Dashboard_Administration/Design_and_Configure/Organizations_and_Networks/Organization_Menu/Organization_Settings
[aruba]: https://www.arubanetworks.com/assets/wp/WP_Retail.pdf
[tuya-ap]: https://developer.tuya.com/en/docs/iot-device-dev/TuyaOS-iot_abi_network_config_AP?id=Kc67sz8ud0obw
[tuya-mcu]: https://developer.tuya.com/en/docs/mcu-standard-protocol/MCUSDK-wifi-base?id=Kd2bxu84567gk
[hc08]: https://www.hc01.com/downloads/HC-08A%20version%20english%20datasheet.pdf
[hm10]: https://people.ece.cornell.edu/land/courses/ece4760/PIC32/uart/HM10/DSD%20TECH%20HM-10%20datasheet.pdf
[hc05]: https://www.hc01.com/downloads/HC-05.pdf
