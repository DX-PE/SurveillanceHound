#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Project-owned pixel geometry -> indexed RLE assets. No third-party art or fonts."""

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STATES = [
    ("IDLE", 4),
    ("WALK", 4),
    ("SNIFF", 3),
    ("ALERT", 2),
    ("EAT", 4),
    ("SLEEP", 3),
    ("HUNGRY", 2),
    ("HAPPY", 3),
    ("SIDE_WALK", 4),
    ("SIDE_EAT", 4),
]


def with_rim(pixels):
    result = pixels.copy()
    for y in range(64):
        for x in range(64):
            if pixels[y * 64 + x]:
                continue
            if any(
                0 <= x + dx < 64 and 0 <= y + dy < 64 and pixels[(y + dy) * 64 + x + dx]
                for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1))
            ):
                result[y * 64 + x] = 7
    return result


def rgb565(value):
    r, g, b = (int(value[i : i + 2], 16) for i in (0, 2, 4))
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def sprite(character, state, frame):
    """Original 64px hounds: heavy outlines, breed markings and patrol bandanas."""
    p = [0] * 4096
    bounce = int(frame % 2 == 1 and state not in ("SLEEP", "HUNGRY"))

    def rect(x, y, w, h, color):
        for yy in range(max(0, y), min(64, y + h)):
            for xx in range(max(0, x), min(64, x + w)):
                p[yy * 64 + xx] = color

    def oval(x, y, rx, ry, color):
        for yy in range(max(0, y - ry), min(64, y + ry + 1)):
            for xx in range(max(0, x - rx), min(64, x + rx + 1)):
                if (xx - x) ** 2 * ry**2 + (yy - y) ** 2 * rx**2 <= rx**2 * ry**2:
                    p[yy * 64 + xx] = color

    def outlined(x, y, rx, ry, color):
        oval(x, y, rx, ry, 3)
        oval(x, y, rx - 2, ry - 2, color)

    def ear(x, top):
        for row in range(19):
            half = row // 3
            rect(x - half, top + row, half * 2 + 1, 1, 3)
            if row > 5:
                rect(x - half + 2, top + row, max(1, half * 2 - 3), 1, 1)
                if row < 15:
                    rect(x - 1, top + row, 3, 1, 4)

    breed = character["name"]
    if state in ("SIDE_WALK", "SIDE_EAT"):
        # Original right-facing profile: four paws, long muzzle and visible jaw.
        walking = state == "SIDE_WALK"
        stride = (0, 2, 0, -2)[frame] if walking else 0
        bob = frame % 2 if walking else 0
        jaw = (0, 3, 1, 3)[frame] if not walking else 0
        head_y = 28 + (3 if not walking else -bob)
        if character["tail"] in (2, 3):
            outlined(9, 33 - bob, 8, 8, 1)
            oval(9, 32 - bob, 3, 3, 0)
        else:
            outlined(7, 36 - frame % 2, 4, 10 if character["tail"] else 5, 1)
            if breed == "BEAGLE":
                oval(7, 28 - frame % 2, 2, 3, 2)
        # Far legs, then torso, then near legs create a clear walking gait.
        for x in (18 - stride, 39 + stride):
            rect(x, 42, 5, 16, 3)
            rect(x + 1, 43, 3, 13, 1)
        outlined(26, 40 - bob, 21, 12, 1)
        oval(29, 44 - bob, 15, 6, 2)
        if breed == "BEAGLE":
            oval(21, 34 - bob, 12, 5, 3)
        if breed == "DALMATIAN":
            oval(17, 37 - bob, 5, 4, 3)
            oval(29, 34 - bob, 4, 3, 3)
        for x in (15 + stride, 37 - stride):
            rect(x, 44 - bob, 7, 13 + bob, 3)
            rect(x + 2, 44 - bob, 4, 12 + bob, 1)
            outlined(x + 4, 57, 6, 4, 1)
        if character["ears"] == "point":
            ear(38, head_y - 22)
        outlined(43, head_y, 13, 14, 1)
        if breed == "HUSKY":
            oval(48, head_y + 1, 7, 10, 2)
        if breed == "DALMATIAN":
            oval(39, head_y - 5, 7, 6, 3)
        # The lower jaw moves separately beneath the upper snout.
        outlined(52, head_y + 11 + jaw, 10, 4, 1)
        if jaw:
            rect(48, head_y + 9, 12, jaw + 2, 3)
            rect(53, head_y + 11 + jaw, 6, 2, 4)
        outlined(52, head_y + 6, 10, 5, 2)
        oval(60, head_y + 4, 3, 3, 3)
        rect(58, head_y + 2, 3, 1, 2)
        rect(47, head_y - 5, 5, 6, 3)
        rect(48, head_y - 5, 2, 2, 2)
        rect(46, head_y - 9, 6, 2, 3)
        if character["ears"] == "flop":
            outlined(35, head_y, 6, 12, 1 if breed == "LABRADOR" else 3)
        else:
            ear(35, head_y - 21)
        rect(36, 39 - bob, 10, 4, 3)
        rect(37, 40 - bob, 8, 3, 6)
        for row in range(7):
            rect(37, 43 + row - bob, 8 - row, 1, 6)
        rect(38, 44 - bob, 2, 2, 5)
        return p
    sad = state == "HUNGRY"
    y = 27 - bounce + (3 if state == "SNIFF" else 2 + frame if sad else 0)
    wag = 0 if sad else frame % 3 - 1
    # A substantial body, paws, wagging tail and ears keep every silhouette canine.
    if character["tail"] in (2, 3):
        outlined(51, 38 + wag, 9, 9, 1)
        oval(50, 36 + wag, 4, 4, 0)
        rect(45, 40, 7, 10, 1)
    else:
        outlined(52, 39 + wag * 2, 5, 10 if character["tail"] else 5, 1)
        if breed == "BEAGLE":
            oval(52, 32 + wag * 2, 3, 4, 2)
    outlined(32, 44 - bounce, 17, 14, 1)
    oval(32, 46 - bounce, 10, 10, 2)
    step = 2 if state == "WALK" and frame % 2 else 0
    outlined(21, 55 - step, 8, 6, 1)
    outlined(42, 55 - (2 - step if state == "WALK" else 0), 8, 6, 1)
    rect(18, 55 - step, 1, 3, 3)
    rect(23, 55 - step, 1, 3, 3)
    rect(39, 55, 1, 3, 3)
    rect(44, 55, 1, 3, 3)
    if character["ears"] == "flop":
        outlined(14, y + 1, 7, 15, 1 if breed == "LABRADOR" else 3)
        outlined(49, y + 1, 7, 15, 1 if breed == "LABRADOR" else 3)
    else:
        ear(17, 2 - bounce)
        ear(46, 2 - bounce)
    outlined(32, y, 21, 18, 1)
    if breed == "HUSKY":
        oval(23, y - 2, 8, 11, 2)
        oval(40, y - 2, 8, 11, 2)
        rect(30, y - 13, 4, 12, 1)
    elif breed in ("CORGI", "BEAGLE"):
        rect(29, y - 14, 6, 20, 2)
    elif breed == "DALMATIAN":
        oval(21, y - 3, 8, 10, 3)
        oval(43, y - 12, 5, 3, 3)
        oval(43, 44, 4, 4, 3)
        oval(19, 50, 4, 3, 3)
    oval(32, y + 9, 13, 8, 2)
    closed = state == "SLEEP" or state == "IDLE" and frame == 3
    for x in (22, 40):
        if sad:
            # Heavy eyelids and falling tears; shared by hunger and low spirits.
            rect(x - 4, y - 2, 8, 2, 3)
            rect(x - 2, y, 5, 2, 3)
            oval(x, y + 7 + frame * 4, 2, 4, 5)
            rect(x - 1, y + 4 + frame * 4, 1, 2, 2)
        elif closed:
            rect(x - 3, y, 7, 2, 3)
        else:
            rect(x - 3, y - 4, 7, 8, 3)
            rect(x - 2, y - 4, 2, 3, 2)
            if breed == "DALMATIAN" and x == 22:
                rect(x - 3, y - 8, 7, 2, 2)
            else:
                rect(x - 4, y - 8, 8, 2, 3)
    oval(32, y + 6, 5, 3, 3)
    rect(30, y + 4, 3, 1, 2)
    rect(31, y + 8, 2, 4, 3)
    if sad:
        rect(28, y + 11, 8, 2, 3)
        rect(25, y + 13, 4, 2, 3)
        rect(36, y + 13, 4, 2, 3)
    else:
        rect(26, y + 12, 12, 2, 3)
    if state in ("EAT", "HAPPY", "WALK"):
        oval(33, y + 14, 4, 3 + frame % 2, 3)
        rect(31, y + 14, 4, 4, 4)
    # Each hound wears its own bright patrol bandana and small badge.
    rect(19, 41 - bounce, 27, 4, 3)
    rect(21, 42 - bounce, 23, 3, 6)
    for row in range(7):
        rect(27 + row // 2, 45 + row - bounce, 11 - row, 1, 6)
    rect(32, 46 - bounce, 3, 3, 5)
    if state in ("ALERT", "HAPPY"):
        rect(57, 5, 3, 9, 5)
        rect(57, 17, 3, 3, 5)
    if state == "SNIFF":
        rect(54 + frame, y + 7, 2, 2, 5)
        rect(57, y + 3 - frame, 2, 2, 5)
    if state == "SLEEP":
        rect(54, 10, 6, 1, 5)
        rect(57, 12, 2, 2, 5)
        rect(54, 15, 6, 1, 5)
    return p


def encode(pixels):
    out = []
    prev = pixels[0]
    count = 0
    for p in pixels:
        if p == prev and count < 255:
            count += 1
        else:
            out.extend([count, prev])
            prev = p
            count = 1
    out.extend([count, prev])
    return out


def generate(check=False):
    chars = json.loads((ROOT / "assets/pets/characters.json").read_text())["characters"]
    data = []
    desc = []
    previews = []
    for c in chars:
        frames = []
        for state, count in STATES:
            for f in range(count):
                pixels = with_rim(sprite(c, state, f))
                packed = encode(pixels)
                frames.append((len(data), len(packed)))
                data += packed
                if state == "IDLE" and f == 0:
                    previews.append(pixels)
        desc.append(frames)
    text = [
        "// SPDX-License-Identifier: CC-BY-4.0",
        "// Original Surveillance Hound art; generated, do not edit.",
        "#pragma once",
        "#include <cstdint>",
        "namespace sniffer::assets {",
        "struct Frame { uint32_t offset; uint16_t length; };",
        "inline constexpr const char* names[] = {"
        + ",".join(json.dumps(c["name"]) for c in chars)
        + "};",
        "inline constexpr uint16_t palettes[6][8] = {",
    ]
    for c in chars:
        text.append(
            "    {"
            + ",".join(
                str(rgb565(v))
                for v in [
                    "000000",
                    c["body"],
                    c["patch"],
                    "191424",
                    "FF8B9A",
                    "78ECE3",
                    c["accent"],
                    "9AA4B5",  # Soft rim light separates black ears from dark backgrounds.
                ]
            )
            + "},"
        )
    frame_count = sum(count for _, count in STATES)
    text += [
        "};",
        f"inline constexpr int frame_count = {frame_count};",
        "inline constexpr int side_walk = 25, side_eat = 29;",
        f"inline constexpr Frame frames[6][{frame_count}] = {{",
    ]
    for row in desc:
        text.append("    {" + ",".join("{%d,%d}" % f for f in row) + "},")
    text += ["};", "inline constexpr uint8_t pixels[] = {"]
    for i in range(0, len(data), 40):
        text.append("    " + ",".join(map(str, data[i : i + 40])) + ",")
    text += ["};", "}", ""]
    path = ROOT / "generated/assets_generated.h"
    content = "\n".join(text)
    if check:
        if not path.exists() or path.read_text() != content:
            raise ValueError("Stale generated assets; run tools/pack_assets.py")
    else:
        path.write_text(content)
    print(f"Assets validated: 6 pets, {frame_count} frames each, {len(data)} RLE bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    generate(parser.parse_args().check)
