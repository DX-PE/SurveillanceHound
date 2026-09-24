#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Stage the checksum-pinned release image and manifest for the static installer."""

import argparse
import hashlib
import json
from pathlib import Path
import re
from urllib.request import urlopen

ROOT = Path(__file__).resolve().parents[1]


def release_config():
    release = json.loads((ROOT / "tools/firmware-release.json").read_text())
    filename = release["filename"]
    version = release["version"]
    base = "https://github.com/DX-PE/SurveillanceHound/releases/"
    if (
        release["chip"] != "ESP32"
        or release["offset"] != 0
        or not re.fullmatch(r"[a-zA-Z0-9._-]+\.bin", filename)
        or not re.fullmatch(r"[a-zA-Z0-9.-]+", version)
        or not re.fullmatch(r"[0-9a-f]{64}", release["sha256"])
        or not 65536 < release["size"] <= 4 * 1024 * 1024
        or release["release_url"] != f"{base}tag/v{version}"
        or release["firmware_url"] != f"{base}download/v{version}/{filename}"
    ):
        raise ValueError("Unsupported or inconsistent release configuration")
    return release


def manifest_for(release):
    return {
        "name": release["name"],
        "version": release["version"],
        # A full factory image overwrites NVS even without an erase operation.
        # Do not offer the upstream optional 'preserve data' erase checkbox.
        "new_install_prompt_erase": False,
        "new_install_improv_wait_time": 0,
        "builds": [
            {
                "chipFamily": release["chip"],
                "parts": [{"path": release["filename"], "offset": release["offset"]}],
            }
        ],
    }


def verify_image(data, release):
    if len(data) != release["size"]:
        raise ValueError("Firmware size does not match the pinned release")
    if hashlib.sha256(data).hexdigest() != release["sha256"]:
        raise ValueError("Firmware SHA-256 does not match the pinned release")


def check_output(output, release):
    verify_image((output / release["filename"]).read_bytes(), release)
    if json.loads((output / "manifest.json").read_text()) != manifest_for(release):
        raise ValueError("Installer manifest does not match the pinned release")
    if json.loads((output / "release.json").read_text()) != release:
        raise ValueError("Installer metadata does not match the pinned release")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "docs/firmware")
    parser.add_argument("--firmware-file", type=Path, help="Use a local release asset")
    parser.add_argument(
        "--check", action="store_true", help="Verify without downloading"
    )
    args = parser.parse_args()
    release = release_config()
    output = args.output.resolve()
    try:
        if not args.check:
            if args.firmware_file:
                data = args.firmware_file.read_bytes()
            elif (output / release["filename"]).exists():
                data = (output / release["filename"]).read_bytes()
            else:
                with urlopen(release["firmware_url"], timeout=60) as response:
                    data = response.read(release["size"] + 1)
            # Never replace a usable installation with an unverified download.
            verify_image(data, release)
            output.mkdir(parents=True, exist_ok=True)
            (output / release["filename"]).write_bytes(data)
            (output / "release.json").write_text(json.dumps(release, indent=2) + "\n")
            (output / "manifest.json").write_text(
                json.dumps(manifest_for(release), indent=2) + "\n"
            )
        check_output(output, release)
    except (OSError, ValueError) as error:
        parser.exit(1, f"Flasher build failed: {error}\n")
    print(
        f"Verified {release['version']}: ESP32 factory image, offset 0, SHA-256 matches"
    )


if __name__ == "__main__":
    main()
