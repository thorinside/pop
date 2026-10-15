# Pop

Pop is a trigger-burst generator for the Expert Sleepers disting NT. One start produces several short trigger pulses for a drum, envelope or another trigger input. **It does not make audio.**

[Download v0.1.0-beta.1](https://github.com/thorinside/pop/releases/tag/v0.1.0-beta.1) · [User guide](docs/Pop-Manual.md) · [Manual PDF](https://github.com/thorinside/pop/releases/download/v0.1.0-beta.1/Pop-Manual-v0.1.0-beta.1.pdf)

The guide was rewritten on **15 October 2026**. It is a revised manual for the same plug-in release; the tag and binary assets have not changed.

## First patch

1. Put `pop.o` at `/programs/plug-ins/pop.o` on the NT's MicroSD card, restart, and add a new Pop instance. Use a preset containing only Pop for the first test.
2. Connect **NT Output 1** to a drum voice's **trigger input**. Connect the drum's audio output to your mixer or audio output.
3. Keep the defaults: **Internal**, **Count 4**, **Rate 20.0 Hz**, **Irregularity 0%**, **Flat**, **Contour amount 100%**. Burst output is **Output 1**, NT bus **13**.
4. On Burst, select **Manual Start: Off**, leave it briefly, then select **Fire**. You should hear four fast hits. Select Off before firing again.

If the hits blend together, set Rate to **5.0 Hz** to hear them separately. This manual test needs no input cable or external clock.

## External trigger and clock

For an external start, patch a trigger to **Input 1** and select it as **Trigger input**. Keep Timing Internal until this works.

**Clock sets the speed; Trigger requests a burst.** For a first clocked patch, use a continuous clock on Input 1, select Timing Clock and Clock input Input 1, choose Trigger input None, and use Manual Start. The guide gives complete settings and explains the first-hit delay.

If Clock and Trigger share an input, use **Retrigger Off**. With On, repeated clock pulses can keep replacing the waiting request and prevent output.

## Controls and compatibility

Count sets the number of hits. Duration sets a time window instead. Rate sets the starting spacing. Irregularity makes gaps less even. Contour changes speed through the burst; Contour amount sets the strength of that change. These controls change timing, not volume.

This is an experimental API v14 plug-in. Physical module qualification and shaped Clock timing equivalence remain incomplete. Its identity is **ThPp**; old **ThPo** development instances may need Pop re-added. See the guide's compatibility section for details.

Developer information: [build and verification](docs/development.md), [claim audit](docs/FACT_CHECK.md), [target qualification](docs/target-qualification.md). `make manual` renders the PDF from the maintained Markdown and print stylesheet.
