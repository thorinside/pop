# Pop

A brief disturbance in time. Pop turns a trigger into a finite burst of gates for the Expert Sleepers disting NT. Choose an exact Count, or a Duration window whose event count can vary. Contours and irregularity shape the spacing, not the gate voltage.

**v0.1.0-beta.1 is an experimental prerelease version, not physical qualification.** Internal and Clock Count/Duration are implemented. Shaped Clock host/Cortex-M/firmware `exp2` subdivision equivalence remains **UNVERIFIED**. Physical AC-08/AC-09 remain operator-owned and not run. Release-candidate review, acceptance and verified publication are separate gates; an unavailable emulator alone is not a release veto under the owner's experimental-risk disposition.

## Four taps

1. Keep Timing at Internal. Patch a trigger to Input 1 and Output 1 to a drum or envelope trigger input.
2. Fresh instances use Count 4, Rate 20.0 Hz, Irregularity 0%, Flat and Contour amount 100%.
3. Send a low, then a rising trigger. At 48 kHz the gates rise 50, 100, 150 and 200 ms after the accepted start. There is no immediate onset gate.
4. For Manual Start, let Off be observed, then select Fire. Leaving Fire selected does not repeat.

Burst defaults to Output 1, host bus 13. Active defaults to Output 2, bus 14. Both contribute 0 V or +5 V. Each high is 5 ms rounded up to whole samples. The low separation is at least 1 ms rounded up. Add mode adds only Pop's contribution, not a ceiling on the shared bus.

## Clock and trigger are different jobs

Clock supplies timing. Trigger requests a finite burst. For a first patch, use a continuous clock on Input 1, a separate trigger on Input 2, Timing Clock, Clock input Input 1 and Trigger input Input 2. Try Count 4, Rate x4, Flat, Irregularity 0% and Retrigger Off.

Two fresh clock edges establish a period. A request activates on an eligible strictly later edge, then waits its generated onset interval. Valid periods are 50 ms to 2 seconds. The activated burst holds its period despite valid tempo changes. Detected clock failure cancels future events and lowers Active immediately, while an existing Burst high drains. Recovery requires a fresh request.

Trigger and Clock may share an input. Use Retrigger Off to let a waiting request survive subsequent shared edges. With On, every shared edge can replace the pending request before activation, preventing output. This is deliberate replacement behavior.

## Manual and compatibility

The [player manual](docs/Pop-Manual.md) covers every parameter, ranges, defaults, routing, shared inputs, contours, installation, troubleshooting and patch recipes. `make manual` renders a versioned PDF from that source with Pandoc and an isolated Chrome profile.

A verified release's installable object belongs at `/programs/plug-ins/pop.o`. Pop's identity is **ThPp**. Earlier ThPo development instances are a different identity and may need Pop re-added. Fresh defaults do not overwrite loaded parameter values.

The build uses the pinned official API v14. Minimum physical firmware compatibility, loader/View info, target resources/cycles, DAC/receiver behavior, host lifecycle and musical audition remain unverified. Native callback execution and ARM inspection are not physical evidence.

See [development and verification](docs/development.md), the [claim audit](docs/FACT_CHECK.md), and the [operator qualification protocol](docs/target-qualification.md). No MIDI, endless cycle, forced onset, seed control or pattern bank is included.
