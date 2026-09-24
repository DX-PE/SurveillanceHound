# SPDX-License-Identifier: Apache-2.0
"""Reject damaged images and mismatched flashing metadata before publication."""

import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "flasher", ROOT / "tools/build_flasher.py"
)
flasher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(flasher)


class FlasherTests(unittest.TestCase):
    def test_release_targets_only_esp32_factory_install(self):
        release = flasher.release_config()
        manifest = flasher.manifest_for(release)
        self.assertEqual(len(manifest["builds"]), 1)
        self.assertEqual(manifest["builds"][0]["chipFamily"], "ESP32")
        self.assertEqual(
            manifest["builds"][0]["parts"], [{"path": release["filename"], "offset": 0}]
        )
        self.assertIs(manifest["new_install_prompt_erase"], False)
        self.assertEqual(manifest["new_install_improv_wait_time"], 0)

    def test_truncation_and_same_length_corruption_are_rejected(self):
        image = bytes(range(256))
        release = {"size": len(image), "sha256": hashlib.sha256(image).hexdigest()}
        flasher.verify_image(image, release)
        for damaged in (image[:-1], image + b"x", b"x" + image[1:]):
            with self.assertRaises(ValueError):
                flasher.verify_image(damaged, release)

    def test_wrong_offset_chip_and_stale_metadata_fail_final_check(self):
        image = b"synthetic test image, never firmware"
        release = flasher.release_config()
        release.update(size=len(image), sha256=hashlib.sha256(image).hexdigest())
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp)
            (output / release["filename"]).write_bytes(image)
            (output / "release.json").write_text(json.dumps(release))
            manifest = flasher.manifest_for(release)
            (output / "manifest.json").write_text(json.dumps(manifest))
            flasher.check_output(output, release)
            for key, value in (
                ("chipFamily", "ESP32-S3"),
                ("parts", [{"path": release["filename"], "offset": 65536}]),
            ):
                changed = flasher.manifest_for(release)
                changed["builds"][0][key] = value
                (output / "manifest.json").write_text(json.dumps(changed))
                with self.assertRaises(ValueError):
                    flasher.check_output(output, release)
            (output / "manifest.json").write_text(json.dumps(manifest))
            (output / "release.json").write_text(
                json.dumps({**release, "version": "old"})
            )
            with self.assertRaises(ValueError):
                flasher.check_output(output, release)


if __name__ == "__main__":
    unittest.main()
