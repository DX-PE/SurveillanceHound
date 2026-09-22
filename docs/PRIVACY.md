<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Privacy

Private mode is the default. Each boot draws a 32-byte HMAC key and an independent random session ID while the ESP32 bootloader entropy source is enabled, before RF initialization. A token is HMAC-SHA256(address + radio + address type), truncated to 12 bytes and base32 encoded. The session key is RAM-only and is never written to the card or exposed in the session ID.

Local private rows contain no raw MAC, SSID, advertised name, manufacturer payload, drone serial or coordinates. Raw frame bytes are held temporarily in fixed RAM queues and normalization buffers. Only matched rule IDs and fixed project labels/reasons enter logs. Strings are encoded by one bounded JSON writer.

Research mode requires an on-device warning and explicit enable action. It adds a raw MAC to local rows and displays `RAW LOG`. It does not enable SSID, serial, coordinate or raw-packet logging. The initial build deliberately has no sensitive-export path.

Default exports use an allowlist. On-device export validates stored historical JSONL and copies only allowlisted fields, rekeying both session IDs and observation tokens for every export. The host exporter reads complete rows, verifies known rule IDs and field types, drops all other fields, and rekeys even syntactically valid identifiers using a fresh discarded key. It cannot export unknown experimental/demo rules. Export filenames and summaries contain no radio identifiers.

Pet deduplication needs persistence across boots. A separate device-local random key derives 64-bit meal hashes; up to 32 recent hashes are saved in NVS. That key and those hashes never appear in exports. Physical access to unencrypted NVS and the SD card is outside the privacy guarantee. Firmware encryption and secure boot are not enabled in this development build.

Demo observations are marked before rendering. They bypass the detector-to-log path, meal guard, XP and count persistence. Entering/exiting demo preserves the real pet state and clears synthetic recent items. Demo data is never accepted by the record writer.

Remote ID Basic/Location/System details are bounded, volatile on-screen snapshots. They are never persisted. Manual UTC is local input, marked as manual, and resets on reboot.

There is no network client, account, telemetry, GPS, upload, or online time service. The SDK includes networking libraries required by its Wi-Fi stack; the application does not initiate network communication.
