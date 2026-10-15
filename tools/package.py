#!/usr/bin/env python3
"""Create and inspect the exact SD layout plus release provenance and checksums."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import zipfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--build', type=Path, default=Path('build'))
p.add_argument('--output', type=Path, default=Path('build/release'))
p.add_argument('--require-pdf', action='store_true')
a = p.parse_args()
version = Path('VERSION').read_text().strip()
assert re.fullmatch(r'(0|[1-9][0-9]*)(\.(0|[1-9][0-9]*)){2}(-[0-9A-Za-z.-]+)?', version)
head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip()
sdk = subprocess.check_output(['git', '-C', 'distingNT_API', 'rev-parse', 'HEAD'], text=True).strip()
assert sdk == '6975a630cb5f86de6f8c709f5467f4666bbd4579'
subprocess.run(['python3', 'tests/check_arm_imports.py', str(a.build/'pop.o')], check=True)
subprocess.run(['python3', 'tests/check_arm_identity.py', str(a.build/'pop.o')], check=True)
a.output.mkdir(parents=True, exist_ok=True)
obj = (a.build/'pop.o').read_bytes()
shutil.copyfile(a.build/'pop.o', a.output/'pop.o')
archive = a.output/f'pop-v{version}.zip'
with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED) as z:
    entry = zipfile.ZipInfo('programs/plug-ins/pop.o', date_time=(2026,1,1,0,0,0))
    entry.compress_type = zipfile.ZIP_DEFLATED
    entry.external_attr = 0o100644 << 16
    z.writestr(entry, obj)
with zipfile.ZipFile(archive) as z:
    assert z.namelist() == ['programs/plug-ins/pop.o'] and z.testzip() is None
    assert z.read('programs/plug-ins/pop.o') == obj
manual = a.build/'manual'/'Pop-Manual.pdf'
assets = [a.output/'pop.o', archive]
if manual.exists():
    target = a.output/f'Pop-Manual-v{version}.pdf'
    shutil.copyfile(manual, target)
    assets.append(target)
elif a.require_pdf:
    raise SystemExit('Required manual PDF is absent')
provenance = dict(version=version, commit=head, sdk=sdk,
    compiler=subprocess.check_output(['arm-none-eabi-g++','--version'],text=True).splitlines()[0],
    objectSha256=hashlib.sha256(obj).hexdigest(), objectBytes=len(obj), guid='ThPp',
    verificationScope='Local build/package inspection, not independent acceptance, publication or physical qualification.')
(a.output/'provenance.json').write_text(json.dumps(provenance, indent=2)+'\n')
assets.append(a.output/'provenance.json')
(a.output/'SHA256SUMS').write_text(''.join(f'{hashlib.sha256(f.read_bytes()).hexdigest()}  {f.name}\n' for f in assets))
print(json.dumps(provenance, indent=2))
