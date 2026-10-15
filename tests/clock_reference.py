#!/usr/bin/env python3
"""Independent Decimal clock oracle. Never imports production scheduling code."""
from decimal import Decimal as D, localcontext, ROUND_CEILING
import json
from pathlib import Path


def advance(x):
    x ^= (x << 13) & 0xffffffff
    x ^= x >> 17
    return (x ^ (x << 5)) & 0xffffffff


def vector(seed, contour, duration):
    p, fs, count, rate = 24001, 48000, 5, 1
    rng = advance(seed)
    request = rng
    deadline = 2 * p
    rises = []
    m = previous = draws = 0
    while True:
        u = D(previous) / deadline if duration else D(len(rises)) / (count - 1)
        h = [D(0), 1-2*u, 2*u-1, 2*abs(2*u-1)-1, 1-2*abs(2*u-1)][contour]
        rng = advance(rng)
        draws += 1
        r = D(rng >> 8) / (1 << 24)
        length = min(D(4*fs), max(D(1), D(p)/2 * (D(2).ln()*h).exp() * (1+D('0.375')*(2*r-1))))
        for shift in range(12):
            d = 1 << shift
            if D(d*p)/16 < length:
                continue
            due = ((m+d)*p+15)//16
            if due < (1 if not rises else previous+288):
                continue
            m += d
            break
        else:
            raise AssertionError('fixed set exhausted')
        if duration and due >= deadline:
            break
        rises.append(due)
        previous = due
        if not duration and len(rises) == count:
            break
    return dict(seed=seed, contour=contour, duration=duration, rises=rises,
                end=max(deadline, rises[-1]+240) if duration else rises[-1]+240,
                request=request, generation=rng, draws=draws)


def main():
    with localcontext() as ctx:
        ctx.prec = 80
        rows = [vector(seed, contour, duration) for seed in (1,17) for contour in range(5) for duration in (0,1)]
    root = Path(__file__).parent/'fixtures'/'clock'
    root.mkdir(exist_ok=True)
    (root/'vectors.json').write_text(json.dumps(rows, indent=2)+'\n')
    text = '#pragma once\n#include <cstdint>\nnamespace clock_fixture {\nstruct Case { unsigned seed; int contour, duration; const uint64_t *rises; unsigned size; uint64_t end; uint32_t request, generation; unsigned draws; };\n'
    for i, row in enumerate(rows):
        text += f'const uint64_t rises{i}[] = {{'+','.join(map(str,row['rises']))+'};\n'
    text += 'const Case cases[] = {\n'
    for i, r in enumerate(rows):
        text += f'{{{r["seed"]},{r["contour"]},{r["duration"]},rises{i},{len(r["rises"])},{r["end"]},{r["request"]}u,{r["generation"]}u,{r["draws"]}}},\n'
    (root/'vectors.h').write_text(text+'};\n}\n')


if __name__ == '__main__':
    main()
