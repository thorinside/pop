# Pop development

## Authority and checkpoint

D1 implements the Internal vertical slice of accepted Spec `2e5b0a88-17a2-4da5-a3fd-8ca7ab964469`. Requirements artifact `b7200762-9537-43dd-93f2-d8cf02b7d6fb` has SHA-256 `2dff2c06aadbf384f478b2b682b8d63fd6d972375166822ed5652b859e3c38d8`. Reviewed plan `13732dd2-c86e-41c1-9da4-1f62bb485bed` and ticket `a06ee2de-31df-4047-a845-5a7e9a5f5351` bound this slice. D2 owns Clock acquisition, rational-lattice scheduling, clock loss, final CI/package work. D3 is read-only assembled acceptance. Neither compilation nor a D1 self-check closes these later gates.

No push, PR, merge, release/tag, deployment or SD/device installation is part of D1. Final independent exact-head review is a separate workflow. Do not resolve the ticket from this document.

## Build

```sh
git submodule update --init distingNT_API
make test
make sanitize
make analyze
make hardware inspect
make desktop
make live
make perf
```

Use Clang C++17 for native tests, Python 3 for the analysis compile database and frozen reference tooling, cppcheck, LLVM clang-tidy and arm-none-eabi-g++/binutils. `CLANG_TIDY` and `ANALYSIS_CXX` may be overridden together. Native flags include strict warnings as errors, no exceptions/RTTI and no floating-point contraction. ARM also uses Cortex-M7 hard-float Thumb and `-fPIC`. Build output stays in ignored `build/`; use `BUILD=...` for separate compilation outputs. Test receipts intentionally use `build/` so normal and sanitizer receipts can be compared.

The SDK is pinned at official commit `6975a630cb5f86de6f8c709f5467f4666bbd4579`. Header SHA-256 is `ec609acff01f5b4f2afcc09ee4af1ee45807f19324c98e503f31b51c2d7da411`. Actual v14 constants define 64 buses. The header's step comment saying 28 buses and the installed writer guide's v13 baseline are dated. Only `pluginEntry` must be globally exported for loading; callbacks are factory pointers. The API page struct includes group and padding, unlike some old examples.

`make analyze` compiles both production translation units with the LLVM compiler corresponding to clang-tidy, emitting its real compilation database. Mixing AppleClang's `-MJ` vendor flags with Homebrew clang-tidy does not work. The explicit analyzer/performance checks ignore SDK-header enum-size suggestions because changing official enum representation would alter ABI. cppcheck suppressions cover only the official JSON stream's non-explicit constructor and the required non-const draw callback signature. No product warning is globally disabled.

## State and ownership

`pop_engine.*` owns musical state, integer deadlines, pulse high/gap and request/generation RNG streams. `Phase` separates Idle, Pending, Generating and Draining. Occupancy is not Active-high. Each sample applies cancellation/reset, completion/deadline, accepted starts, activation, due events and rendering in contract order. There is at most one rise per sample, one RNG draw per scheduled wait and no catch-up loop.

`pop_plugin.cpp` validates host routing before pointer arithmetic, snapshots controls at accepted starts, manages mode-retained definitions with a recursion guard and the audio setter, and renders Add/Replace contributions. State and mutable parameter definitions are in requested per-instance SRAM. There are no persistent workBuffer or mutable RNG globals. Const metadata can occupy relocation-bearing data sections in an ARM object; section names alone do not imply mutable product state.

The host driver allocates exactly the requested aligned memory and invokes actual `pluginEntry`, factory, callbacks and `step` with 64 bus buffers. Host service implementations are narrow test shims, including recursive parameter setters and JSON settings storage. They do not emulate firmware. The test seed seam is compiled only with `POP_TESTING`; it is absent from production objects and presets. The host shim exposes firmware's mutable globals through its official const view so tests can drive observed Fs changes.

GUID `ThPo` uses the thorinside namespace. D1 searched the available local `nosuch` C++ trees before choosing it and tests the actual factory. A local search is not proof of an exhaustive worldwide GUID registry; recheck available community indexes before release.

## Independent expectations and receipts

`tests/fixtures/PROVENANCE.md` records the oracle source and hashes frozen in the commit before production logic. Tests consume literals; they never regenerate them. Decimal reference code imports no product routines. The product's binary64 shaped timing is compared to independent high-precision fixture results. Width/gap integer arithmetic stays exact. Cross-toolchain transcendental results may differ by one Internal sample under R4; this is not Clock numeric proof. D2 must prove Clock tick selection on target-compatible arithmetic or execution, not only compile it.

