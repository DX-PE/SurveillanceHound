#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Package local build artifacts and a source SPDX inventory; never tags or publishes."""

import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import uuid
import tempfile
from audit_binary import audit
from compile_signatures import ROOT, generate
from pack_assets import generate as assets

SDK_COMMIT = "7101770dc6db2667b3c477cc31365dd1acd6db4e"


def package(build, output, development=False):
    generate(check=True)
    assets(check=True)
    if not development:
        report = ROOT / "docs/release-acceptance.json"
        if (
            not report.exists()
            or json.loads(report.read_text()).get("release_approved") is not True
        ):
            raise ValueError(
                "Release gates remain open; only --development packaging is available"
            )
    output = Path(output)
    output.mkdir(parents=True, exist_ok=False)
    build = Path(build)
    paths = {
        "surveillance_hound.bin": build / "surveillance_hound.bin",
        "bootloader.bin": build / "bootloader/bootloader.bin",
        "partition-table.bin": build / "partition_table/partition-table.bin",
    }
    for name, path in paths.items():
        shutil.copy2(path, output / name)
    (output / "FLASH.txt").write_text(
        "esptool --chip esp32 --port PORT write-flash --flash-mode dio --flash-size 4MB "
        "0x1000 bootloader.bin 0x8000 partition-table.bin 0x10000 surveillance_hound.bin\n"
    )
    for notice in ("LICENSE", "NOTICE", "THIRD_PARTY_LICENSES.md"):
        shutil.copy2(ROOT / notice, output / notice)
    with tempfile.TemporaryDirectory(prefix="hound-binary-audit-") as temp:
        evidence = Path(temp) / "evidence"
        audit(build, evidence)
        shutil.make_archive(str(output / "BINARY-EVIDENCE"), "zip", evidence)
    source = []
    for folder in (
        "main",
        "components",
        "tools",
        "tests",
        "data",
        "assets",
        "generated",
    ):
        for path in sorted((ROOT / folder).rglob("*")):
            if not path.is_file() or "__pycache__" in path.parts:
                continue
            data = path.read_bytes()
            relative = path.relative_to(ROOT).as_posix()
            license = (
                "CC-BY-4.0"
                if folder == "assets" or path.name == "assets_generated.h"
                else "CC0-1.0"
                if folder == "data"
                and "schemas" not in path.parts
                or path.name.startswith("signatures")
                else "Apache-2.0"
            )
            source.append(
                dict(
                    SPDXID="SPDXRef-File-"
                    + hashlib.sha256(relative.encode()).hexdigest()[:16],
                    fileName="./" + relative,
                    checksums=[
                        dict(
                            algorithm="SHA256",
                            checksumValue=hashlib.sha256(data).hexdigest(),
                        )
                    ],
                    licenseConcluded=license,
                    licenseInfoInFiles=[license],
                    copyrightText="NOASSERTION",
                )
            )
    epoch = int(os.environ.get("SOURCE_DATE_EPOCH", "0"))
    created = datetime.datetime.fromtimestamp(epoch, datetime.timezone.utc).strftime(
        "%Y-%m-%dT%H:%M:%SZ"
    )
    identity = hashlib.sha256(json.dumps(source, sort_keys=True).encode()).hexdigest()
    sbom = dict(
        spdxVersion="SPDX-2.3",
        dataLicense="CC0-1.0",
        SPDXID="SPDXRef-DOCUMENT",
        name="Surveillance Hound development source inventory",
        documentNamespace="https://surveillance-hound.invalid/spdx/"
        + str(uuid.uuid5(uuid.NAMESPACE_URL, identity)),
        creationInfo=dict(
            created=created, creators=["Tool: Surveillance-Hound-make_release-1"]
        ),
        documentComment="Source inventory and pinned SDK reference. BINARY-EVIDENCE.zip records linked archives, declared source licenses and SDK notices. Final compatibility review remains a release gate.",
        files=source,
        packages=[
            dict(
                SPDXID="SPDXRef-ESP-IDF",
                name="ESP-IDF",
                versionInfo="6.0.2",
                downloadLocation="git+https://github.com/espressif/esp-idf.git@"
                + SDK_COMMIT,
                filesAnalyzed=False,
                licenseConcluded="NOASSERTION",
                licenseDeclared="Apache-2.0",
                copyrightText="NOASSERTION",
            )
        ],
        relationships=[
            dict(
                spdxElementId="SPDXRef-DOCUMENT",
                relationshipType="DESCRIBES",
                relatedSpdxElement="SPDXRef-ESP-IDF",
            )
        ],
    )
    (output / "SBOM.spdx.json").write_text(json.dumps(sbom, indent=2) + "\n")
    with (output / "SHA256SUMS").open("w") as sums:
        for path in sorted(output.iterdir()):
            if path.name != "SHA256SUMS":
                sums.write(
                    hashlib.sha256(path.read_bytes()).hexdigest()
                    + "  "
                    + path.name
                    + "\n"
                )
    return output


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--development", action="store_true")
    args = parser.parse_args()
    print(package(args.build, args.output, args.development))
