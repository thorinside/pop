---
title: Pop
subtitle: A Brief Disturbance in Time
lang: en-GB
---

> **v0.1.0-beta.1 candidate. Not yet a published or physically qualified release.** Native Clock behavior is implemented. Host/ARM shaped-Clock numeric equivalence, complete final qualification and publication remain open. Do not infer those results from this manual.

Pop makes finite bursts of gates for the Expert Sleepers disting NT: four tidy taps, an accelerating rattle, a small shower of events, or something that appears to have escaped a balloon with a timetable.

It does not make an audio voice. Patch its **Burst output** to something that responds to triggers: a drum, an envelope, a switch, a sequencer, or another suitably excitable destination.

# Four things that will happen

Start here. No external clock is required.

1. Add **Pop** to the NT and leave **Timing** at **Internal**.
2. Patch your trigger to **Input 1**. **Trigger input** defaults to Input 1.
3. Patch **Output 1** to a drum or envelope trigger input. **Burst output** defaults to Output 1.
4. Use the fresh-instance settings: **Termination: Count**, **Count: 4**, **Irregularity: 0%**, **Contour: Flat**, **Contour amount: 100%**.
5. Send one trigger. It must first be low, then rise. You should get four evenly spaced output gates.

The default Internal Rate is **20.0 Hz**. With those settings, the four rising edges occur **50, 100, 150 and 200 ms after the accepted start**. Each gate lasts 5 ms. Pop deliberately does **not** copy the incoming trigger to the output, so the first gate is not immediate.

For a manual test, use **Manual Start**: let the NT observe **Off**, then select **Fire**. Return it to Off before firing again. Leaving Fire selected does not make it a small, determined machine gun.

If you want the first burst to finish despite further triggers, set **Retrigger: Off**. The fresh-instance default is On, which allows a new trigger to replace the burst already in progress.

**Flat with 100% contour amount is still flat.** Amount controls the strength of a chosen shape; it does not invent one. Irregularity remains off until you turn it up.

# A short arrangement with time

Two choices describe a burst:

- **Termination** decides how it ends: a number of events, or a window of time.
- **Timing** decides how the events are spaced: Pop's Internal timebase, or an external Clock.

These are separate choices. Count works with either timing source, and so does Duration.

## Count: an appointment book

Count emits the selected number of actual output gates, then finishes. Irregularity and contours can move those gates around in time, but do not randomly delete them. The burst may take longer or shorter; an uninterrupted Count burst still emits its count.

A restart, safety reset or detected clock failure can interrupt that process. Count is not a promise to complete an abandoned burst after the circumstances have changed.

## Duration: closing time

Duration admits events inside a fixed window. It does not promise a particular event count. Different irregular patterns can produce different counts, and a short window can legitimately produce none.

An event scheduled exactly at the deadline is outside the window. Pop does not squeeze it in, shorten the preceding wait, or save it for later. A gate that already started is allowed to finish its full width.

In Internal timing, Duration is in seconds. In Clock timing, it is in incoming-clock beats. **One incoming pulse is one beat** for this purpose; Pop does not assume a particular PPQN convention.

# Installing a small disturbance

Download the **`pop.o` asset from the intended tagged GitHub release**. It is the NT plug-in object, not a desktop library or a source archive.

Place it on the MicroSD card at:

```text
/programs/plug-ins/pop.o
```

You can transfer the file using your usual SD-card method or NT Helper. The NT scans plug-ins at startup and when the card is remounted. **Misc → Plug-ins → View info…** shows scan results. In **Add algorithm**, a plug-in may initially offer **Load**; load it, then add Pop. See the official NT manual's Plug-ins chapter for the procedure applicable to your firmware.

For an update, retain a copy of your preset and replace the plug-in file with the intended release asset. Restarting provides a clear way to load the new file rather than continue using an already-loaded copy.

## Four letters that matter: `ThPp`

Pop's algorithm identity is **`ThPp`**, with exactly that capitalization. Future compatible Pop updates keep it.

Earlier development builds used **`ThPo`**. That is a different identity. Presets containing those development instances may need Pop re-added and their settings recreated. There is no automatic identity migration to assume.

The new simple defaults apply to **new instances**. Loading existing saved parameter values does not intentionally replace them with those defaults. Keep a record of development settings before changing identities.

# The three pages