Core tests cover frozen N1/N2/N4/N64, contour and seeded traces, pulse floors, cancellation, restart matrices, count/duration bounds, Schmitt state and statistics. Adapter tests cover the same goldens at partitions 4/28/64/128, real buffers, manual actions, routing, pages, retained settings, preset load, safety changes, and seed capture. The rate lane tests both terminations at native Fs 32000/44100/48000/96000 for every quarter-frame partition from 4 through 128. These are tested native rates, not a claim of physical firmware support.

Boundary tests use independently checked integer arithmetic. Rate9.9 Hz gives ceil(480000/99)=4849 samples. Duration4748ms has deadline227904; its 47th rise227903 drains to228143. Duration101ms has deadline4848 and rejects first due4849. Rate7.3 Hz gives first due6576, equal to Duration137ms's deadline, and emits no event. The restart trace translates the contract's old rise0 to rise480, giving old fall720, ready768 and replacement first rise5568. Translation does not alter relative width/gap or onset expectations.

Seed distributions retain all seeds1..1024, all five contours and both terminations in CSV, not only means. Core and actual-adapter distributions must agree. Count comparisons use intervals1..7 versus25..31. Duration compares equal-length combined outer quarters with the middle half. Finite windows have no forced event, exact count or tail fill. Flat variable-interval bounds and count variation are reported, not fitted acceptance thresholds.

| Criterion | D1 proof | Still outside D1 |
| --- | --- | --- |
| AC-01 | Frozen Internal sample/voltage traces through engine and actual adapter. | Independent review/assembled acceptance. |
| AC-02 | W/G at four native rates and every 4..128 partition, positive ceil/floors, limits and bounds. | Clock portion and physical rates. |
| AC-03 | Independent seed vectors, accepted/ignored/reset streams, partition goldens, full distributions and automatic seed capture. | Clock and target entropy characterization. |
| AC-04 | Internal Count/Duration restart, gap/high/deadline/final-fall/old-due cases, snapshots and actions. | Clock waiting/acquisition/drain cases. |
| AC-05 | Not implemented. | Entire Clock criterion belongs to D2. |
| AC-06 | Internal ignores Clock; core explicit cancel; recognized load/routing/Fs safety reset. | Clock abort and physical unobservable bypass/re-enable. |
| AC-07 | Actual factory/pages/params/buffers, None/bounds/aliases, Add/Replace, nonfinite/manual/load/retention and status. | Clock statuses/units in working Clock operation. |
| AC-08 | Strict native, ASan/UBSan, cppcheck/Clang analysis, inspected PIC ARM object and host diagnostics. | Physical firmware/loader/View info/target cycles and budget. |
| AC-09 | No physical proof. | Operator electrical/host evidence and Neal's audition. |
| AC-10 | Internal literals, zero events, T-1/T/T+1, full final high, snapshot and retained settings. | Clock Duration. |
| AC-11 | Literal Internal contours/scales and full actual density distributions. | Clock rational boundaries and audition. |

Native live lane1 records the original README-only baseline as feature absent. Lanes2..6 and10 use the actual host driver. Lanes7..9 are explicitly not implemented until D2. Sample-voltage plots visualize native buffers, not oscilloscope measurements. A display-call receipt is not an emulator screenshot.

## Host performance method

The claim is only a diagnostic host callback deadline check, with zero measured C++ hot-path allocations, not a speedup or target performance acceptance. `pop_perf.cpp` times the actual callback outside the production code. Each of 30 repetitions per scenario renders one simulated second in legal 128-frame blocks, after 100 warmup calls, interleaving empty-host and candidate. Scenarios are idle, saturated Internal Duration, frequent restart, safety changes, final drain and invalid routing. Setup, buffer clearing, result validation, telemetry and file output are outside the timed region. Callback work and dynamic parameter changes inside step remain timed.

`perf-blocks.csv` retains every callback measurement; `perf.csv` retains repetitions, edge counts, allocations and p95 block time. The empty host writes zeros to output buffers but has no Pop callback at the original baseline. Do not compute a Pop speed ratio against it. Output assertions require real saturated events and one full drain event; idle/invalid lanes explicitly expect zero. Allocation instrumentation counts C++ `new`/`new[]`; source inspection and undefined-symbol inspection separately rule out production malloc/calloc/realloc paths. A host p95 deadline does not establish a target CPU budget. Report scheduler outliers without deleting them. Profiling is separate from reported timing runs.

## ARM and runtime limits

