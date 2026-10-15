#include "check.h"
#include "nt_host_driver.h"
#include <algorithm>
#include <cstdio>
#include <vector>
using namespace pop;
namespace {
using Samples = std::vector<uint64_t>;
using Windows = std::vector<std::pair<uint64_t, uint64_t>>;
void configure(host::Instance &h, int term, int count = 2, int rate = 1000) {
    h.set(TerminationParam, term);
    h.set(CountParam, count);
    h.set(DurationParam, 101);
    h.set(RateParam, rate);
    h.set(ContourParam, 0);
    h.set(IrregularityParam, 0);
}
uint32_t referenceAdvance(uint32_t x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}
void verify(const char *name, const host::Trace &t, const Samples &rises, const Windows &active,
            const Engine &engine, uint64_t accepted, uint64_t draws, unsigned privateDraws) {
    Samples falls;
    for (auto rise : rises)
        falls.push_back(rise + 240);
    CHECK(t.rises == rises && t.falls == falls);
    for (size_t i = 0; i < t.burst.size(); ++i) {
        const bool high = std::any_of(rises.begin(), rises.end(), [i](uint64_t r) {
            return i >= r && i < r + 240;
        });
        const bool busy = std::any_of(active.begin(), active.end(), [i](const auto &w) {
            return i >= w.first && i < w.second;
        });
        CHECK(t.burst[i] == (high ? 5.0f : 0.0f));
        CHECK(t.active[i] == (busy ? 5.0f : 0.0f));
    }
    uint32_t request = 1;
    for (uint64_t i = 0; i < accepted; ++i)
        request = referenceAdvance(request);
    uint32_t generation = request;
    for (unsigned i = 0; i < privateDraws; ++i)
        generation = referenceAdvance(generation);
    CHECK(engine.phase() == Phase::Idle && engine.acceptedRequests() == accepted);
    CHECK(engine.requestState() == request && engine.generationState() == generation);
    CHECK(engine.generationDraws() == draws);
    std::printf("regression,%s,rises=", name);
    for (auto v : t.rises)
        std::printf("%llu/", static_cast<unsigned long long>(v));
    std::printf(",falls=");
    for (auto v : t.falls)
        std::printf("%llu/", static_cast<unsigned long long>(v));
    std::printf(",active=");
    for (const auto &w : active)
        std::printf("[%llu;%llu)/", static_cast<unsigned long long>(w.first),
                    static_cast<unsigned long long>(w.second));
    std::printf(",accepted=%llu,draws=%llu,request=%u,generation=%u,full-voltage-match=PASS\n",
                static_cast<unsigned long long>(accepted), static_cast<unsigned long long>(draws),
                request, generation);
}
void loadContext() {
    const host::Json json = {{"internalRate", 123}, {"clockRate", -2},
                             {"internalDuration", 1234}, {"clockDuration", 6}};
    for (bool beforeValues : {false, true})
        for (int timing : {0, 1}) {
            host::Instance h(true, 7);
            configure(h, 0);
            h.run(600, 4, {0});
            const auto request = inspectEngine(h.algorithm).requestState();
            const unsigned setters = h.audioSetterCalls;
            if (beforeValues)
                h.algorithm->v = nullptr;
            CHECK(h.load(json));
            CHECK(h.audioSetterCalls == setters);
            h.algorithm->v = h.values.data();
            h.set(TimingParam, timing);
            h.set(RateParam, timing ? 3 : 999);
            h.set(DurationParam, timing ? 7 : 29999);
            h.set(ManualParam, 1);
            CHECK(h.save() == json);
            h.process(4);
            CHECK(h.audioSetterCalls == setters + 2);
            CHECK(h.values[RateParam] == (timing ? -2 : 123));
            CHECK(h.values[DurationParam] == (timing ? 6 : 1234));
            CHECK(h.gray[ClockInputParam] == (timing == 0));
            CHECK(inspectEngine(h.algorithm).phase() == Phase::Idle);
            CHECK(inspectEngine(h.algorithm).requestState() == request);
            CHECK(inspectEngine(h.algorithm).generationState() == 0);
            CHECK(h.buffers[48] == 0 && h.buffers[52] == 0);
            h.process(4);
            CHECK(inspectEngine(h.algorithm).acceptedRequests() == 1);
            h.set(TimingParam, 1 - timing);
            CHECK(h.values[RateParam] == (timing ? 123 : -2));
            CHECK(h.values[DurationParam] == (timing ? 1234 : 6));
            h.set(TimingParam, 0);
            h.process(4);
            h.set(ManualParam, 0);
            h.process(4);
            h.set(ManualParam, 1);
            h.process(4);
            CHECK(inspectEngine(h.algorithm).acceptedRequests() == 2);
            CHECK(inspectEngine(h.algorithm).requestState() == 67634689);
            CHECK(!h.load({{"internalRate", 0}}));
            h.process(4);
            CHECK(inspectEngine(h.algorithm).phase() == Phase::Idle);
            std::printf("load-context,before-v=%d,timing=%d,common-offset=7,no-load-setters/"
                        "supported-step-sync/recursive-callbacks/RNG/arming/retention,PASS\n",
                        beforeValues, timing);
        }
}
void neutral() {
    for (int count : {1, 2})
        for (int contour : {1, 2})
            for (int partition : {4, 28, 128}) {
                host::Instance h;
                configure(h, 0, count, 100);
                h.set(ContourParam, contour);
                h.set(AmountParam, 100);
                const auto t = h.run(12000, partition, {0});
                verify("small-count-neutral", t, count == 1 ? Samples{4800} : Samples{4800, 9600},
                       {{0, count == 1 ? 5040 : 9840}}, inspectEngine(h.algorithm), 1,
                       static_cast<uint64_t>(count), static_cast<unsigned>(count));
            }
    host::Instance h;
    configure(h, 0, 2, 100);
    h.set(RetriggerParam, 0);
    const auto t = h.run(14000, 28, {0, 9600});
    verify("Count-Off-final-old-due", t, {4800, 9600}, {{0, 9840}},
           inspectEngine(h.algorithm), 1, 2, 2);
}
struct Boundary {
    const char *name;
    int term;
    uint64_t request, activation, oldActiveEnd;
    Samples prior;
};
void boundaries() {
    const std::vector<Boundary> rows = {
        {"Count-onset-wait", 0, 100, 100, 100, {}},
        {"Count-high", 0, 580, 768, 720, {480}},
        {"Count-gap", 0, 740, 768, 740, {480}},
        {"Count-inter-event", 0, 800, 800, 800, {480}},
        {"Count-old-due", 0, 960, 960, 960, {480}},
        {"Count-final-drain", 0, 1100, 1248, 1200, {480, 960}},
        {"Count-final-fall", 0, 1200, 1248, 1200, {480, 960}},
        {"Duration-onset-wait", 1, 100, 100, 100, {}},
        {"Duration-high", 1, 580, 768, 720, {480}},
        {"Duration-gap", 1, 740, 768, 740, {480}},
        {"Duration-inter-event", 1, 800, 800, 800, {480}},
        {"Duration-old-due", 1, 960, 960, 960, {480}},
        {"Duration-deadline", 1, 4848, 5088, 5040,
         {480, 960, 1440, 1920, 2400, 2880, 3360, 3840, 4320, 4800}},
        {"Duration-final-drain", 1, 4900, 5088, 5040,
         {480, 960, 1440, 1920, 2400, 2880, 3360, 3840, 4320, 4800}},
        {"Duration-final-fall", 1, 5040, 5088, 5040,
         {480, 960, 1440, 1920, 2400, 2880, 3360, 3840, 4320, 4800}}};
    for (const auto &row : rows)
        for (int policy : {0, 1}) {
            host::Instance h;
            configure(h, row.term);
            h.set(RetriggerParam, policy);
            const auto before = h.run(static_cast<int>(row.request), 4, {0});
            h.set(RateParam, 100);
            const auto after = h.run(17000 - static_cast<int>(row.request), 28, {0});
            host::Trace t = before;
            t.burst.insert(t.burst.end(), after.burst.begin(), after.burst.end());
            t.active.insert(t.active.end(), after.active.begin(), after.active.end());
            for (auto rise : after.rises)
                t.rises.push_back(row.request + rise);
            for (auto fall : after.falls)
                t.falls.push_back(row.request + fall);
            const bool replace = policy || row.request == (row.term ? 5040u : 1200u);
            Samples expected;
            Windows active;
            uint64_t draws;
            if (replace) {
                expected = row.prior;
                expected.push_back(row.activation + 4800);
                if (!row.term)
                    expected.push_back(row.activation + 9600);
                active = {{0, row.oldActiveEnd},
                          {row.activation, row.activation + (row.term ? 5040 : 9840)}};
                const uint64_t oldDraws = row.term ? row.prior.size() + 1
                                                   : (row.prior.empty() ? 1 : 2);
                draws = oldDraws + 2;
            } else {
                expected = row.term ? Samples{480, 960, 1440, 1920, 2400, 2880, 3360, 3840,
                                               4320, 4800} : Samples{480, 960};
                active = {{0, row.term ? 5040 : 1200}};
                draws = row.term ? 11 : 2;
            }
            char name[100];
            std::snprintf(name, sizeof(name), "%s-policy%d", row.name, policy);
            verify(name, t, expected, active, inspectEngine(h.algorithm), replace ? 2 : 1,
                   draws, replace ? 2 : (row.term ? 11 : 2));
        }
}
void livePolicies() {
    for (int term : {0, 1})
        for (bool retain : {false, true}) {
            host::Instance h;
            configure(h, term);
            h.set(RetriggerParam, retain ? 1 : 0);
            host::Trace t = h.run(580, 4, {0});
            h.set(RateParam, 100);
            auto append = [&](host::Trace part, uint64_t base) {
                t.burst.insert(t.burst.end(), part.burst.begin(), part.burst.end());
                t.active.insert(t.active.end(), part.active.begin(), part.active.end());
                for (auto r : part.rises) t.rises.push_back(base + r);
                for (auto f : part.falls) t.falls.push_back(base + f);
            };
            append(h.run(160, 4, {0}), 580);
            if (retain) {
                // Latest accepted request is in the low-wait, not the old high.
                append(h.run(4, 4, {0}), 740);
                h.set(RetriggerParam, 0);
            } else {
                h.set(RetriggerParam, 1);
                append(h.run(4, 4, {}), 740);
            }
            append(h.run(16256, 28, retain ? Samples{4} : Samples{}), 744);
            Samples rises = retain ? (term ? Samples{480, 5568} : Samples{480, 5568, 10368})
                                   : (term ? Samples{480, 960, 1440, 1920, 2400, 2880, 3360,
                                                     3840, 4320, 4800} : Samples{480, 960});
            Windows active = retain ? Windows{{0, 720}, {768, term ? 5808 : 10608}}
                                    : Windows{{0, term ? 5040 : 1200}};
            verify(retain ? "latest-low-wait-On-to-Off" : "ignored-Off-to-On-no-replay", t,
                   rises, active, inspectEngine(h.algorithm), retain ? 3 : 1,
                   retain ? 4 : (term ? 11 : 2), retain ? 2 : (term ? 11 : 2));
        }
}
}
int main() {
    loadContext();
    neutral();
    boundaries();
    livePolicies();
    std::puts("bounded L1/L2 adapter regressions PASS; physical host NOT CLAIMED");
}