| Page | Controls, in page order |
|---|---|
| **Burst** | Termination, Count, Duration, Rate, Manual Start, Retrigger |
| **Shape** | Irregularity, Contour, Contour amount |
| **Routing** | Trigger input, **Timing, Clock input**, Burst output, Burst mode, Active output, Active mode |

Timing sits immediately above Clock input. The clock socket has not moved; the explanation has.

Inactive controls retain their values. Count is inactive in Duration mode; Duration is inactive in Count mode. Clock input is inactive in Internal timing. That is deliberate, not a request to find a larger clock.

## Every parameter

### Burst

| Parameter | Range and fresh default | Operation |
|---|---|---|
| **Termination** | Count / Duration; **Count** | Selects counted events or a time window. |
| **Count** | 1–64; **4** | Number of gates in an uninterrupted Count burst. |
| **Duration** | Internal: 0.100–30.000 s, 0.001 s steps; **0.500 s**. Clock: 0.25, 0.5, 1, 2, 4, 8, 16, 32 beats; **1 beat** | Window used in Duration mode. Internal and Clock values are retained separately. |
| **Rate** | Internal: 1.0–100.0 Hz, 0.1 Hz steps; **20.0 Hz**. Clock: /8, /4, /2, unity, ×2, ×4, ×8; **unity** | Base spacing before contour, irregularity, clock-grid and safety limits. Internal and Clock values are retained separately. |
| **Manual Start** | Off / Fire; **Off** | One observed Off-to-Fire action requests one burst. |
| **Retrigger** | Off / On; **On** | Off consumes and ignores starts while occupied. On replaces the current or waiting request. |

### Shape

| Parameter | Range and fresh default | Operation |
|---|---|---|
| **Irregularity** | 0–100%; **0%** | Varies intervals directly. Zero removes random variation; a shaped contour still changes spacing. |
| **Contour** | Flat, Acceleration, Deceleration, Rise and fall, Fall and rise; **Flat** | Shapes activity through the burst. |
| **Contour amount** | 0–100%; **100%** | Strength of the contour. Zero removes contour shaping; Flat remains flat at any amount. |

### Routing

| Parameter | Range and fresh default | Operation |
|---|---|---|
| **Trigger input** | None or a host bus; **Input 1** | Reads burst-start edges. None still permits Manual Start. |
| **Timing** | Internal / Clock; **Internal** | Selects the finite burst's timebase. Internal is not an endless free-running cycle. |
| **Clock input** | None or a host bus; **None** | Supplies timing in Clock mode. Select an input before requesting a clocked burst. |
| **Burst output** | A host bus; **Output 1** | The individual output gates. |
| **Burst mode** | Add / Replace; **Replace** | Adds Pop's contribution or replaces the selected bus. |
| **Active output** | None or a host bus; **Output 2** | Indicates the active burst process. None disables routing, not the process itself. |
| **Active mode** | Add / Replace; **Replace** | Bus ownership for the Active gate. |

The NT has **64 logical buses**: Inputs 1–12, Outputs 1–8 and Aux buses 1–44. These are not 64 physical sockets. Output 1 is host bus 13; Output 2 is host bus 14.

Keep Burst and Active on different buses, and keep output buses separate from the selected input buses. Trigger and Clock inputs **may** share a bus; the next section explains the consequences.

# A clock is not an invitation

**Clock supplies the timing. Trigger asks Pop to begin a burst.** A clock connected only to Clock input does not, by itself, request a burst.

## A reliable first clocked patch

1. Send a continuous clock to **Input 1**.
2. Set **Timing: Clock**, **Clock input: Input 1**.
3. Use a separate trigger on **Input 2**, with **Trigger input: Input 2**. Alternatively select Trigger input None and use Manual Start.
4. Select **Count: 4**, **Rate: ×4**, **Irregularity: 0%**, **Contour: Flat**, **Retrigger: Off**.
5. Let at least two clock edges arrive, then send a trigger or use Off → Fire.

Pop needs two fresh edges to establish a period. An accepted request starts its process on an eligible **later** clock edge, after any outstanding gate and low gap have cleared. A trigger arriving exactly on a clock edge does not turn that edge into the burst's anchor.

There is then a generated first wait. With a steady 120 BPM quarter-note clock, Flat, no irregularity and ×4, the first gate is 125 ms after activation. It is not copied from the trigger or the activation edge.

