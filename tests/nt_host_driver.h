#pragma once
#include "pop_plugin.h"
#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace host {
using Json = std::vector<std::pair<std::string, int>>;
struct Trace {
    std::vector<uint64_t> rises, falls;
    std::vector<float> burst, active;
};
class Instance {
  public:
    explicit Instance(bool injectedSeed = true);
    ~Instance();
    Instance(const Instance &) = delete;
    Instance &operator=(const Instance &) = delete;
    void set(int p, int value);
    void process(int frames, float fill = 0.0f);
    Trace run(int samples, int partition, const std::vector<uint64_t> &starts, int triggerBus = 1);
    bool load(Json json = {});
    Json save();
    const _NT_factory *factory;
    _NT_algorithmRequirements requirements{};
    _NT_algorithm *algorithm;
    std::array<int16_t, pop::NumParameters> values{};
    std::array<bool, pop::NumParameters> gray{};
    std::vector<std::max_align_t> memory;
    std::array<float, 64 * 128> buffers{};
    std::vector<std::string> display;
    unsigned definitionUpdates = 0;
    bool previousBurst = false;
};
extern bool trackingAllocations;
extern uint64_t allocations;
extern uint32_t cycleCount;
void sampleRate(uint32_t value);
void emptyHost(float *buffers, int frames);
void draw(Instance &instance);
} // namespace host
