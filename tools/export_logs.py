#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Offline, bounded-memory, allowlist export of SD JSONL logs. Never uploads."""

import argparse
import base64
import datetime
import hashlib
import hmac
import json
import os
from pathlib import Path
import re
import secrets
import sqlite3
import tempfile
from compile_signatures import CATEGORIES, validate

MAX_LINE = 4096


def integer(row, key, low, high):
    value = row.get(key)
    if type(value) is not int or not low <= value <= high:
        raise ValueError(f"Invalid {key}")
    return value


def text(row, key, pattern):
    value = row.get(key)
    if not isinstance(value, str) or not re.fullmatch(pattern, value):
        raise ValueError(f"Invalid {key}")
    return value


def sanitize(row, key, catalog):
    if not isinstance(row, dict) or row.get("schema_version") != 1:
        raise ValueError("Unsupported observation schema")
    category = row.get("category")
    if category not in CATEGORIES:
        raise ValueError("Unknown category")
    ids = row.get("rule_ids")
    if (
        not isinstance(ids, list)
        or not 1 <= len(ids) <= 4
        or any(not isinstance(i, str) for i in ids)
    ):
        raise ValueError("Invalid rule IDs")
    if any(
        i not in catalog
        or catalog[i]["experimental"]
        or catalog[i]["category"] != category
        for i in ids
    ):
        raise ValueError(
            "Unverified or mismatched rule IDs; demo records are not exportable"
        )
    session = text(row, "session_id", r"[a-f0-9]{32}")
    token = text(row, "identifier", r"[A-Z2-7]{20}")

    # Rekey even well-formed input identifiers so an SD file cannot smuggle raw IDs in the output.
    def digest(value):
        return hmac.new(key, value.encode("ascii"), hashlib.sha256).digest()

    public_session = digest("session:" + session)[:16].hex()
    public_token = (
        base64.b32encode(digest("device:" + session + ":" + token)[:12])
        .decode()
        .rstrip("=")
    )
    first = integer(row, "first_seen_ms", 0, 2**53 - 1)
    last = integer(row, "uptime_ms", first, 2**53 - 1)
    score = integer(row, "score", 1, 100)
    rmin = integer(row, "rssi_min", -127, 20)
    rmax = integer(row, "rssi_max", rmin, 20)
    seen = integer(row, "seen_count", 1, 2**32 - 1)
    channel = integer(row, "channel", 0, 14)
    radio = row.get("radio")
    if radio not in ("wifi", "ble"):
        raise ValueError("Invalid radio")
    firmware = text(row, "firmware_version", r"[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9.]+)?")
    pack = text(row, "signature_pack", r"v[0-9]+-[a-f0-9]{12}")
    region = row.get("region", "unknown")
    if region not in ("US", "EU", "unknown"):
        raise ValueError("Invalid region")
    privacy = row.get("privacy_mode")
    if privacy not in ("private", "research"):
        raise ValueError("Invalid privacy mode")
    observed = row.get("observed_at")
    quality = row.get("time_quality", "uptime_only")
    if quality == "manual":
        if not isinstance(observed, str) or not re.fullmatch(
            r"20[0-9]{2}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z", observed
        ):
            raise ValueError("Invalid manual timestamp")
        datetime.datetime.strptime(observed, "%Y-%m-%dT%H:%M:%SZ")
    elif quality != "uptime_only" or observed is not None:
        raise ValueError("Invalid time quality")
    return dict(
        observed_at=observed,
        time_quality=quality,
        session_id=public_session,
        identifier=public_token,
        category=category,
        rule_ids=sorted(set(ids)),
        first_seen_ms=first,
        last_seen_ms=last,
        score=score,
        rssi_min=rmin,
        rssi_max=rmax,
        seen_count=seen,
        channel=channel,
        radio=radio,
        firmware_version=firmware,
        signature_pack=pack,
        region=region,
        source_privacy_mode=privacy,
    )