The straight Clock choices use a power-of-two timing grid. There is no triplet or dotted setting. Contour and irregularity select timing on that grid; limiting can also affect spacing.

## One cable doing two jobs

You can select the same input for Trigger and Clock. For a useful starting point, choose **Retrigger: Off**.

- The first eligible input edge requests a burst.
- Further edges provide clock timing while the occupied request ignores extra starts.
- After completion, a later edge can request another burst.

This is not a promise of one complete burst per input edge. Acquisition, activation, the requested count or duration, and the final gate all take time.

With **Retrigger: On**, each shared edge also replaces the waiting request. Repeated edges can keep replacing it before an eligible activation, so nothing gets as far as an output gate. For clear musical control, use a continuous clock and a separate occasional trigger, or Manual Start.

A committee cannot finish a timetable if every tick appoints a new committee.

## Waiting, tempo changes and clock stop

A request made before clock acquisition can wait up to **6 seconds**. If it times out, send a fresh request after the clock is established. Clock input None rejects a clocked start.

Accepted clock periods are **50 ms to 2 seconds**. A burst captures its period at activation. Later valid tempo changes update the tracker for future bursts rather than stretch the current burst's schedule or duration.

Clock validity expires after **three periods** without a valid edge; an invalid observed period can also stop the active clocked burst. On detection, future events are discarded and Active goes low. Any Burst gate already high finishes its full width. Recovery does not resume the abandoned burst: reacquire a valid clock and send a fresh start.

# Shape, without changing the height of the furniture

Contours change **when gates happen**, not their voltage or width.

| Contour | Activity |
|---|---|
| **Flat** | No contour-shaped change in base spacing. |
| **Acceleration** | Starts more slowly and becomes busier. |
| **Deceleration** | Starts busier and becomes more sparse. |
| **Rise and fall** | Sparse ends, busy middle. |
| **Fall and rise** | Busy ends, quieter middle. |

In Count mode, the contour progresses through the requested events. In Duration mode, it progresses through elapsed window time. Very short Count bursts have little room for a grand dramatic arc.

**Irregularity** spreads the intervals around that shape. It is not a probability control that throws away candidate pulses. Count still counts emitted events; Duration can produce a different total for different patterns.

Every accepted start or restart advances the instance's pattern stream. Identical-looking bursts can still occur by chance. Automatic seeding uses a host cycle counter, not a verified entropy source. There is no player Seed or repeat-pattern control. Ignored triggers do not advance a replacement pattern or queue work for later.

Choose a contour and increase Irregularity after the regular patch works. This makes it easier to distinguish intended uncertainty from an incorrectly connected cable.

# What happens when you ask again

**Occupied** includes waiting for activation, generation and the final gate draining. It is not the same thing as Active being high.

| Retrigger | A new start while occupied |
|---|---|
| **Off** | Consumed and ignored. No queue, no delayed surprise and no change to the current pattern. |
| **On** | Replaces the remaining process with the latest request. An existing high gate finishes, then the required low gap clears before the replacement can activate. |

Old gates do not count toward the replacement Count. Further starts replace the pending request; they do not build a queue.

Retrigger is observed live. Changing On to Off does not erase work already accepted. Changing Off to On does not replay starts previously ignored.

The burst settings are sampled at each accepted start. Changing Count, Duration, Rate or Shape during a burst affects the next accepted request. Timing and routing changes are safety changes: they clear the process and require a fresh start. Preset loading or a recognized sample-rate change also clears activity. Pop does not save a half-finished burst and resume it from a preset.

# Two outputs, no committee required

## Burst

Each Pop gate contributes **0 V or +5 V**, with a **5 ms high rounded up to whole samples**, and at least **1 ms low rounded up to whole samples** before another rise. At 48 kHz these are 240 high samples and at least 48 low samples. Pop limits timing where needed to keep distinct, correctly formed gates.

In **Replace**, Pop owns the selected bus. In **Add**, its contribution is added to what is already there. A shared Add-mode bus is not guaranteed to have a final voltage of only 0 V or +5 V.

## Active

Active rises when the process activates, including the wait before its first generated gate. It normally remains high through generation and the final gate draining. Waiting for clock or a pending activation alone is low. A musical restart can retain Active while the old high drains, then leave it low during the remaining wait.

Detected clock failure is different: Active falls immediately on detection while a Burst gate already high is allowed to finish. Active is a process gate, not a contour envelope or a separate Done pulse.

