#pragma once
#include "pop_engine.h"
#include <distingnt/api.h>
namespace pop {
enum Parameter : uint8_t {
    TerminationParam,
    CountParam,
    DurationParam,
    RateParam,
    IrregularityParam,
    ContourParam,
    AmountParam,
    RetriggerParam,
    TimingParam,
    TriggerInputParam,
    ClockInputParam,
    BurstOutputParam,
    BurstModeParam,
    ActiveOutputParam,
    ActiveModeParam,
    ManualParam,
    NumParameters
};
#ifdef POP_TESTING
void injectSeed(_NT_algorithm *self, uint32_t seed);
const Engine &inspectEngine(const _NT_algorithm *self);
#endif
} // namespace pop