Inspect the actual object with readelf, size and nm. `pluginEntry` must be global, ELF32 little-endian ARM REL. No test injection symbols or operator new heap references may enter it. Placement new is construction in host-provided memory. Undefined symbols fall into host callbacks/globals/JSON, PIC GOT, libm `exp2`, and C memory/string functions. `make hardware` now rejects every import absent from the pinned offline beta18 function table and its two substantiated loader-special symbols. It rejects compiler ABI helpers, including the three unsupported imports in the original D1 object. Offline availability does not establish physical loader resolution, relocation behavior or cycles. Pools and loader section placement are firmware-dependent; do not invent timeless byte budgets.

An isolated clone of available nt_emu commit `f7c26fa27af230cd2ab9a7a6ee12b65bf1a68cc1` pins API v13 (`cd12d876dbe060859828053efab1cbc98c9df251`), while its loader rejects unequal API versions. Its GUI build also lacks the tracked imgui source/submodule mapping. D1 attempted isolated CMake configuration and recorded the failure. The unrelated dirty/ahead checkout was inspected read-only and not repaired or installed into. No compatible emulator run or physical display frame is claimed; the actual adapter driver is the available execution seam.

## Urgent D1 loader correction checkpoint

The owner reported `Unresolved symbol __aeabi_d2ulz : pop.o`. The original local D1 object imported that conversion helper, `__aeabi_ul2d` and `__aeabi_uldivmod`. Its identity is only a plausible source of the loaded bytes, not confirmed device provenance. The new arithmetic removes all three without linking an unverified runtime library or reducing timing to binary32.

`pop_sample_math.h` owns positive interval conversion, sample-count conversion and millisecond scaling. Ceil intervals are bounded by `4 * UINT32_MAX < 2^34`. Splitting the rounded integer into two exact 32-bit words avoids a double-to-uint64 runtime call without narrowing the whole interval. Duration progress and deadline differences are at most `30 * UINT32_MAX < 2^37`, so both word conversion and their binary64 sum are exact. Splitting Fs into quotient and remainder modulo1000 gives `ceil(Fs*ms/1000)` with only 32-bit division. For ms<=30000 the remainder numerator is at most29970999. The wide quotient product and addition stay below2^37. Width/gap use ms5/1 under the same law. Supported native rates32000/44100/48000/96000 are a subset; the arithmetic proof covers the complete nonzero uint32 Fs domain, not an undocumented physical rate claim.

The math test checks integer/adjacent binary64 ceil boundaries around2^32, the maximum interval17179869180, every ms1..30000 at eight rate boundaries includingUINT32_MAX, width/gap and exact maximum Duration128849018850. Existing engine/adapter waveforms, retrigger/Duration/contour/RNG regressions remain unchanged. Integer sample timestamps have finite uint64 lifetime; maximum uninterrupted Internal Count64 adds at most256*Fs+W, below2^41 across this numerical domain. No lifetime promise covers unlimited restarts or2^64 samples of uninterrupted instance uptime.

`tests/fixtures/nt-beta18-exports.json` pins the downloaded image hash,114 actual pointer/address pairs and the loader's GOT/NT_globals initialization evidence. `python3 tests/audit_nt_exports.py /path/to/NT_1.19beta_18.zip` verifies the hash, parsed table and exact instruction/literal bytes without reading a device. The loader initialization at0x6007071c..0x60070738 constructs8-byte symbol pairs for `_GLOBAL_OFFSET_TABLE_` and `NT_globals`; the lookup uses the same name-table stride. These are documented special symbols, not an arbitrary helper whitelist. The manifest retains addresses without inventing that every entry is an odd Thumb function pointer. This candidate image is not authenticated download provenance or currently flashed firmware evidence. Physical loader success remains untested.

`make fixtures-check` compares the original three frozen fixture files against committed `SHA256SUMS`; it does not regenerate expectations. A modified isolated copy must fail. Correction L4 is addressed. L1 remains open because deserialise still uses an audio setter outside its documented context. L2's finite named complete waveforms and lifetime/boundary table remain open. L3 has new exact-object receipts and an archive prepared for runner settlement; upload/download verification belongs to the coordinator after this producer finishes. This is an explicitly authorized urgent Internal-only checkpoint, not D1 acceptance or whole-v1 completion.

Target acceptance remains pending with explicit owner responsibilities. The operator must record firmware, loader result, View info, sample rates/buffers, cycle/resource spread, measured DAC widths/gaps/levels with equipment and receiver tolerances, short-trigger thresholds, routing and lifecycle behavior. Neal must audition both modes, all contours, defaults and the regular corner after D2. No native artifact substitutes for those results.
