#include "check.h"
#include "fixtures/internal.h"
#include "nt_host_driver.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
using namespace pop;
void flat(host::Instance &h, int count = 4) {
    h.set(CountParam, count);
    h.set(RateParam, 100);
    h.set(IrregularityParam, 0);
    h.set(ContourParam, 0);
}
void saveTrace(const char *name, const host::Trace &t) {
    const std::string path = std::string("build/") + name + ".csv";
    auto *file = std::fopen(path.c_str(), "w");
    CHECK(file);
    std::fprintf(file, "sample,burstV,activeV\n");
    for (size_t i = 0; i < t.burst.size(); ++i)
        std::fprintf(file, "%zu,%.0f,%.0f\n", i, t.burst[i], t.active[i]);
    std::fclose(file);
}
void golden() {
    for (const auto &f : fixture::cases) {
        if (f.duration == 4801 || f.duration == 4799)
            continue;
        for (const int partition : {4, 28, 64, 128}) {
            host::Instance h;
            h.set(CountParam, f.n);
            h.set(RateParam, f.rate * 10);
            h.set(ContourParam, f.contour);
            h.set(AmountParam, f.amount);
            h.set(IrregularityParam, f.irregularity);
            if (f.duration) {
                h.set(TerminationParam, 1);
                h.set(DurationParam, f.duration * 1000 / 48000);
            }
            injectSeed(h.algorithm, f.seed);
            const auto t = h.run(static_cast<int>((f.end + 1003) / 4 * 4), partition, {0});
            CHECK(t.rises == std::vector<uint64_t>(f.rises, f.rises + f.size));
            for (size_t i = 0; i < t.active.size(); ++i)
                CHECK(t.active[i] == (i < f.end ? 5.0f : 0.0f));
            CHECK(inspectEngine(h.algorithm).requestState() == f.request);
            CHECK(inspectEngine(h.algorithm).generationState() == f.generation);
            CHECK(inspectEngine(h.algorithm).generationDraws() == static_cast<uint64_t>(f.draws));
            if (partition == 128 && f.n == 4 && f.duration == 0)
                saveTrace("01-baseline-count", t);
            if (partition == 128 && f.duration)
                saveTrace(f.name, t);
            std::printf(
                "live,golden,%s,partition=%d,rises=%zu,request=%u,generation=%u,draws=%llu,PASS\n",
                f.name, partition, t.rises.size(), inspectEngine(h.algorithm).requestState(),
                inspectEngine(h.algorithm).generationState(),
                static_cast<unsigned long long>(inspectEngine(h.algorithm).generationDraws()));
        }
    }
}
void actions() {
    host::Instance h;
    flat(h, 1);
    h.set(ManualParam, 1);
    h.process(4);
    CHECK(inspectEngine(h.algorithm).acceptedRequests() == 1);
    h.process(4);
    CHECK(inspectEngine(h.algorithm).acceptedRequests() == 1);
    h.set(RetriggerParam, 0);
    h.set(ManualParam, 0);
    h.process(4);
    h.set(ManualParam, 1);
    h.process(4);
    CHECK(inspectEngine(h.algorithm).acceptedRequests() == 1);
    h.run(6000, 128, {});
    h.process(4);
    CHECK(inspectEngine(h.algorithm).acceptedRequests() == 1);
    h.set(ManualParam, 0);
    h.process(4);
    h.set(ManualParam, 1);
    std::fill(h.buffers.begin(), h.buffers.end(), 0);
    h.buffers[0] = 5;
    h.factory->step(h.algorithm, h.buffers.data(), 1);
    CHECK(inspectEngine(h.algorithm).acceptedRequests() == 2);
    const auto request = inspectEngine(h.algorithm).requestState();
    CHECK(h.load());
    h.process(4);
    CHECK(inspectEngine(h.algorithm).phase() == Phase::Idle &&
          inspectEngine(h.algorithm).requestState() == request);
    h.process(4);
    CHECK(inspectEngine(h.algorithm).acceptedRequests() == 2);
    h.set(ManualParam, 0);
    h.process(4);
    h.set(ManualParam, 1);
    h.process(4);
    CHECK(inspectEngine(h.algorithm).acceptedRequests() == 3);
    host::Instance held;
    flat(held, 1);
    CHECK(held.load());
    held.set(ManualParam, 1);
    held.process(4);
    CHECK(inspectEngine(held.algorithm).acceptedRequests() == 0);
    held.set(ManualParam, 0);
    held.process(4);
    held.set(ManualParam, 1);
    held.process(4);
    CHECK(inspectEngine(held.algorithm).acceptedRequests() == 1);
    host::Instance input;
    flat(input, 1);
    CHECK(input.load());
    for (float v : {5.0f, 1.5f, std::numeric_limits<float>::quiet_NaN(), 2.0f}) {
        std::fill(input.buffers.begin(), input.buffers.end(), v);
        input.factory->step(input.algorithm, input.buffers.data(), 1);
    }
    CHECK(inspectEngine(input.algorithm).acceptedRequests() == 0);
    for (float v : {1.0f, 1.5f, 2.0f, 5.0f, std::numeric_limits<float>::infinity(), 5.0f}) {
        std::fill_n(input.buffers.data(), 256, v);
        input.factory->step(input.algorithm, input.buffers.data(), 1);
    }
    CHECK(inspectEngine(input.algorithm).acceptedRequests() == 1);
    std::puts("live,05-actions,manual/held/ignored/simultaneous/nonfinite/hysteresis,PASS");
}
void routing() {
    for (const int trigger : {0, 1, 13, 64}) {
        host::Instance h;
        flat(h, 1);
        h.set(TriggerInputParam, trigger);
        h.set(BurstOutputParam, trigger == 13 ? 15 : 13);
        h.set(ActiveOutputParam, trigger == 64 ? 0 : 64);
        h.process(4);
        const auto t = h.run(6000, 28, {0}, trigger);
        if (trigger)
            CHECK(t.rises == std::vector<uint64_t>{4800});
        else {
            CHECK(t.rises.empty());
            h.set(ManualParam, 1);
            h.process(4);
            CHECK(inspectEngine(h.algorithm).acceptedRequests() == 1);
        }
    }
    for (const auto pair : {std::pair<int, int>{BurstOutputParam, 0},
                            {BurstOutputParam, 65},
                            {ActiveOutputParam, 13},
                            {ActiveOutputParam, 1},
                            {ClockInputParam, 13},
                            {TriggerInputParam, -1},
                            {TriggerInputParam, 65}}) {
        host::Instance h;
        flat(h, 1);
        h.set(pair.first, pair.second);
        h.process(128, 3.0f);
        CHECK(inspectEngine(h.algorithm).error() == Error::InvalidRouting);
        CHECK(inspectEngine(h.algorithm).acceptedRequests() == 0);
        host::draw(h);
        CHECK(std::find(h.display.begin(), h.display.end(), "Invalid routing") != h.display.end());
        for (int i = 0; i < 128; ++i)
            CHECK(h.buffers[static_cast<size_t>(13 * 128 + i)] == 0.0f ||
                  pair.first == ActiveOutputParam);
    }
    host::Instance h;
    flat(h, 1);
    h.set(BurstModeParam, 0);
    h.set(ActiveModeParam, 0);
    h.process(4, 0);
    h.set(ManualParam, 1);
    for (int j = 0; j < 1300; ++j) {
        h.process(4, 3.0f);
        for (int i = 0; i < 4; ++i) {
            const auto t = static_cast<uint64_t>(j * 4 + i);
            CHECK(h.buffers[static_cast<size_t>(12 * 4 + i)] ==
                  (t >= 4800 && t < 5040 ? 8.0f : 3.0f));
            CHECK(h.buffers[static_cast<size_t>(13 * 4 + i)] == (t < 5040 ? 8.0f : 3.0f));
        }
    }
    h.set(TerminationParam, 1);
    CHECK(h.gray[CountParam] && !h.gray[DurationParam]);
    h.set(RateParam, 123);
    h.set(DurationParam, 1234);
    h.set(TimingParam, 1);
    CHECK(h.values[RateParam] == 0 && h.values[DurationParam] == 2 && h.definitionUpdates == 2);
    h.set(RateParam, -2);
    h.set(DurationParam, 6);
    h.set(TimingParam, 0);
    CHECK(h.values[RateParam] == 123 && h.values[DurationParam] == 1234 && h.gray[ClockInputParam]);
    h.set(TimingParam, 1);
    CHECK(h.values[RateParam] == -2 && h.values[DurationParam] == 6 && !h.gray[ClockInputParam]);
    char buffer[64]{};
    CHECK(h.factory->parameterString(h.algorithm, RateParam, -2, buffer) == 2 &&
          std::string(buffer) == "/4");
    h.process(4);
    h.set(ManualParam, 0);
    h.process(4);
    h.set(ManualParam, 1);
    h.process(4);
    CHECK(inspectEngine(h.algorithm).error() == Error::NoClock);
    const auto json = h.save();
    CHECK(json.size() == 4);
    for (const auto &member : json)
        CHECK(member.first.find("seed") == std::string::npos &&
              member.first.find("phase") == std::string::npos);
    host::Instance loaded;
    CHECK(loaded.load(json));
    loaded.set(TimingParam, 1);
    CHECK(loaded.values[RateParam] == -2 && loaded.values[DurationParam] == 6);
    CHECK(h.factory->guid == NT_MULTICHAR('T', 'h', 'P', 'o'));
    CHECK(h.algorithm->parameterPages->numPages == 3);
    const auto &shape = h.algorithm->parameterPages->pages[1];
    const auto &routing = h.algorithm->parameterPages->pages[2];
    CHECK(std::string(shape.name) == "Shape" && shape.numParams == 3);
    CHECK(std::string(routing.name) == "Routing" && routing.numParams == 7);
    const std::array<uint8_t, 7> routingOrder = {
        TriggerInputParam, TimingParam, ClockInputParam, BurstOutputParam,
        BurstModeParam, ActiveOutputParam, ActiveModeParam};
    for (size_t j = 0; j < routingOrder.size(); ++j)
        CHECK(routing.params[j] == routingOrder[j]);
    std::array<int, NumParameters> visited{};
    for (uint32_t p = 0; p < 3; ++p) {
        const auto &page = h.algorithm->parameterPages->pages[p];
        for (int j = 0; j < page.numParams; ++j) {
            CHECK(page.params[j] < NumParameters);
            ++visited[page.params[j]];
        }
    }
    for (int v : visited)
        CHECK(v == 1);
    std::puts("live,06-routing-ui,None/bounds/aliases/Add/Replace/retention/pages/status,PASS");
}
void lifecycle() {
    for (const int p : {TimingParam, TriggerInputParam, ClockInputParam, BurstOutputParam,
                        BurstModeParam, ActiveOutputParam, ActiveModeParam}) {
        host::Instance h;
        flat(h, 4);
        h.run(4900, 28, {0});
        const auto rng = inspectEngine(h.algorithm).requestState();
        int value = h.values[static_cast<size_t>(p)] + 1;
        if (p == BurstModeParam || p == ActiveModeParam)
            value = 0;
        h.set(p, value);
        h.process(4);
        CHECK(inspectEngine(h.algorithm).phase() == Phase::Idle &&
              inspectEngine(h.algorithm).requestState() == rng);
    }
    host::Instance limited;
    flat(limited, 64);
    limited.set(RateParam, 1000);
    limited.set(ContourParam, 2);
    limited.set(AmountParam, 100);
    limited.run(2000, 4, {0});
    CHECK(inspectEngine(limited.algorithm).limits() & PhysicalLimited);
    host::draw(limited);
    CHECK(std::find(limited.display.begin(), limited.display.end(), "Interval limited") !=
          limited.display.end());
    limited.run(40000, 128, {});
    CHECK(inspectEngine(limited.algorithm).limits() & PhysicalLimited);
    limited.set(CountParam, 1);
    limited.set(ContourParam, 0);
    limited.run(1000, 4, {0});
    CHECK(inspectEngine(limited.algorithm).limits() == 0);
    host::Instance ignored;
    flat(ignored, 4);
    ignored.set(RetriggerParam, 0);
    const auto ignoredTrace = ignored.run(20000, 28, {0, 100, 4800, 4900, 9840});
    CHECK(ignoredTrace.rises == std::vector<uint64_t>({4800, 9600, 14400, 19200}));
    CHECK(inspectEngine(ignored.algorithm).acceptedRequests() == 1 &&
          inspectEngine(ignored.algorithm).requestState() == 270369);
    host::Instance rejected;
    flat(rejected, 0);
    const auto rejectedTrace = rejected.run(6000, 4, {0});
    CHECK(rejectedTrace.rises.empty() && inspectEngine(rejected.algorithm).requestState() == 1);
    rejected.set(TerminationParam, 1);
    rejected.set(DurationParam, 200);
    const auto admitted = rejected.run(10000, 4, {0});
    CHECK(admitted.rises == std::vector<uint64_t>{4800});
    host::Instance h;
    flat(h, 4);
    const auto first = h.run(4900, 4, {0});
    CHECK(first.burst.back() == 5);
    const auto rng = inspectEngine(h.algorithm).requestState();
    const auto json = h.save();
    CHECK(h.load(json));
    h.process(4);
    CHECK(inspectEngine(h.algorithm).requestState() == rng &&
          inspectEngine(h.algorithm).phase() == Phase::Idle);
    const auto second = h.run(20000, 64, {0});
    CHECK(second.rises == std::vector<uint64_t>({4800, 9600, 14400, 19200}));
    CHECK(inspectEngine(h.algorithm).requestState() == 67634689);
    CHECK(!h.load({{"internalRate", 0}}));
    h.process(4);
    CHECK(inspectEngine(h.algorithm).phase() == Phase::Idle);
    std::puts(
        "live,10-lifecycle,load/safety/reconstruction/partition,physical-bypass-pending,PASS");
}
void restartSnapshot() {
    for (int term : {0, 1})
        for (int policy : {0, 1}) {
            host::Instance h;
            flat(h, 2);
            h.set(RateParam, 1000);
            h.set(TerminationParam, term);
            h.set(DurationParam, 200);
            h.set(RetriggerParam, policy);
            host::Trace trace;
            for (int base = 0; base < 11000; base += 4) {
                if (base == 580)
                    h.set(RateParam, 100);
                std::fill_n(h.buffers.data(), 256, 0.0f);
                for (int i = 0; i < 4; ++i)
                    if (base + i == 0 || base + i == 580 || base + i == 680)
                        h.buffers[static_cast<size_t>(i)] = 5;
                h.factory->step(h.algorithm, h.buffers.data(), 1);
                for (int i = 0; i < 4; ++i) {
                    const int t = base + i;
                    const bool expectedBurst =
                        policy ? ((t >= 480 && t < 720) || (t >= 5568 && t < 5808) ||
                                  (!term && t >= 10368 && t < 10608))
                               : (term ? (t >= 480 && t < 9360 && t % 480 < 240)
                                       : ((t >= 480 && t < 720) || (t >= 960 && t < 1200)));
                    const bool expectedActive =
                        policy ? (t < 720 || (t >= 768 && t < (term ? 10368 : 10608)))
                               : t < (term ? 9600 : 1200);
                    CHECK(h.buffers[static_cast<size_t>(48 + i)] == (expectedBurst ? 5.0f : 0.0f));
                    CHECK(h.buffers[static_cast<size_t>(52 + i)] == (expectedActive ? 5.0f : 0.0f));
                    trace.burst.push_back(h.buffers[static_cast<size_t>(48 + i)]);
                    trace.active.push_back(h.buffers[static_cast<size_t>(52 + i)]);
                }
            }
            CHECK(inspectEngine(h.algorithm).acceptedRequests() == (policy ? 3u : 1u));
            if (policy && term == 0)
                saveTrace("05-restart", trace);
        }
    for (int term : {0, 1}) {
        host::Instance h;
        flat(h, 4);
        h.set(TerminationParam, term);
        h.set(DurationParam, 200);
        const auto before = h.run(100, 4, {0});
        CHECK(before.active.back() == 5);
        h.set(CountParam, 1);
        h.set(DurationParam, 100);
        h.set(RateParam, 1000);
        h.set(IrregularityParam, 100);
        h.set(ContourParam, 4);
        const auto after = h.run(20000, 28, {});
        const std::vector<uint64_t> expected =
            term ? std::vector<uint64_t>{4700} : std::vector<uint64_t>{4700, 9500, 14300, 19100};
        CHECK(after.rises == expected);
        CHECK(after.active[static_cast<size_t>((term ? 9600 : 19440) - 101)] == 5);
        CHECK(after.active[static_cast<size_t>((term ? 9600 : 19440) - 100)] == 0);
    }
    host::Instance automatic(false);
    CHECK(inspectEngine(automatic.algorithm).requestState() == 1);
    host::cycleCount = 0x6d2b79f7u;
    host::Instance automatic2(false);
    CHECK(inspectEngine(automatic2.algorithm).requestState() == 2);
    host::cycleCount = 0x6d2b79f5u;
    host::Instance normalized(false);
    CHECK(inspectEngine(normalized.algorithm).requestState() == 0x6d2b79f5u);
    host::cycleCount = 0x6d2b79f4u;
    std::puts("live,05-restart-snapshot,Count+Duration/both-policies/exact-voltage/snapshot/"
              "auto-seed,PASS");
}
void completionBoundaries() {
    // Integer division independently gives ceil(480000/99)=4849 and ceil(480000/73)=6576.
    for (const auto row :
         {std::array<int, 4>{99, 4748, 47, 227903}, std::array<int, 4>{73, 137, 0, 6576},
          std::array<int, 4>{99, 101, 0, 4849}}) {
        host::Instance h;
        flat(h, 1);
        h.set(TerminationParam, 1);
        h.set(RateParam, row[0]);
        h.set(DurationParam, row[1]);
        const auto trace = h.run((row[3] + 1243) / 4 * 4, 128, {0});
        CHECK(trace.rises.size() == static_cast<size_t>(row[2]));
        if (row[2]) {
            CHECK(trace.rises.back() == 227903 && trace.falls.back() == 228143);
            CHECK(trace.active[228142] == 5 && trace.active[228143] == 0);
        } else {
            CHECK(trace.active[static_cast<size_t>(row[1] * 48 - 1)] == 5 &&
                  trace.active[static_cast<size_t>(row[1] * 48)] == 0);
        }
    }
    for (int policy : {0, 1})
        for (uint64_t request : {4848ull, 5040ull}) {
            host::Instance h;
            flat(h, 1);
            h.set(TerminationParam, 1);
            h.set(DurationParam, 101);
            h.set(RateParam, 1000);
            h.set(RetriggerParam, policy);
            const auto trace = h.run(12000, 28, {0, request});
            std::vector<uint64_t> expected = {480,  960,  1440, 1920, 2400,
                                              2880, 3360, 3840, 4320, 4800};
            const bool accepted = policy || request == 5040;
            if (accepted)
                for (uint64_t t : {5568ull, 6048ull, 6528ull, 7008ull, 7488ull, 7968ull, 8448ull,
                                   8928ull, 9408ull, 9888ull})
                    expected.push_back(t);
            CHECK(trace.rises == expected);
            CHECK(inspectEngine(h.algorithm).acceptedRequests() == (accepted ? 2u : 1u));
            CHECK(trace.active[5039] == 5 && trace.active[5040] == 0 && trace.active[5087] == 0);
            CHECK(trace.active[5088] == (accepted ? 5.0f : 0.0f));
        }
    for (int policy : {0, 1}) {
        host::Instance h;
        flat(h, 1);
        h.set(RetriggerParam, policy);
        const auto trace = h.run(16000, 4, {0, 5040});
        CHECK(trace.rises == std::vector<uint64_t>({4800, 9888}));
        CHECK(inspectEngine(h.algorithm).acceptedRequests() == 2);
    }
    host::Instance live;
    flat(live, 2);
    live.set(RateParam, 1000);
    live.run(580, 4, {0});
    live.set(RateParam, 100);
    live.run(100, 4, {0});
    live.set(RetriggerParam, 0);
    const auto tail = live.run(10000, 4, {0});
    CHECK(tail.rises == std::vector<uint64_t>({4888, 9688}));
    CHECK(inspectEngine(live.algorithm).acceptedRequests() == 2);
    std::puts("live,05-completion,deadline-high-drain/final-fall/both-policies/"
              "live-Off-retains-pending,PASS");
}
void statistics() {
    auto *file = std::fopen("build/adapter-seed-distributions.csv", "w");
    CHECK(file);
    std::fprintf(file, "seed,contour,termination,count,earlySum,lateSum,outer,middle\n");
    uint64_t de = 0, dl = 0, ae = 0, al = 0, rm = 0, ro = 0, fm = 0, fo = 0;
    int minCount = 10000, maxCount = 0;
    uint64_t flatMin = 100000, flatMax = 0;
    for (uint32_t seed = 1; seed <= 1024; ++seed)
        for (int c = 0; c < 5; ++c) {
            host::Instance h;
            flat(h, 32);
            h.set(ContourParam, c);
            h.set(AmountParam, 100);
            h.set(IrregularityParam, 50);
            injectSeed(h.algorithm, seed);
            const auto count = h.run(48000 * 8, 128, {0});
            CHECK(count.rises.size() == 32);
            uint64_t early = 0, late = 0;
            for (size_t i = 1; i < 32; ++i) {
                const auto interval = count.rises[i] - count.rises[i - 1];
                if (i <= 7)
                    early += interval;
                if (i >= 25)
                    late += interval;
                if (c == 0) {
                    flatMin = std::min(flatMin, interval);
                    flatMax = std::max(flatMax, interval);
                }
            }
            if (c == 1) {
                ae += early;
                al += late;
            }
            if (c == 2) {
                de += early;
                dl += late;
            }
            std::fprintf(file, "%u,%d,Count,32,%llu,%llu,0,0\n", seed, c,
                         static_cast<unsigned long long>(early),
                         static_cast<unsigned long long>(late));
            h.set(TerminationParam, 1);
            h.set(DurationParam, 2000);
            h.set(RateParam, 200);
            injectSeed(h.algorithm, seed);
            const auto duration = h.run(96240, 128, {0});
            int outer = 0, middle = 0;
            for (auto t : duration.rises) {
                if (t < 24000 || t >= 72000)
                    ++outer;
                else
                    ++middle;
            }
            if (c == 3) {
                ro += static_cast<uint64_t>(outer);
                rm += static_cast<uint64_t>(middle);
            }
            if (c == 4) {
                fo += static_cast<uint64_t>(outer);
                fm += static_cast<uint64_t>(middle);
            }
            const int n = static_cast<int>(duration.rises.size());
            minCount = std::min(minCount, n);
            maxCount = std::max(maxCount, n);
            std::fprintf(file, "%u,%d,Duration,%d,0,0,%d,%d\n", seed, c, n, outer, middle);
        }
    std::fclose(file);
    CHECK(dl > 2 * de && 2 * al < ae && rm > ro && fo > fm && minCount < maxCount &&
          flatMin < flatMax);
    std::printf("live,04-distributions,seeds=1024,decRatio=%.9f,accRatio=%.9f,riseMiddle=%llu,"
                "riseOuter=%llu,fallMiddle=%llu,fallOuter=%llu,minCount=%d,maxCount=%d,"
                "flatIntervalMin=%llu,flatIntervalMax=%llu,PASS\n",
                static_cast<double>(dl) / static_cast<double>(de),
                static_cast<double>(al) / static_cast<double>(ae),
                static_cast<unsigned long long>(rm), static_cast<unsigned long long>(ro),
                static_cast<unsigned long long>(fm), static_cast<unsigned long long>(fo), minCount,
                maxCount, static_cast<unsigned long long>(flatMin),
                static_cast<unsigned long long>(flatMax));
}
int main(int argc, char **) {
    golden();
    actions();
    routing();
    lifecycle();
    restartSnapshot();
    completionBoundaries();
    if (argc == 1)
        statistics();
    std::puts("D1 adapter tested lanes1..6,10; Clock lanes7..9 NOT IMPLEMENTED; hardware NOT RUN");
}
