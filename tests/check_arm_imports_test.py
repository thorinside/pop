import json
from pathlib import Path
import unittest
from check_arm_imports import check


class ImportGateTest(unittest.TestCase):
    def test_pinned_exports_and_loader_specials(self):
        manifest = json.loads((Path(__file__).parent / "fixtures/nt-beta18-exports.json").read_text())
        known = [entry["name"] for entry in manifest["functions"]] + list(manifest["loaderSpecials"])
        self.assertEqual(check(known, manifest), [])
        for unknown in ["__aeabi_d2ulz", "__aeabi_ul2d", "__aeabi_uldivmod", "arbitrary_future_import"]:
            self.assertEqual(check(known + [unknown], manifest), [unknown])


if __name__ == "__main__":
    unittest.main()
