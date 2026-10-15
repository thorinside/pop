#include "nt_host_driver.h"
#include "check.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
using namespace pop;

void clampedEquality() {
    host::sampleRate(32016);
    host::Instance h;
    h.set(TimingParam, 1); h.set(ClockInputParam, 2); h.set(CountParam, 3);
    h.set(RateParam, 3); h.set(ContourParam, 1); h.set(AmountParam, 100);
    h.set(IrregularityParam, 100); h.set(RetriggerParam, 0);
    h.process(4); injectSeed(h.algorithm, 515213918u);
    std::vector<int> rises, falls;
    bool previous = false;
    for (int base = 0; base < 8000; base += 128) {
        const int frames = std::min(128, 8000 - base);
        std::fill_n(h.buffers.data(), 64 * frames, 0.0f);
        for (int i = 0; i < frames; ++i) {
            const int t = base + i;
            if (t == 200) h.buffers[static_cast<size_t>(i)] = 5;
            if (t >= 100 && (t - 100) % 2001 == 0)
                h.buffers[static_cast<size_t>(frames + i)] = 5;
        }
        h.step(frames / 4);
        for (int i = 0; i < frames; ++i) {
            const bool burst = h.buffers[static_cast<size_t>(12 * frames + i)] == 5;
            if (burst && !previous) rises.push_back(base + i);
            if (!burst && previous) falls.push_back(base + i);
            previous = burst;
        }
    }
    std::printf("L1 expected rises 2602 3602 3852; actual");
    for (int t : rises) std::printf(" %d", t);
    std::puts(""); std::fflush(stdout);
    CHECK((rises == std::vector<int>{2602, 3602, 3852}));
    CHECK((falls == std::vector<int>{2763, 3763, 4013}));
    const auto &e = inspectEngine(h.algorithm);
    CHECK(e.acceptedRequests() == 1 && e.generationDraws() == 3);
    CHECK(e.generationState() == 3758096384u && e.limits() == BaseLimited);
}

void finalTailAbort(bool duration, bool expiry, bool fresh) {
    host::sampleRate(48000);
    host::Instance h;
    h.set(TimingParam, 1); h.set(ClockInputParam, 2); h.set(RetriggerParam, 0);
    h.set(CountParam, 3); h.set(RateParam, duration ? 3 : 0);
    if (duration) {
        h.set(TerminationParam, 1); h.set(DurationParam, 4);
        h.set(IrregularityParam, 100);
    }
    h.process(4); injectSeed(h.algorithm, duration ? 66u : 1u);
    const std::vector<int> clocks = duration
        ? (expiry ? std::vector<int>{0, 2400, 4802}
                  : std::vector<int>{0, 2400, 4800, 7200, 9600, 12000, 12010})
        : (expiry ? std::vector<int>{100, 24050, 48000, 71950, 86400, 88800}
                  : std::vector<int>{100, 24050, 48000, 71950, 95900, 96000});
    const std::vector<int> expectedRises = duration
        ? std::vector<int>{2550,2850,3450,4050,4650,5250,5850,7050,7650,8850,9450,9750,10350,10950,11250,11850}
        : std::vector<int>{48000,71950,95900};
    const int detection = duration ? (expiry ? 12008 : 12010) : 96000;
    const int end = duration ? 12300 : 96500;
    unsigned activeMismatches = 0, burstMismatches = 0;
    for (int base = 0; base < end; base += 4) {
        std::fill_n(h.buffers.data(), 64 * 4, 0.0f);
        for (int i = 0; i < 4; ++i) {
            const int t = base + i;
            if (t == (duration ? 100 : 200) || (fresh && t == detection + 4))
                h.buffers[static_cast<size_t>(i)] = 5;
            if (std::find(clocks.begin(), clocks.end(), t) != clocks.end())
                h.buffers[static_cast<size_t>(4 + i)] = 5;
        }
        h.step(1);
        for (int i = 0; i < 4; ++i) {
            const int t = base + i;
            const bool burst = std::any_of(expectedRises.begin(), expectedRises.end(),
                [t](int rise) { return t >= rise && t < rise + 240; });
            const bool active = t >= (duration ? 2400 : 24050) && t < detection;
            burstMismatches += h.buffers[static_cast<size_t>(48 + i)] != (burst ? 5 : 0);
            activeMismatches += h.buffers[static_cast<size_t>(52 + i)] != (active ? 5 : 0);
        }
    }
    const auto &e = inspectEngine(h.algorithm);
    std::printf("L2 %s %s fresh=%d Burst mismatches=%u Active mismatches=%u accepted=%llu expected=%d\n",
        duration ? "Duration" : "Count", expiry ? "expiry" : "invalid", fresh,
        burstMismatches, activeMismatches, static_cast<unsigned long long>(e.acceptedRequests()), fresh ? 2 : 1);
    std::fflush(stdout);
    CHECK(burstMismatches == 0 && activeMismatches == 0);
    CHECK(e.acceptedRequests() == (fresh ? 2u : 1u));
    CHECK(e.error() == (fresh ? Error::None : Error::ClockStopped));
    CHECK(e.phase() == (fresh ? Phase::Pending : Phase::Idle));
}
int main(int argc, char **argv) {
    if (argc == 1 || std::string(argv[1]) == "L1") clampedEquality();
    if (argc == 1 || std::string(argv[1]) == "L2")
        for (bool duration : {false, true}) for (bool expiry : {false, true})
            for (bool fresh : {false, true}) finalTailAbort(duration, expiry, fresh);
    std::puts("Clock correction adapter regressions PASS");
}
