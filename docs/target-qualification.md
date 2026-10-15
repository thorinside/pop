# Operator qualification protocol

Status NOT RUN. Owner/operator only. Agents must not install, flash, access presets or operate the disting NT. Native test traces do not fill these result rows.

Before testing, record the intended version/tag, exact source SHA, downloaded pop.o SHA256/size, module firmware/API, serial or identifying label, sample rate, processing block size, algorithm order, routing, output modes, equipment, receiver model and tolerances. Preserve the original preset separately. Record timestamps and raw captures, including failures.

| Check | Operator procedure | Required record | Result |
|---|---|---|---|
| Loader and identity | Load the exact downloaded ThPp object using the matching official host procedure | Literal scan/load errors, View info, memory usage, firmware and object hashes | NOT RUN |
| Default patch | Fresh Internal Count4/Rate20/Flat/Irregularity0/amount100, dedicated Burst Output1 and Active Output2 | Input and both output captures, expected50/100/150/200ms at48kHz, final fall205ms | NOT RUN |
| Electrical pulse | Measure multiple highs/lows at each supported rate and buffer setting, include fast limits and restarted highs | W=ceil(.005Fs), G=ceil(.001Fs), measured levels/widths/gaps, sample and instrument tolerances, representative receivers | NOT RUN |
| Input thresholds | Sweep around1V and2V, short pulses, held highs, noise and initial-high rearming | Calibrated actual voltages, widths, false/missed events and receiver conditions | NOT RUN |
| Clock operation | Separate clock/start, both Count and Duration, ratios/contours/irregularity and fractional input periods | Synchronized input/Burst/Active captures and no forced onset | NOT RUN |
| Clock stop | Drop clock or send an invalid period before onset, on a due event and during high | Detection at3P, no new event at detection, full existing high, immediate Active-low, no recovery replay | NOT RUN |
| Shared input | Same input selected for Clock and Trigger, Off then On | Off lets request survive. Repeated On edges can replace pending requests indefinitely | NOT RUN |
| Lifecycle | Save/load, remap, output modes, rate changes, supported bypass/re-enable and reconstruction | No resurrected burst, held Fire/high disarming, exact host actions and observable behavior | NOT RUN |
| Resources | Worst-case continuous activity, frequent restart, acquisition/loss/invalid cases | Actual cycle-counter repetitions/distribution, View info and target-derived budget with justification | NOT RUN |
| Musical verdict | Both terminations/timing sources, regular corner, all contours, defaults and several receivers | Neal's explicit dated verdict and settings, not a native statistical proxy | NOT RUN |

Use dedicated outputs when judging voltage and separation. Add mode guarantees only Pop's contribution; other algorithms can add or overwrite the final bus. Document that distinction in each measurement. No fixed CPU percentage, memory limit, firmware compatibility matrix or electrical tolerance is assumed by this protocol.

A reported earlier development object load is historical observation without final-head hash correlation. It does not mark any row above passed. Record later owner evidence with its exact provenance without rewriting the earlier job outcome.
