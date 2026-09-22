#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Inventory actually linked archives, compiled source SPDX tags and SDK notices.

Records evidence for review; never infers a legal conclusion from an umbrella license.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def audit(build, output):
    build, output = Path(build).resolve(), Path(output)
    info = json.loads((build / "project_description.json").read_text())
    sdk = Path(info["idf_path"]).resolve()
    project = Path(info["project_path"]).resolve()
    link_map = build / (info["project_name"] + ".map")
    text = link_map.read_text().split("Discarded input sections", 1)[0]
    linked = {}
    for archive, member in re.findall(r"([^\s()]+\.a)\(([^)]+)\)", text):
        path = Path(archive)
        if not path.is_absolute():
            path = build / path
        linked.setdefault(path.resolve(), set()).add(member)
    if not linked:
        raise ValueError("No linked archive evidence in map")
    output.mkdir(parents=True, exist_ok=False)
    notices = output / "notices"
    notices.mkdir()
    copied = {}

    def display(path):
        for root, prefix in [
            (sdk, "ESP-IDF/"),
            (project, "PROJECT/"),
            (build, "BUILD/"),
        ]:
            if path.is_relative_to(root):
                return prefix + path.relative_to(root).as_posix()
        return "TOOLCHAIN/" + path.name

    def retain(path):
        if path not in copied:
            digest = sha(path)
            name = digest[:16] + "-" + path.name
            shutil.copy2(path, notices / name)
            copied[path] = dict(
                source=display(path), file="notices/" + name, sha256=digest
            )
        return copied[path]["file"]

    packages = []
    for archive, members in sorted(linked.items()):
        component = next(
            (
                c
                for c in info["build_component_info"].values()
                if c.get("file") and Path(c["file"]).resolve() == archive
            ),
            None,
        )
        if component is None and archive.is_relative_to(build / "esp-idf"):
            name = archive.relative_to(build / "esp-idf").parts[0]
            component = info["build_component_info"].get(name)
        sources, licenses = [], set()
        if component:
            for filename in component.get("sources", []):
                path = Path(filename)
                if not path.is_file():
                    continue
                tags = re.findall(
                    r"SPDX-License-Identifier:\s*([^\n]+)",
                    path.read_text(errors="replace")[:8192],
                )
                tags = [tag.strip().removesuffix("*/").strip() for tag in tags]
                sources.append(
                    dict(
                        path=display(path),
                        sha256=sha(path),
                        spdx=tags or ["NOASSERTION"],
                    )
                )
            base = Path(component["dir"])
        else:
            base = archive.parent
        if base.is_relative_to(sdk):
            # Include component/submodule license files and the SDK umbrella notice.
            for parent in [base, *base.parents]:
                if not parent.is_relative_to(sdk):
                    break
                for path in parent.iterdir():
                    if path.is_file() and path.name.upper().startswith(
                        ("LICENSE", "COPYING", "NOTICE")
                    ):
                        licenses.add(retain(path))
            if component:
                for pattern in ["**/LICENSE*", "**/COPYING*", "**/NOTICE*"]:
                    for path in base.glob(pattern):
                        if path.is_file():
                            licenses.add(retain(path))
        if base.is_relative_to(project):
            for name in ["LICENSE", "NOTICE"]:
                if (project / name).is_file():
                    licenses.add(retain(project / name))
        if archive.name in {"libgcc.a", "libstdc++.a", "libc.a"}:
            compiler_root = Path(info["c_compiler"]).resolve().parent.parent
            family = "newlib" if archive.name == "libc.a" else "gcc"
            license_root = compiler_root / "share/licenses" / family
            for path in license_root.rglob("*"):
                if path.is_file() and path.name.upper().startswith(
                    ("COPYING", "LICENSE")
                ):
                    licenses.add(retain(path))
        packages.append(
            dict(
                archive=display(archive),
                sha256=sha(archive),
                members=sorted(members),
                compiled_sources=sources,
                notice_files=sorted(licenses),
                license_concluded="NOASSERTION",
            )
        )
    report = dict(
        schema_version=1,
        target=info["target"],
        sdk_commit="7101770dc6db2667b3c477cc31365dd1acd6db4e",
        application_sha256=sha(build / info["app_bin"]),
        map_sha256=sha(link_map),
        archives=packages,
        notices=list(copied.values()),
        limitation="Archive/member inclusion and compiled-source declarations are evidence, not a completed compatibility opinion. Proprietary binary and compiler runtime notices need maintainer review; compiled source lists may include dead-stripped functions.",
    )
    (output / "BINARY-INVENTORY.json").write_text(json.dumps(report, indent=2) + "\n")
    print(
        f"Inventoried {len(packages)} linked archives and retained {len(copied)} notice files"
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    audit(args.build, args.output)
