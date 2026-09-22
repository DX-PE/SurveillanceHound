#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Compile the JSON subset of YAML. No build-time third-party Python dependencies."""

import argparse
import hashlib
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
CATEGORIES = "FLOCK AXON GLASSES SKIMMER RAVEN AIRTAG TILE SAMSUNG_TAG GOOGLE_TAG DRONE ALPR CAMERA RING IBEACON FLIPPER PWNAGOTCHI PINEAPPLE DEAUTH EVIL_TWIN".split()
KINDS = dict(
    wifi_oui="Oui",
    wifi_ssid_exact="SsidExact",
    wifi_ssid_prefix="SsidPrefix",
    ble_name_exact="NameExact",
    ble_name_prefix="NamePrefix",
    ble_uuid16="Uuid16",
    ble_uuid128="Uuid128",
    ble_company_id="Company",
    payload_prefix="Payload",
    protocol="Protocol",
    behavior="Behavior",
)
CAPS = dict(high=100, medium=79, low=49, informational=19)
PROTOCOLS = {
    "ibeacon",
    "fast_pair_model",
    "find_hub",
    "remote_id",
    "apple_find_my",
    "pwnagotchi",
}
REQUIRED = {
    "id",
    "category",
    "enabled_by_default",
    "experimental",
    "evidence_kind",
    "matcher",
    "score",
    "confidence_cap",
    "generic",
    "vendor_label",
    "reason",
    "food",
    "provenance_refs",
}
PROVENANCE = {
    "id",
    "rule_id",
    "category",
    "signal_kind",
    "value",
    "confidence_cap",
    "source_url",
    "source_title",
    "source_owner",
    "source_type",
    "source_license",
    "retrieved_at",
    "verified_by",
    "verification_method",
    "notes",
}


def load(path):
    return json.loads(path.read_text(encoding="utf-8"))


def validate(root=ROOT):
    rules = []
    for path in sorted((root / "data/signatures").glob("*.yaml")):
        entries = load(path)
        if not isinstance(entries, list):
            raise ValueError(f"{path.name}: expected array")
        rules.extend(entries)
    ledger = load(root / "data/provenance.yaml")
    provenance = {}
    for p in ledger:
        if set(p) != PROVENANCE or any(
            not isinstance(v, str) or not v.strip() for v in p.values()
        ):
            raise ValueError("Incomplete provenance entry")
        if p["id"] in provenance:
            raise ValueError("Duplicate provenance ID")
        if not p["source_url"].startswith("https://") or not re.fullmatch(
            r"\d{4}-\d{2}-\d{2}", p["retrieved_at"]
        ):
            raise ValueError("Invalid source URL or retrieval date")
        provenance[p["id"]] = p
    ids, matches = set(), set()
    for r in rules:
        if set(r) != REQUIRED:
            raise ValueError(f"Unexpected or missing rule fields: {r.get('id')}")
        if not re.fullmatch(r"[a-z][a-z0-9_.]{1,63}", r["id"]) or r["id"] in ids:
            raise ValueError("Invalid or duplicate rule ID")
        ids.add(r["id"])
        if (
            r["category"] not in CATEGORIES
            or r["evidence_kind"] not in KINDS
            or r["confidence_cap"] not in CAPS
        ):
            raise ValueError("Unknown category, evidence kind or confidence cap")
        if (
            type(r["score"]) is not int
            or not 1 <= r["score"] <= CAPS[r["confidence_cap"]]
        ):
            raise ValueError("Invalid score or cap")
        for flag in ("enabled_by_default", "experimental", "generic"):
            if type(r[flag]) is not bool:
                raise ValueError("Flags must be booleans")
        if r["experimental"] and r["enabled_by_default"]:
            raise ValueError("Experimental rules cannot ship enabled")
        for field, limit in [("vendor_label", 40), ("reason", 120), ("food", 24)]:
            if (
                not isinstance(r[field], str)
                or not 1 <= len(r[field]) <= limit
                or any(ord(c) < 32 or ord(c) > 126 for c in r[field])
            ):
                raise ValueError(f"Invalid {field}")
        if set(r["matcher"]) != {"value"} or not isinstance(r["matcher"]["value"], str):
            raise ValueError("Invalid matcher")
        v, k = r["matcher"]["value"], r["evidence_kind"]
        if not 1 <= len(v) <= 64 or any(ord(c) < 32 or ord(c) > 126 for c in v):
            raise ValueError("Invalid matcher value")
        if k in (
            "wifi_oui",
            "ble_uuid16",
            "ble_company_id",
            "payload_prefix",
            "ble_uuid128",
        ):
            if not re.fullmatch(r"[0-9A-F]+", v) or len(v) % 2:
                raise ValueError("Expected uppercase hex octets")
            if (
                k == "wifi_oui"
                and len(v) != 6
                or k in ("ble_uuid16", "ble_company_id")
                and len(v) != 4
                or k == "ble_uuid128"
                and len(v) != 32
            ):
                raise ValueError("Invalid prefix or assigned number width")
        if (
            k == "protocol"
            and v not in PROTOCOLS
            or k == "behavior"
            and v not in {"deauth_burst", "security_downgrade", "security_corroborated"}
        ):
            raise ValueError("Unsupported protocol or behavior")
        if k == "wifi_oui" and int(v[:2], 16) & 1:
            raise ValueError("Multicast address cannot identify a vendor")
        if (r["generic"] or k == "wifi_oui" and int(v[:2], 16) & 2) and CAPS[
            r["confidence_cap"]
        ] > 49:
            raise ValueError("Generic/local evidence must stay Low")
        if (
            k
            in (
                "wifi_ssid_exact",
                "wifi_ssid_prefix",
                "ble_name_exact",
                "ble_name_prefix",
            )
            and CAPS[r["confidence_cap"]] > 79
        ):
            raise ValueError("Mutable names cannot be High")
        key = (k, v)
        if key in matches:
            raise ValueError("Conflicting identical matchers")
        matches.add(key)
        refs = r["provenance_refs"]
        if (
            not isinstance(refs, list)
            or any(not isinstance(ref, str) for ref in refs)
            or len(set(refs)) != len(refs)
        ):
            raise ValueError("Invalid provenance references")
        if not r["experimental"] and not refs:
            raise ValueError("Every production rule requires provenance")
        for ref in refs:
            p = provenance.get(ref)
            if not p or any(
                p[a] != b
                for a, b in [
                    ("rule_id", r["id"]),
                    ("category", r["category"]),
                    ("signal_kind", k),
                    ("value", v),
                    ("confidence_cap", r["confidence_cap"]),
                ]
            ):
                raise ValueError(f"Provenance mismatch: {r['id']}")
    if {r["category"] for r in rules} != set(CATEGORIES):
        raise ValueError("All 19 categories require an entry")
    return sorted(rules, key=lambda r: r["id"])


