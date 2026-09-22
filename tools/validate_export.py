#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
import argparse
import hashlib
import json
from pathlib import Path
import re
from compile_signatures import CATEGORIES

FIELDS = set(
    "schema_version session_id identifier category rule_ids first_seen_ms last_seen_ms score rssi_min rssi_max seen_count channel radio firmware_version signature_pack region source_privacy_mode confidence observed_at time_quality hardware latitude longitude location_source privacy_mode".split()
)


def validate_directory(directory):
    directory = Path(directory)
    entries = (directory / "MANIFEST.SHA256").read_text().splitlines()
    expected = {"SIGHTINGS.JSONL", "SUMMARY.JSON", "README.TXT"}
    seen = set()
    for line in entries:
        match = re.fullmatch(r"([0-9a-f]{64})  ([A-Z.]+)", line)
        if not match or match[2] not in expected or match[2] in seen:
            raise ValueError("Invalid manifest")
        with (directory / match[2]).open("rb") as source:
            if hashlib.file_digest(source, "sha256").hexdigest() != match[1]:
                raise ValueError("Export hash mismatch")
        seen.add(match[2])
    if seen != expected:
        raise ValueError("Incomplete manifest")
    count = 0
    with (directory / "SIGHTINGS.JSONL").open("rb") as stream:
        while True:
            line = stream.readline(4097)
            if not line:
                break
            if len(line) > 4096 or not line.endswith(b"\n"):
                raise ValueError("Invalid JSONL boundary")
            row = json.loads(line)
            if set(row) != FIELDS:
                raise ValueError("Unexpected or missing export fields")
            if (
                row["schema_version"] != 1
                or row["category"] not in CATEGORIES
                or row["latitude"] is not None
                or row["longitude"] is not None
                or row["location_source"] != "none"
            ):
                raise ValueError("Invalid location/schema/category")
            if not re.fullmatch("[A-Z2-7]{20}", row["identifier"]) or not re.fullmatch(
                "[a-f0-9]{32}", row["session_id"]
            ):
                raise ValueError("Invalid export token")
            if (
                row["privacy_mode"] != "sanitized"
                or row["observed_at"] is not None
                or row["time_quality"] != "uptime_only"
            ):
                raise ValueError("Invalid privacy/time metadata")
            score = row["score"]
            if type(score) is not int or not 1 <= score <= 100:
                raise ValueError("Invalid score")
            for field, minimum, maximum in (
                ("first_seen_ms", 0, 2**53 - 1),
                ("last_seen_ms", row["first_seen_ms"], 2**53 - 1),
                ("seen_count", 1, 2**53 - 1),
                ("rssi_min", -127, 20),
                ("rssi_max", row["rssi_min"], 20),
                ("channel", 0, 14),
            ):
                value = row[field]
                if type(value) is not int or not minimum <= value <= maximum:
                    raise ValueError(f"Invalid {field}")
            if row["radio"] not in ("wifi", "ble") or row["region"] not in (
                "US",
                "EU",
                "unknown",
            ):
                raise ValueError("Invalid radio/region")
            if (
                row["source_privacy_mode"] not in ("private", "research")
                or row["hardware"] != "E32R40T"
            ):
                raise ValueError("Invalid source metadata")
            if not re.fullmatch(
                r"[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9.]+)?", row["firmware_version"]
            ) or not re.fullmatch(r"v[0-9]+-[a-f0-9]{12}", row["signature_pack"]):
                raise ValueError("Invalid build metadata")
            expected_grade = (
                "high"
                if score >= 80
                else "medium"
                if score >= 50
                else "low"
                if score >= 20
                else "informational"
            )
            if row["confidence"] != expected_grade:
                raise ValueError("Confidence does not match score")
            ids = row["rule_ids"]
            if (
                not isinstance(ids, list)
                or not 1 <= len(ids) <= 4
                or any(
                    not isinstance(i, str)
                    or not re.fullmatch(r"[a-z][a-z0-9_.]{1,63}", i)
                    for i in ids
                )
                or len(set(ids)) != len(ids)
            ):
                raise ValueError("Invalid rule IDs")
            count += 1
    if json.loads((directory / "SUMMARY.JSON").read_text())["rows"] != count:
        raise ValueError("Summary count mismatch")
    return count


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    print(
        f"Valid export: {validate_directory(parser.parse_args().directory)} sightings"
    )
