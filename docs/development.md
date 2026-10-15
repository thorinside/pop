# Pop development

## Current checkpoint

D2 extends reviewed Internal D1 `eb5d105` with functional Clock Count and Duration. Spec `2e5b0a88-17a2-4da5-a3fd-8ca7ab964469` uses accepted requirements artifact `b7200762-9537-43dd-93f2-d8cf02b7d6fb`, SHA-256 `2dff2c06aadbf384f478b2b682b8d63fd6d972375166822ed5652b859e3c38d8`. Owner amendments require ThPp, fresh Count4/Irregularity0/Flat/Amount100, a versioned manual and a verified experimental GitHub prerelease. The current candidate is not accepted or published. The publisher owns remote operations after independent exact-head review and acceptance. Agents do not access the module, SD card or presets.

**Target numerical equivalence remains UNVERIFIED.** Shaped Clock scheduling calls `exp2`. Native fixtures and exact rational comparisons do not prove that target libm chooses the same subdivision near every mathematical boundary. The owner's verification clarification `3640327d-0691-4529-a19d-490f20542708` permits practical review and an explicitly experimental prerelease without making an unavailable emulator an indefinite gate. It does not waive demonstrated defects, establish numerical equivalence, or qualify physical AC-08/AC-09. Compilation and the offline import gate are separate evidence.

## Build and verify

Use Clang C++17, Python3, cppcheck, matching LLVM clang-tidy/clang++, and arm-none-eabi-g++ with binutils. Document rendering uses Pandoc, Chrome and Poppler. Nothing installs to a module.

```sh
git submodule update --init distingNT_API
make fixtures-check
make clock-test
make test
make sanitize
make analyze
make hardware inspect desktop
make live perf
make manual
make release-package
make BUILD=build/reproduced hardware
cmp build/pop.o build/reproduced/pop.o
```

`BUILD` isolates compilation products. Tests write their raw CSVs to `build/`, so run normal and sanitizer tests sequentially and copy receipts before overwriting them. Full seed sweeps need adequate time. A timed-out process is not a passed test.

Native builds disable exceptions, RTTI and floating-point contraction and treat warnings as errors. ARM adds Cortex-M7 hard-float Thumb and `-fPIC`. The local observed compiler is Arm GNU15.3.Rel1. Hosted apt tool versions may differ, so compare bytes only within a recorded identical toolchain. CI builds twice and compares the actual objects.

The SDK is pinned to official commit `6975a630cb5f86de6f8c709f5467f4666bbd4579`, api.h SHA-256 `ec609acff01f5b4f2afcc09ee4af1ee45807f19324c98e503f31b51c2d7da411`. Official v14/64-bus constants govern the dated v13 writer guide and stale 28-bus step comment. Actual callback pointers and global `pluginEntry` define the loader contract, not old examples requiring every callback exported. `NT_setParameterFromAudio` and definition updates occur only in supported callback contexts. Deserialization retains settings for synchronization in the next `step`.

## State and bounded arithmetic

`pop_engine.*` owns the request snapshot, independent request/generation RNG streams, integer deadlines, pulse high/gap, ClockTracker and rational lattice. Phases separate Idle, Pending, Generating, FinalDraining, explicit-cancel Draining and AbortedDraining. FinalDraining retains activated-process clock ownership until the final pulse falls. Invalidity or expiry aborts that live tail, but not an already completed process, an unactivated Pending request or an unrelated explicit-cancel drain. A clock-abort high is physically retained but does not keep Active high or count as occupied. A fresh Off-policy request can wait during that drain. A separate high-origin flag prevents that new pending request from raising Active before activation.

Each sample applies reset/cancel, clock expiry, request timeout, Duration deadline and completion before clock observations and new starts. Invalid periods can abort before due-event admission. Accepted starts replace old due events. Clock activation requires an observed locked edge strictly later than the request, after the prior pulse and gap. No queued anchors, forced onset or recovery replay exist.

