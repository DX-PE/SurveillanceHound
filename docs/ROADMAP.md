<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Surveillance Hound roadmap

The roadmap follows hands-on exploration of an external browser emulator. Its interactions are references for original Hound implementations; no third-party sprites, text or implementation are incorporated. All characters remain dogs. Landscape remains the default, with the existing rotation lock and portrait support.

## Selected for this iteration

| Feature | Scope and acceptance criteria | Status |
|---|---|---|
| Scent Book | Illustrated cards for all 19 categories; explain clues and limitations, latest evidence/confidence, first/last manual UTC and lifetime observation counts. Persist category statistics without device identifiers. Clearly distinguish demo discoveries and unknown time. | Implemented; hardware validation pending |
| Dog wardrobe | Original bandana, collar, raincoat, detective cap and sunglasses; previews, explicit XP/discovery requirements, persistent equipment and earned unlocks, and an unlock celebration. Support all six dogs, both orientations, sleeping and eating. | Implemented; hardware validation pending |
| Snooze and Ignore | Alert actions and log access; 5/15/60-minute snooze with early resume; persistent, bounded per-device/category ignores with individual removal. Suppress pop-ups and sound while retaining logs, feeding and discoveries. Use keyed identifiers; keep demo state separate. | Implemented; hardware validation pending |
| Follow Scent | Select a frozen log identity; fresh RSSI, 30-second bounded history, recent trend, waiting/lost/paused states and reception-driven dog poses. Readings stay in RAM and never imply direction or distance. | Implemented after the first pass; hardware RF validation pending |
| Atmosphere and personality | Three original environments (Signal Grid, Dog Park and Rooftops) and three color themes; optional speech, varied petting replies/poses and compact patrol panel. Persistent real preferences, isolated demo copy; reduced motion and both orientations. | Implemented after Follow Scent; display/performance validation pending |

The implementation uses 16 saved ignore slots and six wardrobe choices (classic plus five accessories). Snooze is global and temporary; ignores target a device/category. All three features have separate RAM-only demo state. See the [README](../README.md#scent-book-outfits-and-quiet-alerts) for controls and unlock requirements.

## Planned additions

| Feature | Proposed Hound behavior | Scope / validation |
|---|---|---|
| Kennel / desk mode | Resting dog with clock, selectable background and optional 25-minute focus timer. Scanning status stays visible; alerts can interrupt. | Keep Start/Stop distinct from desk mode; handle unset time and snooze consistently. |
| Dog diary and badges | Lifetime discoveries, first finds, session records, petting count and cosmetic milestones. Consider an optional Bingo-style discovery board. | Counts describe radio observations, not confirmed surveillance devices. Avoid rewarding duplicate-event floods. |
| Guided first run | Dog-led control walkthrough and confidence explanation, replayable from Settings. Interactive panel color-order/inversion check. | Retain touch calibration and privacy/region choices; validate panel controls against supported hardware revisions. |
| Hound Pack | Optional nearby Hound visits with dog appearance/name and potentially short messages. | Future design only. Explicit opt-in is required because communication transmits, unlike default passive scanning. Define identity, pairing, privacy, authenticity and radio scheduling before implementation. |

## Tag Watch

Implemented a manual Travel Watch for repeated AirTag/Find My, Samsung, Tile and Google tag presence: ten minutes, eight distinct minute bins, gaps no longer than two minutes. The compact possible-following warning slowly flashes the background red; steady red, acknowledgement, Ignore, Snooze and Stop remain available. A simulator time-jump preview exercises the same threshold. Per-tag status shows observation time, qualifying minutes and last-seen age, with reset counters and alert-filter hints. The table is bounded and volatile, with keyed identifiers and no new persisted record. This does not establish movement, ownership or intent; address rotation and table capacity can cause missed warnings. The user confirmed the alert trigger and red flashing on the Hosyond board, with the dog and controls visible. Controlled RF validation and broader physical acceptance remain pending.

## Existing deferred work and release gates

- Phone GPS/companion work remains deferred at the user's request.
- Initial Hosyond display bring-up and short no-card heap measurements are complete. Remaining acceptance work includes formal display/touch checks, NVS/SD power interruption, sound, battery/current draw, minimum heap, controlled capture loss, cold boots and the long soak. SD testing is on hold at the user’s request. See [status](STATUS.md), [hardware](HARDWARE.md) and [test plan](TEST_PLAN.md).
- Controlled positive/negative device captures, field confidence review, remote CI, license review and signed release acceptance remain separate gates.
- A simulator result does not establish RF accuracy, durable NVS writes or physical display performance.
