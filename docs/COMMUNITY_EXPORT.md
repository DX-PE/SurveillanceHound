<!-- SPDX-License-Identifier: CC-BY-4.0 -->
# Community export

Records follow `data/schemas/observation.schema.json`; exports follow `data/schemas/export.schema.json`, both version 1. Without a manually set UTC clock, timestamps use uptime and `observed_at: null`. With manual UTC, records explicitly carry `time_quality: manual`. Power loss resets the manual clock. Latitude/longitude remain null and `location_source` is `none`; broadcast Remote ID coordinates stay in volatile on-screen details only.

**Settings → Data → Export history** (also available under Tools) streams every validated JSONL event on the card into `SURVSNIFF/EXPORT/E-<random>/`. It retains repeated updates and marks the summary `full_history_events`. At most 4,096 log files are accepted; exceeding the bound fails explicitly. This export uses bounded RAM, yields to the scheduler, and contains `SIGHTINGS.JSONL`, `SUMMARY.JSON`, `README.TXT`, and `MANIFEST.SHA256`. A failed export has no completion manifest; remove its incomplete folder on a computer. Both session IDs and observation identifiers are rekeyed using a fresh, discarded export key. Only allowlisted fields are retained.

For a complete **aggregated** archive, eject the card and run:

```sh
python3 tools/export_logs.py /media/CARD/SURVSNIFF/LOGS ./export-001
python3 tools/validate_export.py ./export-001
```

The host exporter uses disk-backed SQLite with bounded cache memory. It groups by session, token, category, rule set, radio, pack and region. Cumulative counts use the maximum within an episode and sum separate episodes. It rejects unknown rules, bad field values, oversized records and incomplete tails, then installs the finished export atomically. Firmware historical validation accepts syntactically valid historical rule IDs so an older pack's logs remain readable; field values and structure are checked and unknown extra fields are discarded.

Firmware boot validates every complete row before allowing writes. A malformed complete row puts logging into a read-only state. A bounded incomplete final row can be truncated after preceding rows validate. An unmountable FAT volume stays offline; it is never formatted or repaired automatically. Explicit host repair is also available:

```sh
python3 tools/recover_logs.py /media/CARD/SURVSNIFF/LOGS/S-SESSION-000.JSONL
```

Clear logs requires confirmation and deletes only JSONL observation files. It preserves pet progress and prior exports. A sighting is not map-ready without independently supplied location/context. Nothing is uploaded.
