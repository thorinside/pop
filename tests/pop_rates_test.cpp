#include "check.h"
#include "nt_host_driver.h"
#include <algorithm>
#include <cstdio>
using namespace pop;
namespace {
uint32_t referenceAdvance(uint32_t x) {
    x ^= x << 13;
    x ^= x >> 17;
    return x ^ (x << 5);
}
void completeTrace(const host::Trace &t, const std::vector<uint64_t> &rises,
                   const std::vector<std::pair<uint64_t, uint64_t>> &active,
                   uint64_t width, uint64_t gap) {
    CHECK(t.rises == rises && t.falls.size() == rises.size());
    for (size_t i = 0; i < rises.size(); ++i) {
        CHECK(t.falls[i] == rises[i] + width);
        if (i) CHECK(rises[i] - t.falls[i - 1] >= gap);
    }
    size_t pulse = 0;
    for (size_t i = 0; i < t.burst.size(); ++i) {
        while (pulse < rises.size() && i >= rises[pulse] + width) ++pulse;
        const bool high = pulse < rises.size() && i >= rises[pulse];
        const bool busy = std::any_of(active.begin(), active.end(), [i](const auto &w) {
            return i >= w.first && i < w.second;
        });
        CHECK(t.burst[i] == (high ? 5.0f : 0.0f));
        CHECK(t.active[i] == (busy ? 5.0f : 0.0f));
    }
}
void lifetimes(uint32_t fs, uint64_t width, uint64_t gap) {
    for (int lane : {0, 1, 2}) {
        host::Instance h;
        h.set(ContourParam, 0);
        h.set(IrregularityParam, 0);
        h.set(CountParam, 64);
        h.set(TerminationParam, lane == 2 ? 0 : 1);
        h.set(DurationParam, lane == 0 ? 100 : 30000);
        h.set(RateParam, lane == 1 ? 1000 : 10);
        const uint64_t interval = lane == 1 ? (fs + 99) / 100 : fs;
        const uint64_t deadline = lane == 0 ? (fs + 9) / 10 : 30ull * fs;
        const uint64_t count = lane == 2 ? 64 : (deadline - 1) / interval;
        std::vector<uint64_t> rises;
        for (uint64_t n = 1; n <= count; ++n) rises.push_back(n * interval);
        const uint64_t end = lane == 2 ? 64ull * fs + width
                                      : std::max(deadline, count ? rises.back() + width : 0);
        const int samples = static_cast<int>((end + gap + 131) / 4 * 4);
        const auto t = h.run(samples, 128, {0});
        completeTrace(t, rises, {{0, end}}, width, gap);
        const auto &e = inspectEngine(h.algorithm);
        const auto draws = lane == 2 ? count : count + 1;
        uint32_t generation = 270369;
        for (uint64_t i = 0; i < draws; ++i) generation = referenceAdvance(generation);
        CHECK(e.phase() == Phase::Idle && e.acceptedRequests() == 1 && e.requestState() == 270369);
        CHECK(e.generationDraws() == draws && e.generationState() == generation);
        std::printf("lifetime,Fs=%u,lane=%s,count=%llu,interval=%llu,deadline=%llu,"
                    "lastFall=%llu,ActiveEnd=%llu,draws=%llu,generation=%u,full-voltage=PASS\n",
                    fs, lane == 0 ? "min-Duration" : lane == 1 ? "max-Duration" : "slow-Count64",
                    static_cast<unsigned long long>(count), static_cast<unsigned long long>(interval),
                    static_cast<unsigned long long>(deadline),
                    static_cast<unsigned long long>(count ? rises.back() + width : 0),
                    static_cast<unsigned long long>(end), static_cast<unsigned long long>(draws), generation);
    }
}
void separatedAndRestarted(uint32_t fs, uint64_t width, uint64_t gap) {
    const uint64_t interval = (fs + 99) / 100;
    const uint64_t deadline = (101ull * fs + 999) / 1000;
    for (int term : {0, 1})
        for (int separate : {0, 1})
            for (int policy : {0, 1}) {
                host::Instance h;
                h.set(CountParam, 2);
                h.set(RateParam, 1000);
                h.set(ContourParam, 0);
                h.set(IrregularityParam, 0);
                h.set(TerminationParam, term);
                h.set(DurationParam, 101);
                h.set(RetriggerParam, policy);
                const uint64_t count = term ? (deadline - 1) / interval : 2;
                const uint64_t end = term ? std::max(deadline, count * interval + width)
                                          : 2 * interval + width;
                const uint64_t request = separate ? end : interval + width / 2;
                const bool accepted = separate || policy;
                const uint64_t activation = separate ? end + gap : interval + width + gap;
                std::vector<uint64_t> rises;
                for (uint64_t n = 1; n <= (separate || !accepted ? count : 1); ++n)
                    rises.push_back(n * interval);
                if (accepted)
                    for (uint64_t n = 1; n <= count; ++n) rises.push_back(activation + n * interval);
                const uint64_t newEnd = activation + end;
                const std::vector<std::pair<uint64_t, uint64_t>> active = accepted
                    ? std::vector<std::pair<uint64_t, uint64_t>>{{0, separate ? end : interval + width},
                                                                {activation, newEnd}}
                    : std::vector<std::pair<uint64_t, uint64_t>>{{0, end}};
                const auto t = h.run(static_cast<int>((newEnd + gap + 131) / 4 * 4), 28, {0, request});
                completeTrace(t, rises, active, width, gap);
                const auto &e = inspectEngine(h.algorithm);
                const uint64_t privateDraws = count + (term ? 1u : 0u);
                const uint64_t draws = !accepted ? privateDraws
                    : (separate ? 2 * privateDraws : 2 + privateDraws);
                const uint32_t rng = accepted ? 67634689 : 270369;
                uint32_t generation = rng;
                for (uint64_t i = 0; i < privateDraws; ++i) generation = referenceAdvance(generation);
                CHECK(e.acceptedRequests() == (accepted ? 2u : 1u) && e.requestState() == rng);
                CHECK(e.generationDraws() == draws && e.generationState() == generation);
                CHECK(e.phase() == Phase::Idle);
                std::printf("rate-boundary,Fs=%u,term=%d,separate=%d,policy=%d,request=%llu,"
                            "activation=%llu,width=%llu,gap=%llu,events=%zu,draws=%llu,"
                            "full-voltage/Active/final-drain=PASS\n", fs, term, separate, policy,
                            static_cast<unsigned long long>(request),
                            static_cast<unsigned long long>(activation),
                            static_cast<unsigned long long>(width), static_cast<unsigned long long>(gap),
                            rises.size(), static_cast<unsigned long long>(draws));
            }
}
}
int main() {
    for (uint32_t fs : {32000u, 44100u, 48000u, 96000u}) {
        host::sampleRate(fs);
        const uint64_t width = (5ull * fs + 999) / 1000, gap = (fs + 999) / 1000;
        lifetimes(fs, width, gap);
        separatedAndRestarted(fs, width, gap);
        for (int partition = 4; partition <= 128; partition += 4)
            for (int term : {0, 1}) {
                host::Instance h;
                h.set(CountParam, 64);
                h.set(RateParam, 1000);
                h.set(ContourParam, 0);
                h.set(IrregularityParam, 0);
                h.set(TerminationParam, term);
                h.set(DurationParam, 101);
                const auto t = h.run(static_cast<int>(fs), partition, {0});
                const uint64_t interval = (fs + 99) / 100;
                const uint64_t deadline = (101ull * fs + 999) / 1000;
                const size_t expected = term ? static_cast<size_t>((deadline - 1) / interval) : 64;
                CHECK(t.rises.size() == expected && t.falls.size() == expected);
                for (size_t i = 0; i < expected; ++i) {
                    CHECK(t.rises[i] == (i + 1) * interval);
                    CHECK(t.falls[i] == t.rises[i] + width);
                    if (i)
                        CHECK(t.rises[i] - t.falls[i - 1] >= gap);
                }
                const uint64_t end = term ? std::max(deadline, t.falls.back()) : t.falls.back();
                for (size_t i = 0; i < t.active.size(); ++i)
                    CHECK(t.active[i] == (i < end ? 5.0f : 0.0f));
            }
        std::printf("adapter-rate,%u,all-quarter-frame-partitions=4..128,Count64+Duration101ms,"
                    "width=%llu,gap=%llu,PASS\n",
                    fs, static_cast<unsigned long long>(width),
                    static_cast<unsigned long long>(gap));
    }
    host::sampleRate(48000);
    host::Instance h;
    h.set(ContourParam, 0);
    h.set(IrregularityParam, 0);
    h.set(RateParam, 1000);
    const auto t = h.run(600, 4, {0});
    CHECK(t.burst.back() == 5);
    const auto rng = inspectEngine(h.algorithm).requestState();
    host::sampleRate(44100);
    h.process(4);
    CHECK(inspectEngine(h.algorithm).phase() == Phase::Idle &&
          inspectEngine(h.algorithm).requestState() == rng);
    CHECK(h.buffers[48] == 0 && h.buffers[52] == 0);
    std::fill(h.buffers.begin(), h.buffers.end(), 7);
    for (int by4 : {-1, 0, 33, 2147483647}) {
        h.step(by4);
        for (float v : h.buffers)
            CHECK(v == 7);
    }
    host::sampleRate(48000);
    std::puts("adapter observed-Fs-reset and frame-boundary validation PASS; physical rate support "
              "NOT CLAIMED");
}
