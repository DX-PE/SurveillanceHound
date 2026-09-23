<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Ignore storage and SD backups

Hound has **64 saved device/category identities**, shared across detector categories. Settings → Ignored scents shows used/free capacity and four occupied entries per page; tap an entry to allow its alerts again. Full lists reject additions without evicting existing ignores. The upgrade retains all earlier entries, discoveries and outfits.

An entry follows the supported Samsung broadcast ID or, for other detections, its radio address/type/category. It has no timer-based expiry. Changed identifiers can still alert, including AirTag/Find My address rotation. More slots do not solve that continuity limitation.

The alert card and Ignore menu display a short board-local ID derived from the existing keyed identity. The menu indicates when that current identity is already ignored. The selected target is captured from the last drawn Home card, so a radio event processed immediately before touch cannot silently change the target. Apple/Find My sightings also have an identity/status page under Log → More. Counts include ignored signals; a steady count does not establish that a returning alert has the same identity. The short code can collide and is only a comparison aid; matching uses the complete 64-bit keyed value.


## Saving on the card

Internal NVS remains the live, persistent list, so Ignore works without a card. After an internal save succeeds, the storage worker backs up a changed list to `/SURVSNIFF/IGNORES/<board-id>-0.BIN` or `-1.BIN`. It also checks the backup after a normal startup. Unchanged lists do not repeatedly write the card. Deleting the final ignore creates an empty-list backup.

**Ignored scents → SD backup** shows the last result and backed-up entry count; **Back up ignores now** requests a fresh check. Success follows complete writes, flush, fsync and close. Missing, safely ejected, read-only and failed cards have distinct status messages; they do not remove internal ignores. Normal Eject SD serializes behind any backup and prevents later card access. Reinsert/restart using the existing safe-eject procedure. Demo uses only its separate RAM list and does not back up synthetic identities.

Startup mounts the card, then validates historical records in small batches. **SD CHECK** and the record/file counts distinguish this from ready storage. NVS saves are processed between batches; a deferred SD backup reuses one of the two existing state snapshots, replacing it with newer successful saves as needed. Eject closes the history reader before reporting its result. New radio records are not queued during the startup check and count as dropped, preserving capacity for Save/Eject; the current-session log opens after successful validation. Corrupt complete rows remain untouched and make the card read-only. An incomplete trailing row is truncated only after all preceding rows validate and no read-only error has occurred.

A manual backup that encounters a full queue retains its request and retries using the current list. The UI shows **BACKUP PENDING / AUTO RETRY** or **BACKUP QUEUED**, then the actual completion result. Further taps while pending do not fill the queue. Storage-task creation failure has an explicit failure message rather than indefinite waiting. Free-space statistics are first refreshed after logging/flush, rather than delaying the initial backup.

Two alternating files retain the last good generation while a newer file is written. A damaged newest file falls back to the other authenticated file. If both existing files are invalid, backup reports failure and leaves them untouched. This protects against a torn file but cannot guarantee recovery from physical card/filesystem damage.

## Format and recovery boundary

Each file is 624 bytes: magic `HNDIGN02` (8), nonzero little-endian generation (8), 64 entries of eight little-endian digest bytes plus one category byte (576), and HMAC-SHA256 of all preceding bytes (32). Empty entries have a zero digest; categories and duplicate nonzero digests are validated. There are no raw MAC addresses, broadcast IDs or secret keys in these files. The 16-character filename ID is a domain-separated HMAC prefix derived from the existing board identity key, keeping different boards' backups separate.

Authentication binds the backup to that key. **Normal boot never restores SD state over NVS**, which would resurrect removed ignores from an older card. An on-device restore control is not implemented. The production `read_ignore_backup` routine validates both generations and returns the newest valid list for development recovery using the original key from this board's internal/private NVS backup. A factory reset or new board key cannot use an old backup. Retain private NVS backups separately; never put the secret identity key on the public card or in source control.

Companion V2 stores each ignore in nine aligned-safe bytes, increasing the five existing state/UI copies by 1,600 static bytes in total. SD serialization uses bounded temporary buffers and no dynamically growing lists. Field heap/stack and file-recovery acceptance remains part of hardware testing.
