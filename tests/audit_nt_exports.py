#!/usr/bin/env python3
"""Reproduce the committed offline export manifest from the pinned beta18 archive."""
import hashlib
import json
from pathlib import Path
import struct
import sys
import zipfile

manifest_path = Path(__file__).parent / "fixtures/nt-beta18-exports.json"
manifest = json.loads(manifest_path.read_text())
image = zipfile.ZipFile(sys.argv[1]).read(manifest["archiveMember"])
assert hashlib.sha256(image).hexdigest() == manifest["imageSha256"]
base = int(manifest["mappedImageBase"], 16)
entries = []
for offset in range(int(manifest["tableStartOffset"], 16),
                    int(manifest["tableEndOffset"], 16), 8):
    pointer, address = struct.unpack_from("<II", image, offset)
    location = pointer - base
    assert 0 <= location < len(image)
    name = image[location:image.index(0, location)].decode("ascii")
    assert name and all(32 <= ord(c) < 127 for c in name) and address
    entries.append({"name": name, "address": hex(address), "entryOffset": hex(offset)})
assert entries == manifest["functions"]
assert len({entry["name"] for entry in entries}) == len(entries) == 114
for name, special in manifest["loaderSpecials"].items():
    literal = int(special["namePointerLiteral"], 16)
    pointer, = struct.unpack_from("<I", image, literal)
    assert pointer == int(special["nameAddress"], 16)
    assert image[pointer-base:pointer-base+len(name)+1] == name.encode() + b"\0"
evidence = manifest["loaderEvidence"]
start = int(evidence["imageOffsetStart"], 16)
end = int(evidence["imageOffsetEnd"], 16)
assert image[start:end].hex() == evidence["initializationBytesHex"]
start = int(evidence["literalPoolOffset"], 16)
expected = bytes.fromhex(evidence["literalPoolHex"])
assert image[start:start+len(expected)] == expected
print("Pinned firmware hash, 114 export pairs and both loader-special initialization byte ranges PASS")
