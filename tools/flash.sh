#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail
cd "$(dirname "$0")/.."
: "${IDF_PATH:?Activate ESP-IDF 6.0.2 first (see docs/BUILDING.md)}"
python3 tools/compile_signatures.py --check
python3 tools/pack_assets.py --check
idf.py -p "${1:?Usage: tools/flash.sh /dev/ttyUSB0}" build flash monitor
