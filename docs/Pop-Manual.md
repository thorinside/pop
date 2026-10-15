---
title: Pop
subtitle: Trigger bursts for disting NT
lang: en-GB
---

**User guide, revision 2 · 15 October 2026 · for Pop v0.1.0-beta.1.**
This is a revised manual for the same plug-in. The plug-in has not changed.

# What Pop does

Pop turns one start signal into several short trigger pulses. This group of pulses is called a **burst**. A **trigger pulse** is a brief voltage signal that tells another module to do something, such as play a drum sound or start an envelope. An external **clock** is a regular stream of pulses that can set the speed. Pop also works without one.

**Pop does not make audio.** Connect its Burst output to a trigger input. Listen to the sound made by the module or patch receiving those triggers.

With the factory settings, one start produces **four evenly spaced triggers**. They are close together, so a drum plays a short roll. You can change the number of hits, their spacing and how regular that spacing is.

# Installation

1. Download **pop.o** from the [Pop v0.1.0-beta.1 release](https://github.com/thorinside/pop/releases/tag/v0.1.0-beta.1). The PDF is the manual; the `.o` file is the plug-in.
2. Put `pop.o` on the NT's MicroSD card at:

   ```text
   /programs/plug-ins/pop.o
   ```

   Use your usual SD-card file transfer method or NT Helper.
3. Restart the NT so it loads the file you copied.
4. In **Add algorithm**, find **Pop**. If the NT first offers **Load**, load it, then add it to your preset.

For an update, replace the old `pop.o` with the intended release file and restart. Save your current preset before changing your setup.

# First patch: four hits

This example uses the NT, two patch cables and an **external drum voice**: a separate sound-producing module or patch with a trigger input. For this test, use an NT preset containing only a **new instance of Pop**. Saved instances may have different settings, and other NT algorithms can change the same outputs. An internal NT sound source needs a different bus-routing setup; it is not the setup used here.

## 1. Connect the sound

```text
NT Output 1          → drum voice trigger input
drum voice audio out → mixer or audio output
```

Do not listen to NT Output 1 as if it were Pop's audio output. It carries the trigger pulses. If an envelope starts your sound, connect Pop to that envelope's trigger input and use the voice's normal audio path.

## 2. Check the factory settings

Pop has three parameter pages: **Burst**, **Shape** and **Routing**.

| Page | Settings for this patch |
|---|---|
| **Burst** | **Termination: Count**, **Count: 4**, **Rate: 20.0 Hz** |
| **Shape** | **Irregularity: 0%**, **Contour: Flat**, **Contour amount: 100%** |
| **Routing** | **Timing: Internal**, **Burst output: Output 1** (bus **13**), **Burst mode: Replace** |

Leave the other settings at their defaults. This test needs **no input cable and no external clock**.

## 3. Start the burst

1. On the **Burst** page, select **Manual Start: Off** and leave it there briefly.
2. Change **Manual Start** to **Fire**.
3. You should hear four fast hits from the drum voice. Let the roll finish.
4. To play it again, select **Off**, then **Fire** again. Leaving Fire selected does not repeat the burst.

**Hz means “per second.”** At 20.0 Hz, the pulses are 50 milliseconds apart. The first pulse also waits 50 milliseconds; it is not immediate when you choose Fire.

If the four hits blend into one sound, shorten the drum's decay or set **Rate to 5.0 Hz**. At 5.0 Hz, the hits are 0.2 seconds apart and easier to hear separately. This is an optional slower setting; the factory default is 20.0 Hz.

# Starting from another module

Once the manual test works:

1. Connect a trigger source to **NT Input 1**.
2. On **Routing**, select **Trigger input: Input 1** (bus **1**). This is the default.
3. Keep **Timing: Internal** for now.
4. Send one trigger. Pop should play the same four-hit burst.

A trigger must go from low to high. A signal held high does not keep starting bursts; it must return low before the next trigger.

**Manual Start works with Trigger input set to None.** None means that no external start input is selected. Manual Start still uses the selected Timing mode, so it needs a running clock when Timing is Clock.

# Routing

Routing decides which NT connections Pop uses. NT also supports internal connections called **buses**. For the examples, use physical jacks and ignore the Aux buses.

| Physical jack | NT bus number | Default use |
|---|---:|---|
| **Input 1** | **1** | External start trigger |
| **Output 1** | **13** | Burst: the individual trigger pulses |
| **Output 2** | **14** | Active: a signal held high while the burst runs |

**Output 1 is bus 13, not bus 1.** If the screen shows bus numbers, use this table to match the setting to the jack.

| Routing control | What it does | Factory setting |
|---|---|---|
| **Trigger input** | Receives requests to start a burst. None disables the external start input. | Input 1 |
| **Timing** | Selects Pop's own timing or an external clock. | Internal |
| **Clock input** | Receives the running clock in Clock mode. | None |
| **Burst output** | Sends the individual trigger pulses. | Output 1 |
| **Burst mode** | Sets Add or Replace for Burst output. | Replace |
| **Active output** | Sends a gate while a burst runs. None disables this output. | Output 2 |
| **Active mode** | Sets Add or Replace for Active output. | Replace |

A **gate** stays high for a period of time. Active includes the wait before the first hit once the burst has begun. It does not send one trigger for each hit, and it stays low while a request is waiting for a clock. You do not need Active for the first patch.

Leave both output modes at **Replace** for these examples. Replace makes Pop set the signal on that bus. **Add** adds Pop's voltage to whatever is already there; use it only when you intend to combine signals.

Keep Burst and Active on different buses, separate from both selected input buses. Trigger and Clock may share an input, as explained under **Clock setup**. The routing selectors use the NT's available buses, numbered 1–64; Trigger, Clock and Active also allow None.

# Burst and Shape controls

## Count and Duration: how the burst ends

**Termination** chooses between two ways to finish. Its default is **Count**.

- **Count** sends a chosen number of triggers. Count 4 means four triggers if you let the burst finish. Shape and Irregularity change their timing, not that number.
- **Duration** allows triggers during a chosen length of time. It does not promise a particular number of hits. A short duration or slow rate may produce none. The time starts when the burst begins, after any wait for a clock.

| Control | Values | Default |
|---|---|---|
| **Count** | 1–64 triggers | 4 |
| **Duration**, Internal | 0.100–30.000 seconds, in 0.001-second steps | 0.500 seconds |
| **Duration**, Clock | 0.25, 0.5, 1, 2, 4, 8, 16 or 32 clock beats | 1 beat |

In Clock mode, **one received clock pulse counts as one beat**. If your source sends several pulses per musical beat, Pop counts those individual pulses.

Count is greyed out in Duration mode; Duration is greyed out in Count mode. Their values are kept. A restart, routing change or clock failure can interrupt a burst before it finishes.

## Timing and Rate: the spacing between hits

**Timing** is on **Routing**, directly above Clock input.

- **Internal** uses Pop's own timing. It works without an external clock. You still start each burst.
- **Clock** uses a running external clock to set the spacing. It also needs a start request.

**Rate**, on **Burst**, sets the starting spacing before Contour and Irregularity change it.

| Timing | Rate values | Default |
|---|---|---|
| **Internal** | 1.0–100.0 Hz, in 0.1 Hz steps | 20.0 Hz |
| **Clock** | /8, /4, /2, unity, x2, x4, x8 | unity |

In Internal mode, a lower Rate gives wider gaps. With Flat and 0% Irregularity, 5.0 Hz gives one hit every 0.2 seconds.

In Clock mode, **unity** means the same basic spacing as the clock. **x4** asks for four times that rate; **/2** asks for half that rate. At very fast or slow settings, Pop's timing limits can alter the result. If **Base limited** or **Interval limited** appears, try a less extreme rate.

Rate and Duration remember separate values for Internal and Clock timing. Their units change when you switch Timing.

## Irregularity: uneven spacing

**Irregularity** ranges from **0–100%**, with a default of **0%**. At 0%, there is no random variation. Increase it to make some gaps shorter and others longer. Try 30% on an eight-hit drum fill.

In Count mode, it does not randomly remove hits. Count 8 still sends eight triggers if the burst finishes. In Duration mode, the changing gaps can change how many hits fit before the time ends.

In Clock mode, Pop chooses gaps from halves, quarters and other straight divisions or multiples of the clock interval. These allowed gaps form the **clock grid**. Irregularity changes the chosen gap within that structure. It does not add triplets, dotted timing or off-grid timing variation. Small adjustments may choose the same gap.

## Contour and Contour amount: changing speed

**Contour changes timing, not volume.**

| Contour | What it does |
|---|---|
| **Flat** | No speed change from the contour |
| **Acceleration** | Starts slower and gets faster |
| **Deceleration** | Starts faster and gets slower |
| **Rise and fall** | Slower at the start and end, faster in the middle |
| **Fall and rise** | Faster at the start and end, slower in the middle |

**Contour amount** ranges from **0–100%**. At 0%, the contour has no effect. At 100%, it has its full effect. The defaults are **Flat** and **100%**. Flat stays flat at any amount.

Try Count 8 to hear the speed change over more hits. Acceleration and Deceleration have no effect at Count 1 or 2. In Clock mode, speed changes happen in steps because the gaps stay on the clock grid.

## Retrigger: starting again during a burst

**Retrigger**, on **Burst**, decides what another start does while Pop is busy. Its default is **On**.

- **Off:** new starts are ignored until the current burst, including any waiting time, has finished. They are not saved for later.
- **On:** a new start replaces the unfinished burst with a new one. It does not queue another burst. A pulse already high is allowed to finish before the replacement begins.

Use Off when you want a fill to finish. Use On when each new gesture should restart it. Repeated starts with On can keep replacing a waiting burst before it makes any hits.

Most musical settings are taken when you start a burst. Change them, then start again to hear the new settings. Retrigger responds immediately. Changing Timing or Routing stops the current process, so set those before starting.

# Clock setup

**Clock sets the speed. Trigger requests a burst.** A clock connected only to Clock input does not start bursts by itself.

Keep the drum wiring from the first patch. Use a clock sending **one pulse per beat at 120 BPM** (120 beats per minute). That is two pulses per second.

1. Connect the clock to **NT Input 1**.
2. On **Routing**, select **Timing: Clock**, then **Clock input: Input 1** (bus 1).
3. Set **Trigger input: None**. You will use Manual Start, so the clock is not also treated as a start trigger.
4. On **Burst**, set **Termination: Count**, **Count: 4**, **Rate: x4**, **Retrigger: Off**.
5. On **Shape**, set **Irregularity: 0%**, **Contour: Flat**.
6. Let at least two clock pulses arrive. Select **Manual Start: Off**, leave it briefly, then select **Fire**.

Pop waits for a clock pulse after your start request, then waits its first normal gap. In this patch, hits are 125 milliseconds apart. The first hit is 125 milliseconds after that later clock pulse, not immediately when you choose Fire.

For an external start button or sequencer trigger, connect it to **NT Input 2** and set **Trigger input: Input 2** (bus 2). Keep the continuous clock on Input 1.

Keep the clock running, with pulses **0.05–2 seconds apart**. If it stops or becomes invalid, Pop cancels the burst. Restore a steady clock and start again; the old burst does not resume. A burst keeps the clock speed measured when it begins; valid tempo changes affect later bursts.

## Sharing one input for Clock and Trigger

You can select the same input for Clock input and Trigger input. Set **Retrigger: Off** for this patch.

The first eligible pulse requests a burst. Later pulses keep supplying timing, while their extra start requests are ignored until Pop finishes. A later pulse can then request another burst.

With **Retrigger: On**, every shared pulse can restart the waiting request. It may never get far enough to produce a hit. If this patch is silent, use Retrigger Off or separate inputs.

# Examples

**Four separate hits:** Internal, Count 4, Rate 5.0 Hz, Flat, Irregularity 0%. Use Manual Start. This checks the wiring before faster rolls.

**A speeding-up drum fill:** Internal, Count 8, Rate 10.0 Hz, Acceleration, Contour amount 100%, Irregularity 0%, Retrigger Off. Then try Irregularity 30% for uneven gaps. The count stays eight.

**A timed hi-hat fill:** Internal, Termination Duration, Duration 2.000 seconds, Rate 20.0 Hz, Irregularity 30%, Flat, Retrigger Off. Each burst can add hits for two seconds; the number of hits can vary.

# Troubleshooting

First try **First patch: four hits**. For a slower test, change only Rate to 5.0 Hz. If that works, add the external trigger or clock afterward.

| Problem | Check |
|---|---|
| **No sound** | Pop makes triggers, not audio. Send Burst to a drum or envelope trigger input, and listen to that voice's audio output. |
| **No triggers at the expected jack** | Match Burst output to the jack. Output 1 is bus 13; Input 1 is bus 1. Use Burst, not Active, for individual hits. |
| **Manual Start works only once** | Select Off and leave it briefly before selecting Fire again. Holding Fire does not repeat. |
| **Four hits sound like one** | The default 20.0 Hz is a fast roll. Try Rate 5.0 Hz and a shorter decay on the drum voice. |
| **External trigger does nothing** | Match Trigger input to the cable's NT input. Let the signal return low between starts. |
| **Clock mode is silent** | Select Clock input, keep a valid clock running, wait for two pulses, then request a burst. A clock alone is not a start request. |
| **Clock and Trigger share an input** | Use Retrigger Off. On can restart the waiting request on every pulse and prevent hits. |
| **“No clock input”** | Timing is Clock but Clock input is None. Select the connected clock input. |
| **“Waiting for clock” or “Clock wait timeout”** | Check the cable and clock speed. Once it is steady, select Manual Start Off, then Fire again. |
| **“Clock stopped or invalid”** | Restore a steady clock with pulses 0.05–2 seconds apart, then send a new start. |
| **“Invalid routing”** | Keep Burst and Active on different buses, separate from both selected inputs. To edit a greyed-out Clock input, temporarily select Timing Clock. Trigger and Clock may share an input. |
| **A control is greyed out** | Count is inactive in Duration mode; Duration is inactive in Count mode. Clock input is inactive with Internal timing. |
| **Short Duration gives no hits** | The first wait may not fit. Increase Duration or Rate, or use Count for a set number of hits. |
| **An edited setting has no effect yet** | Start again. Most musical settings do not change a burst already in progress. |
| **Rate differs from the selected value** | Contour and Irregularity change gaps. For a timing-limit message, try a less extreme rate. |
| **It works only in a new preset** | Check other algorithms using the same output, and settings saved in the old instance. |
| **Pop is missing or fails to load** | Check the `.o` file and folder, restart, and inspect **Misc → Plug-ins → View info…**. Keep the exact error when reporting a problem. |

# Compatibility and technical notes

- This is **Pop v0.1.0-beta.1**, an experimental release. Physical firmware compatibility, output levels, receiver response and target performance have not been fully qualified. Matching shaped Clock timing between desktop tests and the module also remains unverified.
- The host must load **disting NT API v14** plug-ins. An exact minimum firmware version is not established here. See the [official NT manual and firmware page](https://www.expert-sleepers.co.uk/distingNTfirmwareupdates.html) for host installation procedures.
- Pop's identity is **ThPp**. Earlier development builds used **ThPo**; old instances may need Pop re-added. New defaults do not overwrite saved settings in an existing instance.
- Burst pulses contribute nominally **+5 V for 5 milliseconds**, with at least **1 millisecond low** before the next pulse. Times are rounded up to whole audio samples. Add mode combines this with the bus's existing voltage.
- External inputs re-arm at **1 V or below** and trigger at **2 V or above**. A low-to-high 0–5 V trigger meets those thresholds.
