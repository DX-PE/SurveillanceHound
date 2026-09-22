<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Original assets

Creator: Surveillance Hound contributors, 2026.
License: [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).

`pets/characters.json` and `tools/pack_assets.py` define six original geometric pixel pets. The generated header holds 150 64x64 indexed RLE frames with six colors (transparent index 0; the format reserves four-bit indices). No vendor logos, third-party sprites or bitmap fonts are used.

Animation frame ranges: idle 0-3; walk 4-7; sniff 8-10; alert 11-12; eat 13-16; sleep 17-19; hungry 20-21; happy 22-24. Current UI triggers the first seven ranges; remaining happy/level-up behavior is listed in STATUS.md.

The first implementation draws an original dolphin, pineapple and generic biscuit in UI geometry. Other category-specific food silhouettes are still pending. Audio is a project-owned brief low-amplitude step envelope, not a copied sound effect.

The Hound redesign uses six original canine silhouettes and breed markings with colored patrol bandanas. The broad neon/mascot presentation was informed by an external public UI reference requested by the user; no reference sprites or drawing code were imported.

Each dog also has four original side-walk and four side-eating frames. Dolphin and pineapple treats use project-owned pixel geometry, with progressive bites and crumbs.
