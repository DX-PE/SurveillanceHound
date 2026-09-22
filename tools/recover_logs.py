#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Validate all complete rows, then remove only a partial final row; no formatting."""

import argparse
import json
import os
from pathlib import Path


def recover(path):
    path = Path(path)
    good = 0
    with path.open("rb+") as stream:
        while True:
            line = stream.readline(4097)
            if not line:
                break
            if len(line) > 4096:
                raise ValueError("Oversized record; refusing repair")
            if not line.endswith(b"\n"):
                stream.truncate(good)
                stream.flush()
                os.fsync(stream.fileno())
                return len(line)
            if not isinstance(json.loads(line), dict):
                raise ValueError("Invalid complete record; refusing repair")
            good = stream.tell()
    return 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file", type=Path)
    print(f"Removed {recover(parser.parse_args().file)} partial-tail bytes")
