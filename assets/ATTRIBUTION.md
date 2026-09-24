<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Original assets

Copyright 2026 Jascha Wanger ([https://dx.pe](https://dx.pe)).
Credit: Surveillance Hound contributors.
License: [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).

`pets/characters.json` and `tools/pack_assets.py` define six original geometric pixel pets. The generated header holds 198 64×64 indexed RLE frames (33 per dog) with an eight-entry palette, including the dark-scene rim (transparent index 0; the format reserves four-bit indices). No vendor logos, third-party sprites or bitmap fonts are used.

Animation frame ranges: idle 0-3; walk 4-7; sniff 8-10; alert 11-12; eat 13-16; sleep 17-19; hungry 20-21; happy 22-24. Side-walk uses frames 25–28 and side-eat uses 29–32. Current UI includes happy/level-up and meal-recovery poses.

The UI draws original food silhouettes for all 19 categories, including a dolphin for Flipper and pineapple for Pineapple. Audio is a project-owned brief low-amplitude step envelope, not a copied sound effect.

The Hound redesign uses six original canine silhouettes and breed markings with colored patrol bandanas. The broad neon/mascot presentation was informed by an external public UI reference requested by the user; no reference sprites or drawing code were imported.

Each dog also has four original side-walk and four side-eating frames. Dolphin and pineapple treats use project-owned pixel geometry, with progressive bites and crumbs.