ClockTracker has Empty, First and Locked states. Periods below `ceil(Fs/20)` or above `2*Fs` discard the offending edge and all history. Locked validity expires at `last+3*P` before that sample's edge. Pending Clock requests expire after `6*Fs` samples. Internal ignores tracker validity.

The fixed clock search checks at most twelve power-of-two increments1..2048. It compares the unrounded length with `d*P/16`, then rounds the absolute timestamp, then independently checks physical separation. Clock base samples remain rational, including the lower clamp `Fs/100` and upper clamp `Fs`. Jitter is exactly `[400*2^24 + 3*Irregularity*(2*q-2^24)]/(400*2^24)`, where q is the RNG's upper24 bits. `pop_clock_math.h` multiplies these rational factors with the exact binary64 representation of the evaluated contour scale. Five fixed uint32 limbs compare cross-products without division, decimal-clamp roundoff or an epsilon. Internal timing retains its existing binary64 path.

For valid settings, base numerator is below2^36, jitter numerator below2^34, and the scale mantissa has53 bits. The numerator is below2^123. Scale in[0.5,2] requires a denominator shift of51..53. The denominator is below2^93, its product with `4*Fs` below2^127, and its largest candidate product below2^137. All fit160 bits, including numerator times16. Each limb multiply uses one uint32 product plus a limb and carry, at most2^64-1. Fixed five-limb/two-factor loops introduce no growing search, allocation or unsupported integer division. This proves exact comparison of the evaluated scale, not accuracy or host/target equality of `exp2` itself.

Nextafter tests select d2/d2/d4 at64 phases for both the original P/8 seam and the configured clamped equality. P4609 retains d1 despite alternating rounded gaps. P4593 promotes only when physical safety requires it. A separate Python Fraction oracle compares5760 production rational-selector rows, including tiny and maximum uint32 Fs, percentage jitter and adjacent binary64 scales.

For nonzero uint32 Fs, `P<=2*Fs<2^33`. A Count64 has `m<=64*2048=2^17`; `m*P+15<2^51`, within uint64 and exact binary64 integer representation. Clock Duration is at most `32*P<2^38`. Width and gap fit uint32. Pending and expiry offsets fit uint64. The maximum uninterrupted Clock Count bound is `64*(ceil(8*Fs)+1)+W`, below2^42 over this numerical domain. Absolute uptime must leave room for the relative offsets. Unlimited uptime or repeated-restart lifetime is not promised.

`pop_sample_math.h` splits words for binary64/sample conversion and decomposes Fs around1000 for milliseconds. This avoids unsupported `__aeabi_d2ulz`, `__aeabi_ul2d` and `__aeabi_uldivmod` without narrowing the supported sample-rate domain. All engine state is in requested per-instance memory. There are no queues, allocations, logging or I/O in production step.

## Independent expectations

The original three frozen Internal files and SHA256SUMS are unchanged. `fixtures-check` checks rather than regenerates them. New Clock fixtures live separately under `tests/fixtures/clock/`. `clock_reference.py` uses Decimal precision80, independent integer lattice arithmetic and its own xorshift. It imports no product routines. Do not regenerate goldens to conceal a failing product trace.

`clock_test` invokes the actual factory, parameters and step callback. It checks all four fractional P vectors, neighboring ratios, exclusive Duration deadlines, shared-input policies, default four-pulse behavior, saved settings, raw-enum rejection, no-replay aborts and20 seeded contour/termination vectors at three partitions. `POP_CLOCK_TRACE=1 build/clock_test` writes complete sample/input/output CSV receipts. `clock_boundaries_test` exercises the production scheduling boundary, tracker and lifecycle directly. It is not a cloned arithmetic helper.

