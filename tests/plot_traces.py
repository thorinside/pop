#!/usr/bin/env python3
"""Render native adapter voltage CSVs, not physical measurements."""
import csv
import html
from pathlib import Path
import subprocess
import sys

output = Path(sys.argv[1])
output.mkdir(parents=True, exist_ok=True)
head = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
for name in ["01-baseline-count", "05-restart", "duration4800-1", "duration4848-100", "duration9600-10"]:
    rows = list(csv.DictReader(Path("build", name + ".csv").open()))
    last = int(rows[-1]["sample"])
    svg = ['<svg xmlns="http://www.w3.org/2000/svg" width="1280" height="360">',
           '<rect width="1280" height="360" fill="white"/>',
           f'<text x="60" y="30" font-family="Helvetica" font-size="20">{html.escape(name)}. Native adapter. Not hardware.</text>',
           f'<text x="60" y="55" font-family="Helvetica" font-size="13">Commit {head}</text>']
    for key, baseline, colour in [("burstV", 150, "#1668b8"), ("activeV", 270, "#a33062")]:
        points = []
        previous = None
        for row in rows:
            t, value = int(row["sample"]), float(row[key])
            if value != previous:
                x = 100 + 1100 * t / last
                if previous is not None:
                    points.append(f"{x:.4f},{baseline-previous*12:.4f}")
                points.append(f"{x:.4f},{baseline-value*12:.4f}")
            previous = value
        points.append(f"1200,{baseline-previous*12:.4f}")
        svg += [f'<text x="10" y="{baseline-25}" font-family="Helvetica" font-size="14">{key}</text>',
                f'<text x="75" y="{baseline-60}" font-family="Helvetica" font-size="12">5V</text>',
                f'<text x="75" y="{baseline}" font-family="Helvetica" font-size="12">0V</text>',
                f'<path d="M100 {baseline} H1200" stroke="#bbb" fill="none"/>',
                f'<polyline points="{" ".join(points)}" fill="none" stroke="{colour}" stroke-width="2"/>']
    svg += [f'<text x="100" y="310" font-family="Helvetica" font-size="14">Sample 0</text>',
            f'<text x="1050" y="310" font-family="Helvetica" font-size="14">Sample {last}</text>',
            '<text x="100" y="338" font-family="Helvetica" font-size="13">Fs48000. Exact sample values and comparison assertions are in the accompanying CSV and test logs.</text>', '</svg>']
    path = output / (name + ".svg")
    path.write_text("\n".join(svg))
    subprocess.run(["magick", "-background", "white", str(path), str(path.with_suffix(".png"))], check=True)
    print(path.with_suffix(".png"))
