#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Small project invariants. Complements compiler warnings and sanitizers; not an audit."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
BANNED = (
    "esp_wifi_connect",
    "esp_wifi_scan_start",
    "esp_wifi_80211_tx",
    "ble_gap_connect",
    "ble_gap_adv_start",
    "esp_http_client_init",
    "esp_https_ota",
)
for folder in ("main", "components"):
    for path in (ROOT / folder).rglob("*"):
        if path.suffix not in (".cpp", ".hpp", ".h", ".c"):
            continue
        text = path.read_text()
        assert "SPDX-License-Identifier:" in text, path
        for api in BANNED:
            assert not re.search(r"\b" + api + r"\s*\(", text), (path, api)
radio = (ROOT / "components/radio/radio.cpp").read_text()
assert re.search(r"params\.passive\s*=\s*1", radio)
assert "WIFI_MODE_NULL" in radio
assert "xQueueSend(queue, &raw, 0)" in radio
for source in ("wifi_callback", "gap_event"):
    body = radio.split(f"{source}(", 1)[1].split("\n}", 1)[0]
    for forbidden in ("new ", "malloc(", "fwrite(", "parse_wifi(", "parse_ble("):
        assert forbidden not in body, (source, forbidden)
print("Static invariants passed: passive APIs, bounded callbacks, SPDX headers")