`clock_corrections_test` retains the reviewed Fs32016/P2001/seed515213918 counterexample with rises2602,3602,3852 and fall4013. Neighbor Fs32015/32017, both clamp transitions, configured onset/Duration exclusion, and Count/Duration final-tail invalidity/expiry are separate regressions. Completion-first fall, Idle/Pending and explicit-cancel controls prevent broad abort ownership. `POP_CORRECTION_TRACE=1` prints full new Count/tail output rows for durable evidence.

Existing engine, adapter, rates, math and regression suites retain their Internal predicates. They cover all legal quarter-frame partitions, four native Fs values, exact pulses/Active/drain, restart boundaries, snapshots, load order and full seed1..1024 Internal distributions. Current Clock coverage does not yet claim a complete cross-mode boundary/statistical qualification.

The native host driver allocates the actual requested aligned memory and invokes the real exported factory with64 bus buffers. Its services and JSON parser are test shims, not firmware. It enforces audio setter context and captures draw calls. Test seed injection is absent from production objects and presets.

## Performance and object inspection

The claim is only a diagnostic native callback deadline check, not a speedup or target CPU acceptance. `pop_perf.cpp` interleaves30 empty-host/candidate pairs per scenario after100 warmup calls. Each side renders one simulated second in128-frame blocks. Setup, validation, telemetry and file writes are outside the timed callback. Idle, Internal activity, restart, safety, final drain, invalid routing, Clock activity, Clock replacement, loss and invalid periods have separate receipts and event assertions. C++ hot-path allocation counts must remain zero. This is not a complete malloc profiler; source and imports separately rule out heap paths.

Keep every block in `perf-blocks.csv` and every repetition in `perf.csv`. Report median/range/p95, event counts and outliers. The empty host has no Pop callback, so do not call it baseline Pop or calculate a speedup. A D1/D2 identical-driver comparison remains required before claiming a comparative result. Target cycles and a target-derived resource budget remain pending.

`make hardware` rejects the complete undefined-symbol set unless supported by `tests/fixtures/nt-beta18-exports.json`, including substantiated GOT/NT_globals loader specials. This is an offline image audit, not a statement about the flashed firmware. `check_arm_identity.py` parses ELF section/symbol tables and reads the real factory's GUID word. It requires ELF32LE ARM REL and exactly ThPp0x70506854, not a string match. Inspect global pluginEntry, sections, relocations, imports and requested memory separately. Dated ITC/DTC pool sizes are not guaranteed budgets.

Prior D1 isolated nt_emu exploration found APIv13 versus this plugin's v14 and missing GUI sources. No compatible emulator GUI run is claimed or unrelated checkout repaired. Native actual-adapter execution is not emulator, electrical or device proof.

## Manual, packaging and release

`VERSION` owns the intended version. Pandoc and `tools/render_manual.py` render exact source Markdown/CSS with an isolated temporary browser profile. Inspect extracted text and every rasterized page, including tables. Do not use a signed-in browser profile.

`tools/package.py` validates SDK, imports and actual factory identity, copies standalone pop.o, creates a deterministic ZIP whose only entry is `programs/plug-ins/pop.o`, verifies ZIP CRC/byte equality, and writes provenance plus SHA256SUMS. `release-package` requires the manual PDF. Local output is not proof of hosted publication or downloaded native blobs.

`.github/workflows/release.yml` verifies source/SDK/tag/version, runs native/static/sanitizer/live/perf, inspects ARM, renders the manual, rebuilds for equality and retains assets. A version tag publishes an explicitly experimental prerelease only after build success. The publisher may tag only after exact-head independent review and release-candidate acceptance under the owner's experimental-risk disposition. Keep unverified target numerical and physical qualification explicit in release notes. Workflow existence is not a successful hosted run.

The authorized publisher must verify repository identity/visibility and the exact reviewed release SHA, use the allowed PR/merge path, inspect the tag's peeled commit, wait for hosted CI, then download the actual release object/archive/PDF/checksums into a fresh directory and inspect their bytes, ELF identity and install layout. No tag overwrite, visibility change or native credential recovery is authorized. See `target-qualification.md` for the separate operator-owned physical record.