The 5 ms width and nominal +5 V contribution describe the algorithm. Physical output levels and receiver tolerances belong to the hardware qualification record for the release.

# A few useful disturbances

## A civilised drum fill

Use Internal Count 4, Rate 20 Hz, Flat and Irregularity 0%. Send Burst to a drum trigger. Use a separate performance trigger or Manual Start. Retrigger Off lets the fill finish; On lets a new gesture replace it.

## An increasingly urgent meeting

Use Count 8, Acceleration, Contour amount 100% and a moderate Internal Rate. Begin with Irregularity 0%, then add a little. Send Burst to a noise envelope or percussion voice. The notes gather pace; nobody needs to raise their voice.

## A busy middle

Use Duration 2 seconds, Rise and fall, Contour amount 100%, Rate 20 Hz and Irregularity around 50%. Send Burst to a hi-hat or small percussion voice. The window stays fixed while the event count varies. Try Fall and rise for the opposite distribution.

## A clocked handful

Use the separate-clock patch above: Count 4, Clock ×4, Flat and no irregularity. Trigger it occasionally over a steady beat. Then try a contour or a little irregularity. The grid remains straight even when the arrangement becomes less punctual.

## A gate for the whole affair

Send Active to a VCA or an external envelope while Burst excites a percussion source. Remember that Active includes the pre-first-event wait after activation. It is a useful process signal, not a substitute for a shaped audio envelope.

# If the universe remains silent

| Symptom | Check |
|---|---|
| **No audio** | Pop produces gates. Route Burst to a drum or envelope trigger, not directly to an audio input expecting a voice. |
| **No burst in Clock timing** | Select Clock input, provide valid edges, and send a separate start. A clock alone is only a timebase. |
| **Both inputs share a bus and nothing starts** | Try Retrigger Off, or use separate clock and trigger sources. Restarting the pending request on every edge can prevent activation. |
| **An input stays high** | Starts are edges, not levels. Let it fall to 1 V or below before a rise to 2 V or above. A held high does not repeatedly start bursts. |
| **Manual Start does nothing** | Let Off be observed before selecting Fire. A saved or held Fire is not a new action. |
| **It stopped after the clock disappeared** | That is cancellation, not pause. Restore a valid clock and send a fresh start. |
| **A short Duration emits nothing** | The first generated wait may not fit inside the window. Increase Duration or Rate, or use Count when event count matters. |
| **A control is inactive** | Count and Duration are mutually applicable; Clock input is inactive in Internal timing. Values are retained. |
| **Changing settings seems to do nothing** | Most burst settings are held from the accepted start. Trigger a new burst after editing. |
| **Invalid routing** | Keep outputs separate from each other and the selected inputs. Check the intended physical or internal bus. |
| **Fewer old-burst gates than expected** | A restart, safety reset or clock failure can abandon the old process. There is no backlog. |
| **ClockIncomplete on an early development build** | That build did not implement Clock generation. Use the functional tagged release; rewiring does not implement missing code. |
| **Unresolved symbol / plug-in fails to load** | Verify the exact release asset and required host API. Preserve the literal error and release version when reporting it. This is a binary/firmware issue, not a trigger cable diagnosis. |
| **A development preset cannot find Pop** | Check the identity change from ThPo to ThPp. You may need to re-add Pop. |

If several starts arrive together, a simultaneous external edge and Manual Start collapse to one request. If a safety change has just cleared the process, send a fresh edge after the new configuration is established.

# Compatibility: evidence before optimism

The release build uses the pinned official **disting NT API v14**. The host must provide the required API and symbols. The release's build record identifies the SDK, compiler, object digest and firmware export set used for the offline check.

Native engine and actual-adapter tests, ARM object inspection and firmware-symbol checks establish different things. They do not, by themselves, establish physical DAC levels, trigger-receiver tolerances, target CPU limits or every firmware/host transition.

Consult the release notes for the exact hardware observations and remaining qualification. An earlier development object was reported to load and run; do not turn that into a claim that every later release, firmware or patch has been tested on the module.

For the host's own file-loading and firmware procedures, use the [official NT manual and firmware page](https://www.expert-sleepers.co.uk/distingNTfirmwareupdates.html). The [official C++ API](https://github.com/expertsleepersltd/distingNT_API) is the development reference.

Pop is a finite disturbance. The rest of the patch is your department.
