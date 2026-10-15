# Pop

Pop turns a trigger into a finite, organically spaced burst for disting NT. Choose an exact **Count**, or choose a **Duration** window and let the event count vary. Burst and Active contribute either 0 V or +5 V. A start is not copied to Burst. The first event waits for the same timing process as later events.

**Development checkpoint D1. Internal Count and Duration are implemented. Clock controls are reserved for the next slice and do not generate bursts yet. This is not a release or a hardware-tested build.** The build targets API v14. Minimum compatible firmware, physical loading, output levels and musical audition have not been verified. Do not install this checkpoint on a module as an accepted release.

## Try Internal timing

1. Keep Timing at Internal. Patch a trigger into Input 1 and route Output 1 to a receiver that accepts triggers.
2. Burst defaults to host bus 13 (Output 1). Active defaults to bus 14 (Output 2). Keep both separate from the inputs and from each other.
3. Set Count to 4, Rate to 10.0 Hz, Contour to Flat and Irregularity to 0%. Give the trigger a low level before its rising edge. In native tests at 48 kHz the events begin 100, 200, 300 and 400 ms after the request.
4. For a fixed window, choose Duration. At 10 Hz, Flat, 0% irregularity and 0.200 seconds, there is one event at 100 ms. There is no event at the 200 ms deadline.
5. For Manual Start, select Off, then Fire. Leaving it at Fire does not repeat. A held or saved Fire after load/reset does nothing until Off has been observed.

The trigger detector arms at or below 1 V and fires at or above 2 V. It retains its state between those thresholds. Non-finite samples neither fire nor rearm. The first high after construction/reset does not fire without a preceding low. These are software thresholds, not measured electrical tolerances.

## Controls

All controls use normal NT parameter pages and mappings. Burst, Shape and Routing pages keep standard parameter navigation.

| Control | Range | Default | Meaning |
| --- | --- | --- | --- |
| Termination | Count / Duration | Count | Applied at the next accepted request. |
| Count | 1..64 | 8 | Actual rising edges, including the first event. Grayed out in Duration. |
| Duration, Internal | 0.100..30.000 seconds, 0.001-second steps | 0.500 s | Admission window. Grayed out in Count. The final high may end later. |
| Rate, Internal | 1.0..100.0 Hz, 0.1-Hz steps | 20.0 Hz | Base interval before contour, irregularity and pulse-separation limiting. |
| Irregularity | 0..100% | 50% | Varies intervals directly. Does not randomly delete events. Zero is deterministic. |
| Contour | Flat / Acceleration / Deceleration / Rise and fall / Fall and rise | Deceleration | Activity direction, not an envelope output. |
| Contour amount | 0..100% | 75% | Zero flattens every contour. |
| Retrigger | Off / On | On | Live request policy, including during waits and final drain. |
| Timing | Internal / Clock | Internal | Clock is **not implemented in D1**. |
| Trigger input | None or bus 1..64 | 1 | None still permits Manual Start. |
| Clock input | None or bus 1..64 | None | Grayed out and ignored for Internal timing. Still cannot alias an output. |
| Burst output | Bus 1..64 | 13 (Output 1) | 0/+5 V contribution. |
| Burst mode | Add / Replace | Replace | Add does not limit the final shared bus voltage. |
| Active output | None or bus 1..64 | 14 (Output 2) | Process-active gate, not a contour or Done pulse. |
| Active mode | Add / Replace | Replace | Independent of Burst mode. |
| Manual Start | Off / Fire | Off | One observed Off-to-Fire action. |

The reserved Clock view retains a separate Rate (-3..+3, /8..x8, unity at 0) and Duration (0.25, 0.5, 1, 2, 4, 8, 16, 32 beats, default 1). Switching Timing restores each mode's saved settings. These controls describe the accepted future Clock interface, not functioning Clock timing in this checkpoint. Clock requests report No clock input or Clock not implemented (D1).

## Timing and contours

Count distributes contour progress over its generated waits. Duration uses elapsed window progress when scheduling each wait. Acceleration shortens intervals; Deceleration lengthens them. Rise and fall is sparser at the edges and denser near the middle; Fall and rise is the inverse. At small Counts a complete audible contour arc may not be represented. N1 and N2 use neutral progress for Acceleration and Deceleration.

At 48 kHz each Burst high lasts 240 samples (5 ms) and each separating low is at least 48 samples (1 ms). Width and gap use ceiling conversion at the actual sample rate. Fast requested intervals may be lengthened to preserve separation; the display reports Interval limited. The onset wait does not receive an unnecessary 6 ms floor when there is no preceding pulse.

Duration admits only events strictly before its deadline. It can produce zero events. It never shortens a final interval to squeeze in an event, and any high already emitted finishes its full width. Count ends at its last pulse fall. Active is high from activation through generation/final drain, but low during gap waiting with no old high. Active can therefore be low while the engine is occupied.

## Restart, settings and safety

With Retrigger Off, occupied starts are consumed and ignored without a queue or RNG change. With On, the latest start replaces the remaining process. An existing high and its required low gap finish before replacement activation. The new burst starts its own contour and generated onset wait. Old pulses do not count toward the replacement Count.

Count, Duration, Rate, Irregularity, Contour, amount and termination mode are held for the accepted request. Editing them affects the next accepted request. Retrigger remains live. Manual and external starts observed on the same sample collapse to one request.

Timing/routing/output-mode changes, a recognized preset load, reconstruction or an observed sample-rate change clear generation and reset input/action arming. These safety resets may truncate a high. An in-place load preserves the advancing request RNG stream. Presets save retained control settings, not timing or RNG state. Normal reconstruction seeds from the host cycle counter; this is not a verified entropy source and does not promise unique patterns. There is no player Seed or repeat-pattern control.

Burst and Active may not share a bus or alias either selected input. Trigger and Clock may share an input. Invalid routing rejects starts and reports Invalid routing. None never indexes a bus. Replace writes this algorithm's contribution; Add adds it. Other algorithms can overwrite or add to shared buses, so physical pulse levels/separation require appropriate dedicated routing.

The status display keeps standard parameter navigation and reports Idle, waiting for gap, Active or Draining, plus termination mode, errors and limiting. Limiting stays visible until the next accepted request or safety reset. A display refresh is not sample-exact timing evidence.

## Verification limits

Native tests drive the actual exported factory, callbacks, parameter definitions and sample buffers. ARM object inspection is separate from loading a module. Physical firmware compatibility, loader/View info, target-cycle/memory budget, DAC/receiver behavior, bypass/re-enable behavior and Neal's audition remain pending. No physical result is inferred from a native voltage trace.

Developer build and verification instructions are in [docs/development.md](docs/development.md).
