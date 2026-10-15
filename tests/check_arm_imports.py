#!/usr/bin/env python3
"""Check the complete ARM import set against one pinned offline NT image audit."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def check(symbols, manifest):
    supported = {entry["name"] for entry in manifest["functions"]}
    supported.update(manifest["loaderSpecials"])
    return sorted(set(symbols) - supported)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("object", type=Path)
    parser.add_argument("--nm", default="arm-none-eabi-nm")
    parser.add_argument("--manifest", type=Path,
                        default=Path(__file__).parent / "fixtures/nt-beta18-exports.json")
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text())
    output = subprocess.check_output([args.nm, "-u", str(args.object)], text=True)
    symbols = [line.split()[-1] for line in output.splitlines() if line.strip()]
    unknown = check(symbols, manifest)
    receipt = {
        "object": str(args.object),
        "objectSha256": hashlib.sha256(args.object.read_bytes()).hexdigest(),
        "firmwareImageSha256": manifest["imageSha256"],
        "firmwareProvenance": manifest["provenance"],
        "manifestSha256": hashlib.sha256(args.manifest.read_bytes()).hexdigest(),
        "imports": symbols,
        "unsupported": unknown,
        "status": "FAIL" if unknown else "PASS",
        "scope": "Offline symbol availability only. Not target loading, relocation or firmware compatibility proof."
    }
    print(json.dumps(receipt, indent=2))
    return bool(unknown)


if __name__ == "__main__":
    sys.exit(main())