def export(logs, destination):
    logs, destination = Path(logs), Path(destination)
    if destination.exists():
        raise ValueError("Destination already exists; choose a new directory")
    catalog = {r["id"]: r for r in validate()}
    salt = secrets.token_bytes(32)
    destination.parent.mkdir(parents=True, exist_ok=True)
    rows = 0
    with tempfile.TemporaryDirectory(
        prefix=".sniffer-export-", dir=destination.parent
    ) as stage:
        stage = Path(stage)
        db = sqlite3.connect(stage / "aggregate.sqlite")
        db.execute("PRAGMA cache_size=-2048")
        db.execute(
            "CREATE TABLE episodes (identity TEXT, first INTEGER, last INTEGER, seen INTEGER, score INTEGER, rmin INTEGER, rmax INTEGER, row TEXT, PRIMARY KEY(identity,first))"
        )
        files = [logs] if logs.is_file() else sorted(logs.glob("*.JSONL"))
        if not files:
            raise ValueError("No JSONL files found")
        for path in files:
            with path.open("rb") as stream:
                while True:
                    line = stream.readline(MAX_LINE + 1)
                    if not line:
                        break
                    if len(line) > MAX_LINE:
                        raise ValueError(f"{path.name}: oversized record")
                    if not line.endswith(b"\n"):
                        raise ValueError(
                            f"{path.name}: partial tail; run recover_logs.py first"
                        )
                    row = sanitize(json.loads(line), salt, catalog)
                    identity = json.dumps(
                        [
                            row[k]
                            for k in (
                                "session_id",
                                "identifier",
                                "category",
                                "rule_ids",
                                "radio",
                                "signature_pack",
                                "region",
                            )
                        ],
                        separators=(",", ":"),
                    )
                    db.execute(
                        """INSERT INTO episodes VALUES(?,?,?,?,?,?,?,?) ON CONFLICT(identity,first) DO UPDATE SET
                        last=MAX(last,excluded.last),seen=MAX(seen,excluded.seen),score=MAX(score,excluded.score),
                        rmin=MIN(rmin,excluded.rmin),rmax=MAX(rmax,excluded.rmax),
                        row=CASE WHEN excluded.last>=last THEN excluded.row ELSE row END""",
                        (
                            identity,
                            row["first_seen_ms"],
                            row["last_seen_ms"],
                            row["seen_count"],
                            row["score"],
                            row["rssi_min"],
                            row["rssi_max"],
                            json.dumps(row),
                        ),
                    )
            db.commit()
        with (stage / "SIGHTINGS.JSONL").open("w") as output:
            for row_json, first, last, count, score, rmin, rmax in db.execute(
                "SELECT (SELECT e2.row FROM episodes e2 WHERE e2.identity=e.identity ORDER BY e2.last DESC LIMIT 1),MIN(first),MAX(last),SUM(seen),MAX(score),MIN(rmin),MAX(rmax) FROM episodes e GROUP BY identity ORDER BY identity"
            ):
                row = json.loads(row_json)
                row.update(
                    schema_version=1,
                    first_seen_ms=first,
                    last_seen_ms=last,
                    seen_count=count,
                    score=score,
                    rssi_min=rmin,
                    rssi_max=rmax,
                    confidence="high"
                    if score >= 80
                    else "medium"
                    if score >= 50
                    else "low"
                    if score >= 20
                    else "informational",
                    hardware="E32R40T",
                    latitude=None,
                    longitude=None,
                    location_source="none",
                    privacy_mode="sanitized",
                )
                output.write(
                    json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n"
                )
                rows += 1
        db.close()
        (stage / "aggregate.sqlite").unlink()
        (stage / "SUMMARY.JSON").write_text(
            json.dumps(
                dict(
                    schema_version=1,
                    rows=rows,
                    source_files=len(files),
                    scope="historical",
                    identifiers="rekeyed_per_export",
                ),
                indent=2,
            )
            + "\n"
        )
        (stage / "README.TXT").write_text(
            "Surveillance Hound community export\n\n"
            "Offline export. Nothing was uploaded. No raw MACs, SSIDs, payloads, drone serials or coordinates.\n"
            "Identifiers are rekeyed for each export; the export key is discarded. Times are uptime offsets.\n"
            "Not map-ready: location is not collected. Radio signatures can be spoofed and do not prove surveillance.\n"
            "Rule IDs refer to the provenance ledger distributed with the recorded signature pack.\n"
            "seen_count combines cumulative maxima per observation episode, avoiding duplicate-update inflation.\n"
        )
        with (stage / "MANIFEST.SHA256").open("w") as output:
            for name in ("SIGHTINGS.JSONL", "SUMMARY.JSON", "README.TXT"):
                with (stage / name).open("rb") as content:
                    digest = hashlib.file_digest(content, "sha256").hexdigest()
                output.write(f"{digest}  {name}\n")
        from validate_export import validate_directory

        validate_directory(stage)
        os.rename(stage, destination)
    return rows


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logs", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    print(
        f"Exported {export(args.logs, args.destination)} aggregated sightings to {args.destination}"
    )
