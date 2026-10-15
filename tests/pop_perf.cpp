#include "check.h"
#include "nt_host_driver.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numeric>
using Clock = std::chrono::steady_clock;
using namespace pop;
struct Result {
    double nsPerSample;
    uint64_t events;
    uint64_t allocations;
    double p95;
};
Result measure(const char *scenario, bool baseline, std::FILE *blocks, int repetition) {
    host::Instance h;
    h.set(RateParam, 1000);
    h.set(ContourParam, 2);
    h.set(AmountParam, 100);
    h.set(IrregularityParam, 0);
    h.set(TerminationParam, 1);
    h.set(DurationParam, 30000);
    if (std::strcmp(scenario, "drain") == 0) {
        h.set(TerminationParam, 0);
        h.set(CountParam, 1);
        h.set(ContourParam, 0);
    }
    if (std::strcmp(scenario, "invalid") == 0)
        h.set(ActiveOutputParam, 13);
    for (int i = 0; i < 100; ++i)
        h.process(128);
    const bool idle = std::strcmp(scenario, "idle") == 0;
    if (!idle)
        h.set(ManualParam, 1);
    std::vector<double> durations;
    durations.reserve(375);
    uint64_t events = 0;
    bool previous = false;
    host::allocations = 0;
    int counter = 0;
    for (int base = 0; base < 48000; base += 128) {
        const int frames = std::min(128, 48000 - base);
        if (std::strcmp(scenario, "restart") == 0)
            h.set(ManualParam, counter % 2 ? 0 : 1);
        if (std::strcmp(scenario, "safety") == 0) {
            h.set(TriggerInputParam, counter % 2);
            h.set(ManualParam, counter % 2 ? 0 : 1);
        }
        std::fill_n(h.buffers.data(), 64 * frames, 0.0f);
        host::trackingAllocations = true;
        const auto start = Clock::now();
        if (baseline)
            host::emptyHost(h.buffers.data(), frames);
        else
            h.factory->step(h.algorithm, h.buffers.data(), frames / 4);
        const auto stop = Clock::now();
        host::trackingAllocations = false;
        const double ns = std::chrono::duration<double, std::nano>(stop - start).count();
        durations.push_back(ns);
        if (blocks)
            std::fprintf(blocks, "%s,%s,%d,%d,%d,%.0f\n", scenario,
                         baseline ? "emptyHost" : "candidate", repetition, counter, frames, ns);
        for (int i = 0; i < frames; ++i) {
            const float v = h.buffers[static_cast<size_t>(12 * frames + i)];
            CHECK(v == 0 || v == 5);
            const bool high = v == 5;
            if (high && !previous)
                ++events;
            previous = high;
        }
        ++counter;
    }
    CHECK(host::allocations == 0);
    if (!baseline && std::strcmp(scenario, "saturated") == 0)
        CHECK(events >= 165 && events <= 167);
    if (!baseline && std::strcmp(scenario, "drain") == 0)
        CHECK(events == 1);
    if (idle || baseline || std::strcmp(scenario, "invalid") == 0)
        CHECK(events == 0);
    const double total = std::accumulate(durations.begin(), durations.end(), 0.0);
    std::sort(durations.begin(), durations.end());
    const double p95 =
        durations[static_cast<size_t>(std::ceil(0.95 * static_cast<double>(durations.size()))) - 1];
    CHECK(baseline || p95 < 128.0 / 48000.0 * 1e9);
    return {total / 48000.0, events, host::allocations, p95};
}
int main(int argc, char **) {
    const int repetitions = argc > 1 ? 600 : 30;
    auto *raw = argc > 1 ? nullptr : std::fopen("build/perf-blocks.csv", "w");
    auto *results = argc > 1 ? nullptr : std::fopen("build/perf.csv", "w");
    if (raw)
        std::fprintf(raw, "scenario,side,repetition,block,frames,nanoseconds\n");
    if (results)
        std::fprintf(results,
                     "scenario,side,repetition,nsPerSample,events,allocations,p95BlockNs\n");
    for (const char *scenario : {"idle", "saturated", "restart", "safety", "drain", "invalid"}) {
        std::vector<double> samples;
        double worstP95 = 0;
        for (int r = 0; r < repetitions; ++r)
            for (int side = 0; side < 2; ++side) {
                const auto m = measure(scenario, side == 0, raw, r);
                if (results)
                    std::fprintf(results, "%s,%s,%d,%.6f,%llu,%llu,%.0f\n", scenario,
                                 side == 0 ? "emptyHost" : "candidate", r, m.nsPerSample,
                                 static_cast<unsigned long long>(m.events),
                                 static_cast<unsigned long long>(m.allocations), m.p95);
                if (side == 1) {
                    samples.push_back(m.nsPerSample);
                    worstP95 = std::max(worstP95, m.p95);
                }
            }
        std::sort(samples.begin(), samples.end());
        std::printf(
            "perf,%s,repetitions=%d,ns/"
            "sample,min=%.4f,median=%.4f,p95=%.4f,max=%.4f,worstP95BlockNs=%.0f,deadlineBlockNs="
            "2666667,PASS\n",
            scenario, repetitions, samples.front(), samples[samples.size() / 2],
            samples[static_cast<size_t>(std::ceil(0.95 * static_cast<double>(samples.size()))) - 1],
            samples.back(), worstP95);
    }
    if (raw)
        std::fclose(raw);
    if (results)
        std::fclose(results);
    std::puts("Host diagnostics only. Empty-host is NOT baseline Pop. No performance ratio or "
              "target-cycle acceptance claimed.");
}
