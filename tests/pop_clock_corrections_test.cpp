#include "nt_host_driver.h"
#include "check.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
using namespace pop;

void clampedEquality(uint32_t fs, int period, int rate, const std::vector<int> &expectedRises) {
    host::sampleRate(fs);
    host::Instance h;
    h.set(TimingParam, 1); h.set(ClockInputParam, 2); h.set(CountParam, 3);
    h.set(RateParam, rate); h.set(ContourParam, 1); h.set(AmountParam, 100);
    h.set(IrregularityParam, 100); h.set(RetriggerParam, 0);
    h.process(4); injectSeed(h.algorithm, 515213918u);
    std::vector<int> rises, falls;
    bool previous = false;
    const int width = static_cast<int>((5ull * fs + 999) / 1000);
    const int end = (expectedRises.back() + width + 103) & ~3;
    for (int base = 0; base < end; base += 128) {
        const int frames = std::min(128, end - base);
        std::fill_n(h.buffers.data(), 64 * frames, 0.0f);
        for (int i = 0; i < frames; ++i) {
            const int t = base + i;
            if (t == 200) h.buffers[static_cast<size_t>(i)] = 5;
            if (t >= 100 && (t - 100) % period == 0)
                h.buffers[static_cast<size_t>(frames + i)] = 5;
        }
        h.step(frames / 4);
        for (int i = 0; i < frames; ++i) {
            const bool burst = h.buffers[static_cast<size_t>(12 * frames + i)] == 5;
            if (std::getenv("POP_CORRECTION_TRACE"))
                std::printf("L1-row,%u,%d,%d,%d,%.0f,%.0f\n", fs, period, rate, base + i,
                    static_cast<double>(h.buffers[static_cast<size_t>(12 * frames + i)]),
                    static_cast<double>(h.buffers[static_cast<size_t>(13 * frames + i)]));
            if (burst && !previous) rises.push_back(base + i);
            if (!burst && previous) falls.push_back(base + i);
            previous = burst;
        }
    }
    std::printf("L1 Fs%u P%d rate%d expected", fs, period, rate);
    for (int t : expectedRises) std::printf(" %d", t);
    std::printf("; actual");
    for (int t : rises) std::printf(" %d", t);
    std::puts(""); std::fflush(stdout);
    CHECK(rises == expectedRises);
    std::vector<int> expectedFalls;
    for (int t : expectedRises) expectedFalls.push_back(t + width);
    CHECK(falls == expectedFalls);
    const auto &e = inspectEngine(h.algorithm);
    CHECK(e.acceptedRequests() == 1 && e.generationDraws() == 3);
    CHECK(e.generationState() == 3758096384u);
    const bool limited = rate == 3 ? 100ull * static_cast<unsigned>(period) < 8ull * fs
                                   : 8ull * static_cast<unsigned>(period) > fs;
    CHECK(e.limits() == (limited ? BaseLimited : 0));
}

void clampedDuration() {
    for (uint32_t fs : {32015u, 32016u, 32017u}) for (int index : {0, 1}) {
        host::sampleRate(fs);
        host::Instance h;
        h.set(TimingParam, 1); h.set(ClockInputParam, 2); h.set(RateParam, 3);
        h.set(TerminationParam, 1); h.set(DurationParam, index); h.set(IrregularityParam, 100);
        h.process(4); injectSeed(h.algorithm, 1152556007u);
        const bool admitted = index == 1 && fs <= 32016;
        const int deadline = index == 0 ? 2602 : 3102;
        for (int base = 0; base < 4000; base += 4) {
            std::fill_n(h.buffers.data(), 256, 0.0f);
            for (int i = 0; i < 4; ++i) {
                const int t = base + i;
                if (t == 200) h.buffers[static_cast<size_t>(i)] = 5;
                if (t == 100 || t == 2101) h.buffers[static_cast<size_t>(4 + i)] = 5;
            }
            h.step(1);
            for (int i = 0; i < 4; ++i) {
                const int t = base + i;
                CHECK(h.buffers[static_cast<size_t>(48 + i)] == (admitted && t >= 2602 && t < 2763 ? 5 : 0));
                CHECK(h.buffers[static_cast<size_t>(52 + i)] == (t >= 2101 && t < deadline ? 5 : 0));
            }
        }
        const auto &e = inspectEngine(h.algorithm);
        CHECK(e.emitted() == (admitted ? 1u : 0u));
        CHECK(e.generationDraws() == (admitted ? 2u : 1u));
        std::printf("L1 Duration Fs%u index%d emitted=%u deadline=%d PASS\n", fs, index, e.emitted(), deadline);
    }
}

