"""Independent exact Fraction oracle for the production ClockLength/selector seam."""
import subprocess
import sys
from fractions import Fraction

result = subprocess.run([sys.argv[1]], check=True, text=True, capture_output=True)
rows = result.stdout.splitlines()
for row in rows:
    fs, period, base, denominator, scale, irregularity, random24, ticks, offset, limits = row.split()
    fs, period, base, denominator, irregularity, random24, ticks, offset, limits = map(
        int, (fs, period, base, denominator, irregularity, random24, ticks, offset, limits))
    jitter = 1 + Fraction(3 * irregularity, 400) * (Fraction(2 * random24, 2**24) - 1)
    raw = Fraction(base, denominator) * Fraction.from_float(float.fromhex(scale)) * jitter
    length = min(max(raw, 1), 4 * fs)
    expected_limits = 2 if raw != length else 0
    increment = next(2**s for s in range(12) if Fraction(2**s * period, 16) >= length)
    expected_offset = (increment * period + 15) // 16
    assert (ticks, offset, limits) == (increment, expected_offset, expected_limits), (row, length)
    print(row)
assert len(rows) == 5760, len(rows)
print(f"Exact Fraction vs production ClockLength/selector: {len(rows)} rows PASS")
