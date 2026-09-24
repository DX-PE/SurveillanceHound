<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# SD card, exports and saved state

**Settings → Data → Export history** (also available under Tools) streams validated stored events from the SD card, retaining repeated updates, with a 4,096-file safety bound. It rekeys session IDs and observation identifiers and writes a checksum manifest only for a completed export. The button shows **EXPORTING...**, then **COMPLETE: N RECORDS**, or a visible error. Repeated taps are ignored while the export is pending. Demo reports that no files were saved.

If the storage queue is temporarily busy, settings/progress saves retry with the latest state after one second. **SAVE WAITING / RETRYING** clears when a save is accepted into the queue; that is not a separate confirmation of NVS durability.

**Settings → Data → Eject SD** shows **EJECTING...**, then **SAFE TO REMOVE** (and **SD SAFE** in the top bar) after the worker flushes, syncs and closes the log. A close failure shows **FAILED / POWER OFF**; a busy queue shows **BUSY / TAP TO RETRY**. Safe removal confirms the card is no longer being accessed, not that all earlier writes succeeded. Power off before reinserting; cards mount at startup. Diagnostics page 3 shows **NVS SAVES** and **NVS ERRORS** for completed progress/settings saves in this boot; queue acceptance alone is not proof of a saved state.

For an aggregated historical export, safely eject the card in Settings, connect it to a computer reader, and run:

```sh
python3 tools/export_logs.py /media/CARD/SURVSNIFF/LOGS ./community-export
python3 tools/validate_export.py ./community-export
```

Nothing is uploaded. Raw MACs, SSIDs, payloads and Remote ID serials/coordinates are excluded from sanitized exports. The device collects no GPS location. See [privacy](PRIVACY.md) and [export format](COMMUNITY_EXPORT.md).

Version 1, 2, 3 and 4 saves migrate to version 5, preserving pet progress, privacy settings, recent meals and touch calibration. Old cat selection slots become Husky, Labrador and Dalmatian. Settings, pet, system, companion and appearance state use CRC-protected generation slots in separate NVS namespaces. V4 and V5 commits require the companion record; V5 also requires the appearance record. Interrupted or corrupt state never silently discards ignores or preferences. Older saves start with Midnight / Signal Grid / occasional speech / detailed panel. Appearance record V3 migrates V1/V2 while retaining the V5 generation scheme; it keeps theme/scene/speech/panel choices, defaults inversion off and supplies the 2/5/15-minute idle defaults. Companion V2 migrates the previous 16-entry ignore list into 64 slots. Retain a private pre-upgrade NVS backup before downgrading: older firmware cannot read these newer records. Simulator state is not persisted.

See [ignore backups](IGNORE_STORAGE.md) for the separate authenticated recovery copy and its limits.