void reacquireDuringTail() {
    host::sampleRate(48000);
    host::Instance h;
    h.set(TimingParam, 1); h.set(ClockInputParam, 2); h.set(CountParam, 1);
    h.set(RateParam, 3); h.set(RetriggerParam, 0);
    h.process(4); injectSeed(h.algorithm, 1);
    for (int base = 0; base < 7000; base += 4) {
        std::fill_n(h.buffers.data(), 256, 0.0f);
        for (int i = 0; i < 4; ++i) {
            const int t = base + i;
            if (t == 100 || t == 3104) h.buffers[static_cast<size_t>(i)] = 5;
            if (t == 0 || t == 2400 || t == 3100 || t == 3104 || t == 5504)
                h.buffers[static_cast<size_t>(4 + i)] = 5;
        }
        h.step(1);
        for (int i = 0; i < 4; ++i) {
            const int t = base + i;
            const bool burst = (t >= 3000 && t < 3240) || (t >= 6104 && t < 6344);
            const bool active = (t >= 2400 && t < 3100) || (t >= 5504 && t < 6344);
            CHECK(h.buffers[static_cast<size_t>(48 + i)] == (burst ? 5 : 0));
            CHECK(h.buffers[static_cast<size_t>(52 + i)] == (active ? 5 : 0));
        }
    }
    const auto &e = inspectEngine(h.algorithm);
    CHECK(e.acceptedRequests() == 2 && e.generationDraws() == 2 && e.phase() == Phase::Idle);
    std::puts("L2 fresh Off during abort tail, coincident first fresh clock, later measured anchor5504/rise6104 PASS");
}

void ownershipControls() {
    for (bool cancel : {false, true}) for (bool finalFall : {false, true}) {
        Settings s; s.timing = Timing::Clock; s.clockRate = 3; s.count = 1;
        Engine e;
        const int invalid = finalFall ? 3240 : 3100;
        for (int t = 0; t <= 3300; ++t) {
            const auto o = e.tick(s, false, {t == 100 || t == invalid + 4, cancel && t == 3050, false,
                                          t == 0 || t == 2400 || t == invalid});
            CHECK(o.burst == (t >= 3000 && t < 3240));
            CHECK(o.active == (t >= 2400 && t < (cancel || finalFall ? 3240 : invalid)));
        }
        CHECK(e.acceptedRequests() == (cancel && !finalFall ? 1u : 2u));
        CHECK(e.error() == Error::None);
    }
    Settings s; s.timing = Timing::Clock;
    for (bool pending : {false, true}) {
        Engine e;
        for (int t = 0; t <= 200; ++t) {
            const auto o = e.tick(s, false, {pending && t == 50, false, false, t == 0 || t == 100});
            CHECK(!o.burst && !o.active && e.error() == Error::None);
        }
        CHECK(e.phase() == (pending ? Phase::Pending : Phase::Idle));
    }
    std::puts("L2 completion-first final fall, explicit-cancel ownership, Idle/Pending controls PASS");
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
            if (std::getenv("POP_CORRECTION_TRACE"))
                std::printf("L2-row,%d,%d,%d,%d,%.0f,%.0f,%d,%d\n", duration, expiry, fresh, t,
                    static_cast<double>(h.buffers[static_cast<size_t>(48 + i)]),
                    static_cast<double>(h.buffers[static_cast<size_t>(52 + i)]), burst ? 5 : 0, active ? 5 : 0);
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
    if (argc == 1 || std::string(argv[1]) == "L1") {
        clampedEquality(32015, 2001, 3, {2602,3602,3852});
        clampedEquality(32016, 2001, 3, {2602,3602,3852});
        clampedEquality(32017, 2001, 3, {2602,3602,4102});
        clampedEquality(32000, 2559, 3, {3299,3939,4259});
        clampedEquality(32000, 2560, 3, {3300,3940,4260});
        clampedEquality(32000, 2561, 3, {3302,3942,4262});
        clampedEquality(32016, 4001, -3, {68117,132133,164141});
        clampedEquality(32016, 4002, -3, {68134,132166,164182});
        clampedEquality(32016, 4003, -3, {68151,132199,164223});
        clampedDuration();
    }
    if (argc == 1 || std::string(argv[1]) == "L2") {
        for (bool duration : {false, true}) for (bool expiry : {false, true})
            for (bool fresh : {false, true}) finalTailAbort(duration, expiry, fresh);
        ownershipControls();
        reacquireDuringTail();
    }
    std::puts("Clock correction adapter regressions PASS");
}
