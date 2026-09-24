<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Your hound and wardrobe

Every five powered-on minutes without an earned meal lowers fullness by **3** and mood by **2**, down to zero. A meal restarts that timer. Power-off time is not counted; stopping sniffing shows the sleeping hound while needs continue to decay. At **25 or below** in either stat, the hound has droopy eyes, animated tears and a slow head movement instead of its usual patrol. Low bars turn amber.

Meals add fullness/mood by confidence: **20/15** for high, **14/12** for medium and **6/6** for enabled low-confidence feeding. An earned meal lifts critical needs to at least **35 fullness / 40 mood**, capped at 100, so one snack can cheer the hound up. The existing identity/category/global meal cooldowns and XP rules still apply; an ordinary repeated alert does not feed the dog. Petting adds 2 mood without filling its belly.

The eating sequence ends in happy hops for about four seconds. Reduced animation uses still sad/happy poses without jumping; Stop keeps the sleeping Zs. Demo hunger and mood are separate from saved progress. In the local lab, use **Care for your hound → Hungry hound / Low mood**, then send a Flipper or Pineapple snack. **Wait 30 minutes** advances the simulator clock and applies real decay rules without changing hardware settings.

## Scent Book and outfits

Scent Book counts emitted observations, including repeat updates, rather than unique devices. First/last times are manual UTC when available; unset times remain unknown. Statistics start when this feature is installed or progress is reset. Existing saves retain XP and earn eligible XP outfits, without inventing historical sightings. Evidence pages use the latest 32 RAM log entries; they may be empty after reboot even when totals remain saved.

| Outfit | Unlock |
|---|---|
| Classic | Available immediately |
| Trail bandana | 25 XP |
| Brass collar | 100 XP |
| Raincoat | Discover 5 categories |
| Detective cap | 300 XP |
| Sunglasses | Discover 10 categories |


Outfit unlocks stay earned until **Reset progress**, which also clears the Scent Book and equipped outfit. Reset retains observation logs and ignored devices. Each dog has its own saved equipped accessory. Demo outfits and progress are temporary.

Choose Corgi, Beagle, Shiba, Husky, Labrador or Dalmatian. Dark ears and tails keep a light outline in dark scenes and on the saver. All characters and food sprites are [original project artwork](assets.md).
