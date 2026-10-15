#!/usr/bin/env python3
"""Independent offline R1-R4 oracle, frozen before production code.

Decimal transcendental arithmetic (60 digits), integer xorshift, no product imports.
The accepted Spec's hand-calculated AC-01/10/11 literals cross-check this oracle.
Run from repository root. This writes fixtures, never run it during make test.
"""
from decimal import Decimal as D, getcontext, ROUND_CEILING
import json
from pathlib import Path
getcontext().prec = 60

def advance(x):
    x ^= (x << 13) & 0xffffffff
    x ^= x >> 17
    return (x ^ ((x << 5) & 0xffffffff)) & 0xffffffff

def trace(n=4, rate=10, contour=0, amount=0, irregularity=0, seed=1, duration=None):
    request = advance(seed)
    x = request
    t = 0
    rises, states = [], []
    k = 0
    while True:
        u = D(k)/D(n-1) if duration is None and n > 2 else D('0.5')
        if duration is not None:
            u = D(t)/D(duration)
        h = [D(0), 1-2*u, 2*u-1, 2*abs(2*u-1)-1, 1-2*abs(2*u-1)][contour]
        x = advance(x)
        states.append(x)
        r = D(x >> 8)/D(16777216)
        scale = (D(2).ln()*D(amount)/100*h).exp()
        jitter = 1 + D('0.75')*D(irregularity)/100*(2*r-1)
        length = max(D(1), min(D(192000), D(48000)/D(rate)*scale*jitter))
        gap = int(length.to_integral_value(rounding=ROUND_CEILING))
        t += max(gap, 288 if rises else 1)
        if duration is not None and t >= duration:
            break
        rises.append(t)
        k += 1
        if duration is None and k == n:
            break
    end = max(duration or 0, rises[-1]+240 if rises else 0)
    return dict(rises=rises, falls=[r+240 for r in rises], activeEnd=end,
                request=request, generation=x, draws=len(states), states=states)

cases = []
for n in (1, 2, 4, 64):
    cases.append(dict(name=f'flat{n}', n=n, rate=10, contour=0, amount=0,
                      irregularity=0, seed=1, duration=None))
for contour in range(1, 5):
    cases.append(dict(name=f'contour{contour}', n=5, rate=10, contour=contour,
                      amount=100, irregularity=0, seed=1, duration=None))
for duration, rate in ((9600,10),(4800,1),(4848,100),(4801,10),(4799,10)):
    cases.append(dict(name=f'duration{duration}-{rate}', n=8, rate=rate, contour=0,
                      amount=0, irregularity=0, seed=1, duration=duration))
for seed in (1, 17, 1024, 4294967295):
    cases.append(dict(name=f'seed{seed}', n=8, rate=20, contour=2, amount=75,
                      irregularity=50, seed=seed, duration=None))
for c in cases:
    c['expected'] = trace(**{k:v for k,v in c.items() if k != 'name'})
assert cases[2]['expected']['rises'] == [4800,9600,14400,19200]
assert cases[4]['expected']['rises'] == [9600,16389,21189,24584,26984]
assert [advance(1), advance(270369), advance(67634689), advance(2647435461)] == [270369,67634689,2647435461,307599695]
Path('tests/fixtures/internal.json').write_text(json.dumps(cases, indent=2)+'\n')
lines = ['#pragma once', '#include <cstdint>', 'namespace fixture {',
         'struct Case { const char* name; int n, rate, contour, amount, irregularity; uint32_t seed; int duration; int size; uint64_t rises[64]; uint64_t end; uint32_t request, generation; int draws; };',
         'inline constexpr Case cases[] = {']
for c in cases:
    e=c['expected']
    values=[json.dumps(c['name']),str(c['n']),str(c['rate']),str(c['contour']),str(c['amount']),str(c['irregularity']),str(c['seed'])+'u',str(c['duration'] or 0),str(len(e['rises'])), '{'+','.join(map(str,e['rises']))+'}',str(e['activeEnd']),str(e['request'])+'u',str(e['generation'])+'u',str(e['draws'])]
    lines.append('{'+','.join(values)+'},')
lines += ['};','}']
Path('tests/fixtures/internal.h').write_text('\n'.join(lines)+'\n')
print(f'Frozen {len(cases)} independent Decimal fixtures')
