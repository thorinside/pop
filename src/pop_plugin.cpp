#include "pop_plugin.h"
#include <climits>
#include <cstring>
#include <distingnt/serialisation.h>
#include <new>

namespace pop {
namespace {
const char *const terminationNames[] = {"Count", "Duration", nullptr};
const char *const timingNames[] = {"Internal", "Clock", nullptr};
const char *const contourNames[] = {"Flat",          "Acceleration",  "Deceleration",
                                    "Rise and fall", "Fall and rise", nullptr};
const char *const offOn[] = {"Off", "On", nullptr};
const char *const fireNames[] = {"Off", "Fire", nullptr};
const char *const beatNames[] = {"0.25 beats", "0.5 beats", "1 beat",   "2 beats", "4 beats",
                                 "8 beats",    "16 beats",  "32 beats", nullptr};
const char *const ratioNames[] = {"/8", "/4", "/2", "unity", "x2", "x4", "x8", nullptr};
const _NT_parameter defaults[] = {
    {"Termination", 0, 1, 0, kNT_unitEnum, 0, terminationNames},
    {"Count", 1, 64, 8, kNT_unitNone, 0, nullptr},
    {"Duration", 100, 30000, 500, kNT_unitSeconds, kNT_scaling1000, nullptr},
    {"Rate", 10, 1000, 200, kNT_unitHz, kNT_scaling10, nullptr},
    {"Irregularity", 0, 100, 50, kNT_unitPercent, 0, nullptr},
    {"Contour", 0, 4, 2, kNT_unitEnum, 0, contourNames},
    {"Contour amount", 0, 100, 75, kNT_unitPercent, 0, nullptr},
    {"Retrigger", 0, 1, 1, kNT_unitEnum, 0, offOn},
    {"Timing", 0, 1, 0, kNT_unitEnum, 0, timingNames},
    {"Trigger input", 0, kNT_lastBus, 1, kNT_unitCvInput, 0, nullptr},
    {"Clock input", 0, kNT_lastBus, 0, kNT_unitCvInput, 0, nullptr},
    {"Burst output", 1, kNT_lastBus, 13, kNT_unitCvOutput, 0, nullptr},
    {"Burst mode", 0, 1, 1, kNT_unitOutputMode, 0, nullptr},
    {"Active output", 0, kNT_lastBus, 14, kNT_unitCvOutput, 0, nullptr},
    {"Active mode", 0, 1, 1, kNT_unitOutputMode, 0, nullptr},
    {"Manual Start", 0, 1, 0, kNT_unitEnum, 0, fireNames},
};
static_assert(sizeof(defaults) / sizeof(defaults[0]) == NumParameters);
const uint8_t mainParams[] = {TerminationParam, CountParam,  DurationParam,
                              RateParam,        ManualParam, RetriggerParam};
const uint8_t shapeParams[] = {IrregularityParam, ContourParam, AmountParam, TimingParam};
const uint8_t routingParams[] = {TriggerInputParam, ClockInputParam,   BurstOutputParam,
                                 BurstModeParam,    ActiveOutputParam, ActiveModeParam};
const _NT_parameterPage pages[] = {{"Burst", 6, 0, {0, 0}, mainParams},
                                   {"Shape", 4, 0, {0, 0}, shapeParams},
                                   {"Routing", 6, 0, {0, 0}, routingParams}};
const _NT_parameterPages allPages = {3, pages};
struct Algorithm : _NT_algorithm {
    explicit Algorithm(uint32_t seed) : _NT_algorithm{}, engine(NT_globals.sampleRate, seed) {
        std::memcpy(definitions, defaults, sizeof(defaults));
        parameters = definitions;
        parameterPages = &allPages;
        v = vIncludingCommon = nullptr;
    }
    _NT_parameter definitions[NumParameters]{};
    Engine engine;
    Schmitt trigger, clock;
    bool manualArmed = false, resetPending = true, updating = false;
    int mode = 0;
    int internalRate = 200, clockRate = 0, internalDuration = 500, clockDuration = 2;
    uint32_t fs = NT_globals.sampleRate;
    int16_t safety[7] = {0, 1, 0, 13, 1, 14, 1};
    bool routingValid = true;
};
uint32_t index(const Algorithm &a) { return static_cast<uint32_t>(NT_algorithmIndex(&a)); }
void remember(Algorithm &a) {
    if (a.mode == 0) {
        a.internalRate = a.v[RateParam];
        a.internalDuration = a.v[DurationParam];
    } else {
        a.clockRate = a.v[RateParam];
        a.clockDuration = a.v[DurationParam];
    }
}
void setValue(const Algorithm &a, int p, int value) {
    NT_setParameterFromAudio(index(a), static_cast<uint32_t>(p) + NT_parameterOffset(),
                             static_cast<int16_t>(value));
}
void updateMode(Algorithm &a) {
    if (a.updating || !a.v || a.v[TimingParam] < 0 || a.v[TimingParam] > 1)
        return;
    if (a.mode == a.v[TimingParam])
        return;
    // parameterChanged sees the new Timing but the old mode's Rate/Duration.
    remember(a);
    a.mode = a.v[TimingParam];
    a.updating = true;
    if (a.mode) {
        a.definitions[RateParam] = {"Rate", -3, 3, 0, kNT_unitHasStrings, 0, nullptr};
        a.definitions[DurationParam] = {"Duration", 0, 7, 2, kNT_unitEnum, 0, beatNames};
    } else {
        a.definitions[RateParam] = defaults[RateParam];
        a.definitions[DurationParam] = defaults[DurationParam];
    }
    NT_updateParameterDefinition(index(a), RateParam);
    NT_updateParameterDefinition(index(a), DurationParam);
    setValue(a, RateParam, a.mode ? a.clockRate : a.internalRate);
    setValue(a, DurationParam, a.mode ? a.clockDuration : a.internalDuration);
    a.updating = false;
}
void changed(_NT_algorithm *self, int p) {
    auto &a = *static_cast<Algorithm *>(self);
    if (a.updating || !a.v)
        return;
    if (p == TimingParam)
        updateMode(a);
    const bool duration = a.v[TerminationParam] == 1;
    NT_setParameterGrayedOut(index(a), CountParam + NT_parameterOffset(), duration);
    NT_setParameterGrayedOut(index(a), DurationParam + NT_parameterOffset(), !duration);
    NT_setParameterGrayedOut(index(a), ClockInputParam + NT_parameterOffset(),
                             a.v[TimingParam] == 0);
    if (p == RateParam || p == DurationParam)
        remember(a);
}
bool bus(int value, bool none) { return value >= (none ? 0 : 1) && value <= kNT_lastBus; }
bool validRouting(const int16_t *v) {
    const int t = v[TriggerInputParam], c = v[ClockInputParam], b = v[BurstOutputParam],
              a = v[ActiveOutputParam];
    return bus(t, true) && bus(c, true) && bus(b, false) && bus(a, true) && b != t && b != c &&
           (a == 0 || (a != b && a != t && a != c)) && v[BurstModeParam] >= 0 &&
           v[BurstModeParam] <= 1 && v[ActiveModeParam] >= 0 && v[ActiveModeParam] <= 1 &&
           v[TimingParam] >= 0 && v[TimingParam] <= 1;
}
Settings settings(const Algorithm &a) {
    Settings s;
    s.termination = static_cast<Termination>(a.v[TerminationParam]);
    s.timing = static_cast<Timing>(a.v[TimingParam]);
    s.count = a.v[CountParam];
    s.durationMs = a.mode ? a.internalDuration : a.v[DurationParam];
    s.rateTenths = a.mode ? a.internalRate : a.v[RateParam];
    s.clockRate = a.mode ? a.v[RateParam] : a.clockRate;
    s.durationBeatsIndex = a.mode ? a.v[DurationParam] : a.clockDuration;
    s.irregularity = a.v[IrregularityParam];
    s.contour = static_cast<Contour>(a.v[ContourParam]);
    s.amount = a.v[AmountParam];
    return s;
}
void requirements(_NT_algorithmRequirements &r, const int32_t *) {
    r = {NumParameters, sizeof(Algorithm), 0, 0, 0};
}
_NT_algorithm *construct(const _NT_algorithmMemoryPtrs &ptrs, const _NT_algorithmRequirements &,
                         const int32_t *) {
    return new (ptrs.sram) Algorithm(NT_getCpuCycleCount() ^ 0x6d2b79f5u);
}
void write(float *buffers, int frames, int i, int busNumber, int mode, float voltage) {
    if (!bus(busNumber, false))
        return;
    float &destination = buffers[(busNumber - 1) * frames + i];
    if (mode == 1)
        destination = voltage;
    else if (mode == 0)
        destination += voltage;
}
void step(_NT_algorithm *self, float *buffers, int by4) {
    auto &a = *static_cast<Algorithm *>(self);
    if (!buffers || !a.v || by4 <= 0 || by4 > INT_MAX / 4 ||
        static_cast<uint32_t>(by4) * 4 > NT_globals.maxFramesPerStep)
        return;
    const int frames = by4 * 4;
    updateMode(a);
    const int params[7] = {TimingParam,    TriggerInputParam, ClockInputParam, BurstOutputParam,
                           BurstModeParam, ActiveOutputParam, ActiveModeParam};
    bool reset = a.resetPending || a.fs != NT_globals.sampleRate;
    a.resetPending = false;
    for (int p = 0; p < 7; ++p) {
        if (a.safety[p] != a.v[params[p]])
            reset = true;
        a.safety[p] = a.v[params[p]];
    }
    a.routingValid = validRouting(a.v);
    if (!a.routingValid)
        reset = true;
    if (reset) {
        a.fs = NT_globals.sampleRate;
        a.engine.safetyReset(a.fs);
        a.trigger = {};
        a.clock = {};
        a.manualArmed = false;
    }
    const Settings current = settings(a);
    const int trigger = a.v[TriggerInputParam], clock = a.v[ClockInputParam];
    const int burst = a.v[BurstOutputParam], active = a.v[ActiveOutputParam];
    for (int i = 0; i < frames; ++i) {
        const float tv = bus(trigger, false) ? buffers[(trigger - 1) * frames + i] : 0.0f;
        const float cv = bus(clock, false) ? buffers[(clock - 1) * frames + i] : 0.0f;
        bool start = a.trigger.observe(tv);
        (void)a.clock.observe(cv);
        if (i == 0) {
            if (a.v[ManualParam] == 0)
                a.manualArmed = true;
            else if (a.v[ManualParam] == 1 && a.manualArmed) {
                start = true;
                a.manualArmed = false;
            }
        }
        if (!a.routingValid || (reset && i == 0))
            start = false;
        const Output out = a.engine.tick(current, a.v[RetriggerParam] == 1, {start, false, false});
        if (!a.routingValid)
            a.engine.setError(Error::InvalidRouting);
        else if (start && current.timing == Timing::Clock && clock == 0)
            a.engine.setError(Error::NoClock);
        write(buffers, frames, i, burst, a.v[BurstModeParam],
              a.routingValid && out.burst ? 5.0f : 0.0f);
        if (active != burst)
            write(buffers, frames, i, active, a.v[ActiveModeParam],
                  a.routingValid && out.active ? 5.0f : 0.0f);
    }
}
// The official factory callback requires a non-const algorithm pointer.
// cppcheck-suppress constParameterCallback
bool draw(_NT_algorithm *self) {
    const auto &a = *static_cast<const Algorithm *>(self);
    const char *status = "Idle";
    switch (a.engine.phase()) {
    case Phase::Idle:
        break;
    case Phase::Pending:
        status = "Waiting for gap";
        break;
    case Phase::Generating:
        status = "Active";
        break;
    case Phase::Draining:
        status = "Draining";
        break;
    }
    NT_drawText(2, 25, status);
    NT_drawText(150, 25, a.v && a.v[TerminationParam] == 1 ? "Duration" : "Count");
    const char *error = "";
    switch (a.engine.error()) {
    case Error::None:
        break;
    case Error::InvalidSettings:
        error = "Invalid settings";
        break;
    case Error::InvalidRouting:
        error = "Invalid routing";
        break;
    case Error::NoClock:
        error = "No clock input";
        break;
    case Error::ClockIncomplete:
        error = "Clock not implemented (D1)";
        break;
    }
    NT_drawText(2, 38, error);
    if (a.engine.limits() & BaseLimited)
        NT_drawText(2, 49, "Base limited");
    if (a.engine.limits() & DurationLimited)
        NT_drawText(90, 49, "Duration limited");
    if (a.engine.limits() & PhysicalLimited)
        NT_drawText(2, 60, "Interval limited");
    return false;
}
int parameterString(_NT_algorithm *, int p, int v, char *buffer) {
    if (p != RateParam || v < -3 || v > 3)
        return 0;
    const char *text = ratioNames[v + 3];
    const auto size = std::strlen(text);
    std::memcpy(buffer, text, size + 1);
    return static_cast<int>(size);
}
void serialise(_NT_algorithm *self, _NT_jsonStream &stream) {
    auto &a = *static_cast<Algorithm *>(self);
    if (a.v)
        remember(a);
    stream.addMemberName("internalRate");
    stream.addNumber(a.internalRate);
    stream.addMemberName("clockRate");
    stream.addNumber(a.clockRate);
    stream.addMemberName("internalDuration");
    stream.addNumber(a.internalDuration);
    stream.addMemberName("clockDuration");
    stream.addNumber(a.clockDuration);
}
bool deserialise(_NT_algorithm *self, _NT_jsonParse &parse) {
    auto &a = *static_cast<Algorithm *>(self);
    a.engine.safetyReset(NT_globals.sampleRate);
    a.trigger = {};
    a.clock = {};
    a.manualArmed = false;
    a.resetPending = true;
    int n = 0, ir = 200, cr = 0, id = 500, cd = 2;
    if (!parse.numberOfObjectMembers(n) || n < 0 || n > 64)
        return false;
    for (int i = 0; i < n; ++i) {
        if (parse.matchName("internalRate")) {
            if (!parse.number(ir))
                return false;
        } else if (parse.matchName("clockRate")) {
            if (!parse.number(cr))
                return false;
        } else if (parse.matchName("internalDuration")) {
            if (!parse.number(id))
                return false;
        } else if (parse.matchName("clockDuration")) {
            if (!parse.number(cd))
                return false;
        } else if (!parse.skipMember())
            return false;
    }
    if (ir < 10 || ir > 1000 || cr < -3 || cr > 3 || id < 100 || id > 30000 || cd < 0 || cd > 7)
        return false;
    a.internalRate = ir;
    a.clockRate = cr;
    a.internalDuration = id;
    a.clockDuration = cd;
    if (a.v) {
        a.updating = true;
        setValue(a, RateParam, a.mode ? cr : ir);
        setValue(a, DurationParam, a.mode ? cd : id);
        a.updating = false;
    }
    return true;
}
const _NT_factory factory = {NT_MULTICHAR('T', 'h', 'P', 'o'),
                             "Pop",
                             "Finite organic bursts (Internal D1)",
                             0,
                             nullptr,
                             nullptr,
                             nullptr,
                             requirements,
                             construct,
                             changed,
                             step,
                             draw,
                             nullptr,
                             nullptr,
                             kNT_tagUtility,
                             nullptr,
                             nullptr,
                             nullptr,
                             serialise,
                             deserialise,
                             nullptr,
                             nullptr,
                             parameterString,
                             nullptr};
} // namespace
#ifdef POP_TESTING
void injectSeed(_NT_algorithm *self, uint32_t seed) {
    static_cast<Algorithm *>(self)->engine.injectSeed(seed);
}
const Engine &inspectEngine(const _NT_algorithm *self) {
    return static_cast<const Algorithm *>(self)->engine;
}
#endif
} // namespace pop
uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch (selector) {
    case kNT_selector_version:
        return kNT_apiVersion14;
    case kNT_selector_numFactories:
        return 1;
    case kNT_selector_factoryInfo:
        return data == 0 ? reinterpret_cast<uintptr_t>(&pop::factory) : 0;
    }
    return 0;
}
