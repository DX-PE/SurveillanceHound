# SPDX-License-Identifier: Apache-2.0
import copy
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import compile_signatures as signatures
from export_logs import export, sanitize
from validate_export import validate_directory
from recover_logs import recover


class Signatures(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        shutil.copytree(ROOT / "data", self.root / "data")
        (self.root / "generated").mkdir()

    def tearDown(self):
        self.temp.cleanup()

    def mutate(self, fn):
        path = self.root / "data/signatures/catalog.yaml"
        rules = json.loads(path.read_text())
        fn(rules)
        path.write_text(json.dumps(rules))

    def test_deterministic(self):
        signatures.generate(self.root)
        first = (self.root / "generated/signatures.bin").read_bytes()
        signatures.generate(self.root)
        self.assertEqual(first, (self.root / "generated/signatures.bin").read_bytes())
        signatures.generate(self.root, check=True)

    def test_missing_provenance(self):
        self.mutate(lambda r: r[0].update(provenance_refs=[]))
        with self.assertRaises(ValueError):
            signatures.validate(self.root)

    def test_experimental_must_be_disabled(self):
        self.mutate(lambda r: r[-1].update(experimental=True, enabled_by_default=True))
        with self.assertRaises(ValueError):
            signatures.validate(self.root)

    def test_duplicate_ids(self):
        self.mutate(lambda r: r.append(copy.deepcopy(r[0])))
        with self.assertRaises(ValueError):
            signatures.validate(self.root)

    def test_invalid_caps(self):
        self.mutate(lambda r: r[0].update(generic=True))
        with self.assertRaises(ValueError):
            signatures.validate(self.root)

    def test_provenance_binding(self):
        self.mutate(lambda r: r[0]["matcher"].update(value="remote_id"))
        with self.assertRaises(ValueError):
            signatures.validate(self.root)

    def test_schemas(self):
        import jsonschema

        schema = json.loads((ROOT / "data/schemas/signature.schema.json").read_text())
        for rule in signatures.validate():
            jsonschema.validate(rule, schema)


class Export(unittest.TestCase):
    def row(self):
        return dict(
            schema_version=1,
            session_id="a" * 32,
            identifier="A" * 20,
            category="GOOGLE_TAG",
            rule_ids=["ble.google.find_hub"],
            first_seen_ms=100,
            uptime_ms=1000,
            score=85,
            rssi_min=-80,
            rssi_max=-50,
            seen_count=3,
            channel=0,
            radio="ble",
            firmware_version="0.1.0-dev",
            signature_pack="v1-123456abcdef",
            region="US",
            privacy_mode="research",
            raw_mac="AA:BB:CC:DD:EE:FF",
            ssid="SECRET-SSID",
            drone_serial="SECRET-SERIAL",
            latitude=42,
            longitude=-70,
            reason="SECRET-SSID",
        )

    def test_export_sanitizes_and_aggregates(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            logs = root / "LOGS"
            logs.mkdir()
            row = self.row()
            later = dict(row, seen_count=8, uptime_ms=3000)
            (logs / "S.JSONL").write_text(
                json.dumps(row)
                + "\n"
                + json.dumps(later)
                + "\n"
                + json.dumps(later)
                + "\n"
            )
            self.assertEqual(export(logs, root / "EXPORT"), 1)
            self.assertEqual(validate_directory(root / "EXPORT"), 1)
            result = (root / "EXPORT/SIGHTINGS.JSONL").read_text()
            sighting = json.loads(result)
            self.assertEqual(sighting["seen_count"], 8)
            for value in ("SECRET", "AA:BB", "raw_mac", "ssid", "serial"):
                self.assertNotIn(value, result)
            self.assertIsNone(sighting["latitude"])
            self.assertNotEqual(sighting["identifier"], row["identifier"])
            with self.assertRaises(ValueError):
                export(logs, root / "EXPORT")

    def test_unknown_rule_rejected(self):
        row = self.row()
        row["rule_ids"] = ["demo.flipper"]
        with self.assertRaises(ValueError):
            sanitize(row, b"key", {r["id"]: r for r in signatures.validate()})

    def test_malicious_types_rejected(self):
        for key, value in [
            ("seen_count", True),
            ("score", 101),
            ("identifier", "AA:BB:CC:DD:EE:FF"),
            ("region", "SECRET"),
        ]:
            row = self.row()
            row[key] = value
            with self.assertRaises(ValueError):
                sanitize(row, b"key", {r["id"]: r for r in signatures.validate()})

    def test_recovery(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "S.JSONL"
            path.write_bytes(b'{"ok":1}\n{"partial')
            self.assertEqual(recover(path), 9)
            self.assertEqual(path.read_bytes(), b'{"ok":1}\n')
            path.write_bytes(b"INVALID\n")
            with self.assertRaises(ValueError):
                recover(path)
            self.assertEqual(path.read_bytes(), b"INVALID\n")

    def test_hash_tampering(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            log = root / "S.JSONL"
            log.write_text(json.dumps(self.row()) + "\n")
            export(log, root / "OUT")
            (root / "OUT/README.TXT").write_text("tampered")
            with self.assertRaises(ValueError):
                validate_directory(root / "OUT")


if __name__ == "__main__":
    unittest.main()
