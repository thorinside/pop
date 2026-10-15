#include "nt_host_driver.h"
#include "check.h"
#define _DISTINGNT_SERIALISATION_INTERNAL
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <distingnt/serialisation.h>
#include <new>

#ifndef HOST_FS
#define HOST_FS 48000
#endif
// Firmware exposes a const view of globals that the host owns and can update.
#ifdef __APPLE__
#define HOST_GLOBALS_SYMBOL "_NT_globals"
#else
#define HOST_GLOBALS_SYMBOL "NT_globals"
#endif
extern "C" {
_NT_globals hostGlobals __asm__(HOST_GLOBALS_SYMBOL) = {HOST_FS, 128, nullptr, 0, 0, 0};
}
namespace host {
bool trackingAllocations = false;
uint64_t allocations = 0;
uint32_t cycleCount = 0x6d2b79f4u;
void sampleRate(uint32_t value) { hostGlobals.sampleRate = value; }
static Instance *instances[64]{};
static Instance *drawing = nullptr;
static unsigned audioContextDepth = 0;
static uint32_t commonOffset = 0;
struct AudioContext {
    AudioContext() { ++audioContextDepth; }
    ~AudioContext() { --audioContextDepth; }
};
Instance::Instance(bool injectedSeed, uint32_t offset)
    : commonOffset(offset), factory(reinterpret_cast<const _NT_factory *>(pluginEntry(kNT_selector_factoryInfo, 0))) {
    CHECK(factory && pluginEntry(kNT_selector_version, 0) == 14 &&
          pluginEntry(kNT_selector_numFactories, 0) == 1);
    factory->calculateRequirements(requirements, nullptr);
    CHECK(requirements.numParameters == pop::NumParameters && requirements.dram == 0 &&
          requirements.dtc == 0 && requirements.itc == 0);
    memory.resize((requirements.sram + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t));
    _NT_algorithmMemoryPtrs ptrs{reinterpret_cast<uint8_t *>(memory.data()), nullptr, nullptr,
                                 nullptr};
    algorithm = factory->construct(ptrs, requirements, nullptr);
    CHECK(algorithm);
    bool registered = false;
    for (auto &instance : instances)
        if (!instance) {
            instance = this;
            registered = true;
            break;
        }
    CHECK(registered);
    for (size_t i = 0; i < values.size(); ++i)
        values[i] = algorithm->parameters[i].def;
    algorithm->v = values.data();
    algorithm->vIncludingCommon = values.data();
    for (int p = 0; p < pop::NumParameters; ++p)
        set(p, values[static_cast<size_t>(p)]);
    process(4);
    if (injectedSeed)
        pop::injectSeed(algorithm, 1);
}
Instance::~Instance() {
    for (auto &instance : instances)
        if (instance == this)
            instance = nullptr;
}
void Instance::set(int p, int value) {
    CHECK(p >= 0 && p < pop::NumParameters);
    values[static_cast<size_t>(p)] = static_cast<int16_t>(value);
    AudioContext context;
    host::commonOffset = commonOffset;
    factory->parameterChanged(algorithm, p);
}
void Instance::step(int framesBy4) {
    AudioContext context;
    host::commonOffset = commonOffset;
    factory->step(algorithm, buffers.data(), framesBy4);
}
void Instance::process(int frames, float fill) {
    CHECK(frames > 0 && frames <= 128 && frames % 4 == 0);
    std::fill_n(buffers.data(), 64 * frames, fill);
    step(frames / 4);
    const int output = values[pop::BurstOutputParam];
    previousBurst = output >= 1 && output <= 64 &&
                    buffers[static_cast<size_t>((output - 1) * frames + frames - 1)] >= 5;
}
Trace Instance::run(int samples, int partition, const std::vector<uint64_t> &starts,
                    int triggerBus) {
    CHECK(samples % 4 == 0 && partition % 4 == 0 && partition > 0 && partition <= 128);
    Trace trace;
    trace.burst.reserve(static_cast<size_t>(samples));
    trace.active.reserve(static_cast<size_t>(samples));
    bool previous = previousBurst;
    for (int base = 0; base < samples; base += partition) {
        const int frames = std::min(partition, samples - base);
        std::fill_n(buffers.data(), 64 * frames, 0.0f);
        for (int i = 0; i < frames; ++i) {
            const uint64_t t = static_cast<uint64_t>(base + i);
            if (std::find(starts.begin(), starts.end(), t) != starts.end() && triggerBus > 0)
                buffers[static_cast<size_t>((triggerBus - 1) * frames + i)] = 5.0f;
        }
        step(frames / 4);
        for (int i = 0; i < frames; ++i) {
            const float b =
                buffers[static_cast<size_t>((values[pop::BurstOutputParam] - 1) * frames + i)];
            const float a = values[pop::ActiveOutputParam]
                                ? buffers[static_cast<size_t>(
                                      (values[pop::ActiveOutputParam] - 1) * frames + i)]
                                : 0;
            CHECK(b == 0.0f || b == 5.0f);
            CHECK(a == 0.0f || a == 5.0f);
            const bool high = b == 5.0f;
            const auto t = static_cast<uint64_t>(base + i);
            if (high && !previous)
                trace.rises.push_back(t);
            if (!high && previous)
                trace.falls.push_back(t);
            trace.burst.push_back(b);
            trace.active.push_back(a);
            previous = high;
        }
    }
    previousBurst = previous;
    return trace;
}
bool Instance::load(Json json) {
    _NT_jsonParse parser(&json, 0);
    return factory->deserialise(algorithm, parser);
}
Json Instance::save() {
    Json json;
    _NT_jsonStream stream(&json);
    factory->serialise(algorithm, stream);
    return json;
}
void emptyHost(float *buffers, int frames) { std::fill_n(buffers + 12 * frames, 2 * frames, 0.0f); }
} // namespace host
void *operator new(std::size_t size) {
    if (host::trackingAllocations)
        ++host::allocations;
    void *p = std::malloc(size ? size : 1);
    if (!p)
        std::abort();
    return p;
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
extern "C" {
uint32_t NT_getCpuCycleCount() { return host::cycleCount; }
int32_t NT_algorithmIndex(const _NT_algorithm *a) {
    for (int i = 0; i < 64; ++i)
        if (host::instances[i] && host::instances[i]->algorithm == a)
            return i;
    return -1;
}
uint32_t NT_parameterOffset() { return host::commonOffset; }
void NT_setParameterFromAudio(uint32_t i, uint32_t p, int16_t value) {
    CHECK(host::audioContextDepth > 0);
    CHECK(i < 64 && host::instances[i]);
    auto &instance = *host::instances[i];
    CHECK(p >= instance.commonOffset && p < instance.commonOffset + pop::NumParameters);
    ++instance.audioSetterCalls;
    instance.set(static_cast<int>(p - instance.commonOffset), value);
}
void NT_setParameterGrayedOut(uint32_t i, uint32_t p, bool gray) {
    CHECK(i < 64 && host::instances[i]);
    auto &instance = *host::instances[i];
    CHECK(p >= instance.commonOffset && p < instance.commonOffset + pop::NumParameters);
    instance.gray[p - instance.commonOffset] = gray;
}
void NT_updateParameterDefinition(uint32_t i, uint32_t p) {
    CHECK(i < 64 && host::instances[i] && p < pop::NumParameters);
    ++host::instances[i]->definitionUpdates;
}
void NT_drawText(int, int, const char *text, int, _NT_textAlignment, _NT_textSize) {
    CHECK(host::drawing);
    host::drawing->display.emplace_back(text);
}
}
_NT_jsonParse::_NT_jsonParse(void *context, int token) : refCon(context), i(token) {}
_NT_jsonParse::~_NT_jsonParse() = default;
bool _NT_jsonParse::numberOfObjectMembers(int &n) {
    n = static_cast<int>(static_cast<host::Json *>(refCon)->size());
    return true;
}
bool _NT_jsonParse::matchName(const char *name) {
    const auto &j = *static_cast<host::Json *>(refCon);
    return static_cast<size_t>(i) < j.size() && j[static_cast<size_t>(i)].first == name;
}
bool _NT_jsonParse::number(int &v) {
    const auto &j = *static_cast<host::Json *>(refCon);
    if (static_cast<size_t>(i) >= j.size())
        return false;
    v = j[static_cast<size_t>(i++)].second;
    return true;
}
bool _NT_jsonParse::skipMember() {
    ++i;
    return static_cast<size_t>(i) <= static_cast<host::Json *>(refCon)->size();
}
_NT_jsonStream::_NT_jsonStream(void *context) : refCon(context) {}
_NT_jsonStream::~_NT_jsonStream() = default;
void _NT_jsonStream::addMemberName(const char *name) {
    static_cast<host::Json *>(refCon)->emplace_back(name, 0);
}
void _NT_jsonStream::addNumber(int value) {
    static_cast<host::Json *>(refCon)->back().second = value;
}
namespace host {
void draw(Instance &instance) {
    drawing = &instance;
    instance.display.clear();
    CHECK(!instance.factory->draw(instance.algorithm));
    drawing = nullptr;
    auto *file = std::fopen("build/display-calls.txt", "a");
    CHECK(file);
    std::fprintf(file, "actual factory draw callback, sample=%llu\n",
                 static_cast<unsigned long long>(pop::inspectEngine(instance.algorithm).sample()));
    for (const auto &line : instance.display)
        std::fprintf(file, "%s\n", line.c_str());
    std::fclose(file);
}
} // namespace host
