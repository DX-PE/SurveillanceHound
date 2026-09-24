#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Export original firmware sprites as deterministic documentation SVGs."""

import argparse
import json
from pathlib import Path

from pack_assets import sprite, with_rim

ROOT = Path(__file__).resolve().parents[1]


def drawing(character, offset):
    pixels = with_rim(sprite(character, "IDLE", 0))
    colors = [
        "000000",
        character["body"],
        character["patch"],
        "191424",
        "FF8B9A",
        "78ECE3",
        character["accent"],
        "9AA4B5",
    ]
    result = [f'<g transform="translate({offset} 0)">']
    for y in range(64):
        x = 0
        while x < 64:
            color = pixels[y * 64 + x]
            end = x + 1
            while end < 64 and pixels[y * 64 + end] == color:
                end += 1
            if color:
                result.append(
                    f'<rect x="{x}" y="{y}" width="{end - x}" height="1" '
                    f'fill="#{colors[color]}"/>'
                )
            x = end
    result.append("</g>")
    return "\n".join(result)


def generate(characters):
    width = len(characters) * 72 - 8
    body = "\n".join(drawing(c, i * 72) for i, c in enumerate(characters))
    return (
        "<!-- SPDX-License-Identifier: CC-BY-4.0 -->\n"
        "<!-- Copyright 2026 Jascha Wanger (https://dx.pe). Original Hound art. -->\n"
        '<svg xmlns="http://www.w3.org/2000/svg" '
        f'viewBox="0 0 {width} 64" shape-rendering="crispEdges">\n'
        f"{body}\n</svg>\n"
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    characters = json.loads((ROOT / "assets/pets/characters.json").read_text())[
        "characters"
    ]
    for name, cast in (("hound.svg", characters[:1]), ("hound-pack.svg", characters)):
        target = ROOT / "docs/images" / name
        content = generate(cast)
        if args.check:
            if not target.is_file() or target.read_text() != content:
                raise SystemExit(
                    f"Stale {target.name}; run python3 tools/docs_assets.py"
                )
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(content)
    print("Documentation sprites match the original firmware geometry.")


if __name__ == "__main__":
    main()
