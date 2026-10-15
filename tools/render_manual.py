#!/usr/bin/env python3
"""Render the versioned manual using Pandoc and an isolated Chrome profile."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', type=Path, default=Path('build/manual'))
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
out = a.output.resolve()
version = Path('VERSION').read_text().strip()
chrome = next((c for c in (shutil.which('google-chrome'), shutil.which('chromium'),
              '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome') if c and Path(c).is_file()), None)
if not chrome:
    raise SystemExit('Chrome is required. No PDF was produced.')
shutil.copyfile('docs/Pop-Manual.print.css', out/'Pop-Manual.print.css')
subprocess.run(['pandoc', 'docs/Pop-Manual.md', '--standalone', '--toc', '--toc-depth=1',
                '--css=Pop-Manual.print.css', '--metadata', f'date=v{version}',
                '-o', str(out/'Pop-Manual.html')], check=True)
with tempfile.TemporaryDirectory(prefix='pop-manual-profile-') as profile:
    subprocess.run([chrome, '--headless', '--disable-gpu', '--no-pdf-header-footer',
                    '--no-first-run', '--no-default-browser-check', f'--user-data-dir={profile}',
                    f'--print-to-pdf={out / "Pop-Manual.pdf"}', (out/'Pop-Manual.html').as_uri()],
                   check=True, timeout=90)
subprocess.run(['pdftotext', '-layout', str(out/'Pop-Manual.pdf'), str(out/'Pop-Manual.txt')], check=True)
text = (out/'Pop-Manual.txt').read_text()
for required in (version, 'ThPp', 'ThPo', '20.0', 'Clock', 'Retrigger', 'Irregularity'):
    if required not in text:
        raise SystemExit(f'Manual PDF missing {required!r}')
print(f'Rendered {out / "Pop-Manual.pdf"}. Inspect all rasterized pages before release.')
