<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Fixtures

Radio fixtures are built programmatically in tests/host/core_tests.cpp. No real radio addresses, SSIDs, identifiers or captures are shipped. BLE payloads and Wi-Fi frames are independently generated; the HMAC vector comes from RFC 4231. Experimental family marker strings are test-only and never compiled into the production pack.
