#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Build the synthetic Hound lab for static hosting with pinned Emscripten."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
EMSCRIPTEN_VERSION = "6.0.10"
EMSCRIPTEN_IMAGE = "emscripten/emsdk:6.0.10@sha256:e077d54e2b8970575ebc4f185ac1de0b95c05f2b266134d4ba27449af7aebf65"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build-web")
    parser.add_argument("--output", type=Path, default=ROOT / "docs/lab-app")
    parser.add_argument(
        "--docker", action="store_true", help="Use the pinned compiler container"
    )
    args = parser.parse_args()
    if args.docker:
        try:
            build = args.build_dir.resolve().relative_to(ROOT)
            output = args.output.resolve().relative_to(ROOT)
        except ValueError:
            parser.error("Docker build and output paths must be inside the checkout")
        subprocess.run(
            [
                "docker",
                "run",
                "--rm",
                "--network=none",
                "--user",
                f"{os.getuid()}:{os.getgid()}",
                "-e",
                f"EM_CACHE=/src/{build}/emscripten-cache",
                "-v",
                f"{ROOT}:/src",
                "-w",
                "/src",
                EMSCRIPTEN_IMAGE,
                "python3",
                "tools/build_web.py",
                "--build-dir",
                f"/src/{build}",
                "--output",
                f"/src/{output}",
            ],
            check=True,
        )
        return
    compiler = shutil.which("em++")
    if not compiler:
        parser.error(
            f"Activate Emscripten {EMSCRIPTEN_VERSION}; see docs/documentation.md"
        )
    version = subprocess.check_output([compiler, "--version"], text=True).splitlines()[
        0
    ]
    if f" {EMSCRIPTEN_VERSION} " not in version:
        parser.error(f"Expected Emscripten {EMSCRIPTEN_VERSION}, found {version}")
    build = args.build_dir.resolve()
    output = args.output.resolve()
    build.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    sources = [
        "tools/simulator.cpp",
        "tools/simulator_browser.cpp",
        "tools/simulator_crypto.cpp",
        "components/ui/ui.cpp",
        "components/sniffer_core/parsers.cpp",
        "components/sniffer_core/json.cpp",
        "components/sniffer_core/utilities.cpp",
        "components/sniffer_core/engine.cpp",
        "components/sniffer_core/privacy.cpp",
        "components/sniffer_core/records.cpp",
    ]
    includes = [
        "tools",
        "components/ui/include",
        "components/sniffer_core/include",
        "generated",
    ]
    subprocess.run(
        [
            compiler,
            "-std=c++20",
            "-O2",
            "-Wall",
            "-Wextra",
            "-Werror",
            *[str(ROOT / source) for source in sources],
            *[f"-I{ROOT / folder}" for folder in includes],
            "--no-entry",
            "-sMODULARIZE=1",
            "-sEXPORT_ES6=1",
            "-sEXPORT_NAME=createHound",
            "-sEMIT_EMSCRIPTEN_LICENSE=1",
            "-sDYNAMIC_EXECUTION=0",
            "-sENVIRONMENT=web,worker,node",
            "-sMIN_NODE_VERSION=200000",
            "-sASYNCIFY=1",
            "-sASYNCIFY_STACK_SIZE=65536",
            "-sSTACK_SIZE=262144",
            "-sINITIAL_MEMORY=16777216",
            "-sALLOW_MEMORY_GROWTH=1",
            "-sMAXIMUM_MEMORY=67108864",
            "-sEXPORTED_RUNTIME_METHODS=ccall,UTF8ToString,HEAPU8",
            "-o",
            str(build / "hound.mjs"),
        ],
        check=True,
        cwd=ROOT,
    )
    for name in ["hound.mjs", "hound.wasm"]:
        shutil.copy2(build / name, output / name)
    sdk = Path(compiler).resolve().parent
    notices = ["Copyright 2026 Jascha Wanger (https://dx.pe).\n"]
    for source in [
        ROOT / "LICENSE",
        ROOT / "NOTICE",
        sdk / "LICENSE",
        sdk / "system/lib/libc/musl/COPYRIGHT",
        sdk / "system/lib/libcxx/LICENSE.TXT",
        sdk / "system/lib/libcxxabi/LICENSE.TXT",
        sdk / "system/lib/compiler-rt/LICENSE.TXT",
        sdk / "system/lib/libunwind/LICENSE.TXT",
    ]:
        notices.append(
            f"\n--- {source.relative_to(ROOT) if source.is_relative_to(ROOT) else source.relative_to(sdk)} ---\n\n{source.read_text()}"
        )
    (output / "LICENSES.txt").write_text("\n".join(notices))
    for name in ["style.css", "app.js", "transport.js", "browser-worker.js"]:
        shutil.copy2(ROOT / "tools/simulator" / name, output / name)
    html = (ROOT / "tools/simulator/index.html").read_text()
    html = html.replace('data-runtime="native"', 'data-runtime="browser"')
    html = html.replace("· Local lab", "· Browser lab").replace(
        "<h1>Local lab</h1>", "<h1>Browser lab</h1>"
    )
    html = html.replace(
        "<span>Good dog. Sharp nose.</span>",
        '<a href="../lab/" target="_top">Back to the guide</a>',
    )
    html = html.replace(
        "<span>0.1.0 / FIELD LAB</span>",
        '<a href="./LICENSES.txt" target="_blank" rel="noopener">Copyright 2026 Jascha Wanger · Licenses</a>',
    )
    (output / "index.html").write_text(html)
    print(f"Browser lab ready in {output}")


if __name__ == "__main__":
    main()
