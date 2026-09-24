<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Controls and settings

The bottom navigation is **HOUND / LOG / SCAN / SET**. Tap the display in the emulator just as you would touch the board.

## Control reference

| Control | Behavior |
|---|---|
| Home → Start/Stop sniffing | Pause/resume scanning; show the sleeping hound while stopped |
| Settings → More: Display → Rotation lock / View | Unlock, choose landscape or portrait, then lock the chosen orientation |
| Settings → More: Display → Alert types | Toggle pop-ups and sounds per category while retaining sightings; the confidence threshold still applies |
| Detectors | Enable or disable observation processing for each category |
| Home → BOOK | Browse all 19 field-guide cards, statistics and recent evidence |
| Scent Book → COVERAGE | Five help pages on supported signals, silent devices and detection limits; Back returns to the same book page |
| Home → hound name / Settings → Dog wardrobe | Preview accessories, see requirements and equip unlocked gear |
| Alert or Log → details → Snooze / Ignore | Quiet all alerts for 5/15/60 minutes or ignore the selected device/category |
| Settings → first page | Manage ignored scents, resume alerts, open Scent Book, wardrobe or Atmosphere |
| Settings → More: Display → Display options | Brightness, immediate saved Panel inversion toggle, animation, color/touch test and screen saver |
| Settings → More: Display → Display options → Screen saver / Timeouts | Configure independent idle timeouts or preview your bouncing hound; first waking touch only wakes |
| Log → sighting → Follow Scent | Follow reception from that radio identity; Home → Follow returns to it |
| Settings → Atmosphere | Cycle theme, background, speech frequency and compact panel |
| Settings → Tools (page 6) | Set UTC, calibrate battery, clear SD logs, run self-test or export history |

Manual UTC is marked as manual in records and resets after power loss. Battery mode defaults off; voltage and estimated percentage require a valid, explicitly enabled calibration against a meter. Power thresholds still require validation with the actual board and battery.

## Settings pages

Settings always opens on **Hound**, with **Wardrobe, Atmosphere, Scent Book, Ignored scents and Snooze/Resume alerts**. The More button names the next page: **Display → Alerts → Data → Maintenance → Tools**, then back to Hound. Returning from a submenu keeps its parent page; tapping the main SET tab or cycling to Settings with BOOT returns to Hound.

**Settings → More: Display → Display options → Panel inversion** changes the LCD color polarity immediately and saves it on the real device. Keep it **Off** on the tested Hosyond board; enabling it turns the dark background light and changes the dog colors. Its value is restored at boot. Demo inversion is temporary and the real setting is restored when leaving demo. The simulator previews the same color reversal without altering the underlying theme.

## Detector switches and alert switches

| Setting | New observations and logs | Pop-ups and sound |
| --- | --- | --- |
| SCAN → Detectors → category disabled | Stops new detections for that category; existing history remains | Off for that category |
| Settings → Alert types → category off | Continues | Off for that category |
| Ignore one identity | Continues for that identity | Off for that identity/category |
| Snooze | Continues | Temporarily quiet |
| Sound mute | Continues | Visual cards remain; audio is muted |
| Stop sniffing | Pauses reception | No new sightings; hound sleeps |

Turning off a tag's alert category also excludes it from Travel Watch. [Alert and identity behavior](alerts.md) explains the differences.