def generate(root=ROOT, check=False):
    rules = validate(root)
    live = [r for r in rules if r["enabled_by_default"] and not r["experimental"]]
    canonical = json.dumps(live, sort_keys=True, separators=(",", ":")).encode()
    version = "v1-" + hashlib.sha256(canonical).hexdigest()[:12]
    lines = [
        "// SPDX-License-Identifier: CC0-1.0",
        "// Generated by tools/compile_signatures.py; do not edit.",
        "#pragma once",
        '#include "sniffer/core.hpp"',
        "namespace sniffer {",
        f'inline constexpr char signature_pack_version[] = "{version}";',
        f"inline constexpr std::array<Rule, {len(live)}> production_rules{{{{",
    ]
    for r in live:
        q = json.dumps
        lines.append(
            f"    {{{q(r['id'])}, Category::{r['category']}, Kind::{KINDS[r['evidence_kind']]}, "
            f"{q(r['matcher']['value'])}, {r['score']}, {CAPS[r['confidence_cap']]}, "
            f"{str(r['generic']).lower()}, {q(r['vendor_label'])}, {q(r['reason'])}}},"
        )
    lines += ["}};", "}", ""]
    outputs = {
        "signatures_generated.h": "\n".join(lines).encode(),
        "signatures.bin": b"SSN1" + canonical + b"\n",
    }
    for name, content in outputs.items():
        path = root / "generated" / name
        if check:
            if not path.exists() or path.read_bytes() != content:
                raise ValueError(
                    f"Stale generated file: {name}; run tools/compile_signatures.py"
                )
        else:
            path.parent.mkdir(exist_ok=True)
            path.write_bytes(content)
    return len(live), len(rules)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    try:
        live, total = generate(check=args.check)
        print(
            f"Signatures validated: {live} enabled / {total} entries; provenance complete"
        )
    except (ValueError, KeyError, TypeError) as exc:
        sys.exit(str(exc))
