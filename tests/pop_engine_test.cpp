#include "check.h"
#include "fixtures/internal.h"
#include "pop_engine.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
using namespace pop;

Settings literal(const fixture::Case &f) {
    Settings s;
    s.count = f.n;
    s.rateTenths = f.rate * 10;
    s.contour = static_cast<Contour>(f.contour);
    s.amount = f.amount;
    s.irregularity = f.irregularity;
    if (f.duration) {
        s.termination = Termination::Duration;
        s.durationMs = f.duration * 1000 / 48000;
    }
    return s;
}
void golden() {
    for (const auto &f : fixture::cases) {
        // Two test-only T +/-1 vectors are sub-millisecond boundary seams, tested below.
        if (f.duration == 4801 || f.duration == 4799)
            continue;
        Engine e(48000, f.seed);
        const Settings s = literal(f);
        std::vector<uint64_t> rises, falls;
        bool last = false;
        uint64_t activeEnd = 0;
        for (uint64_t t = 0; t < f.end + 1000; ++t) {
            const Output o = e.tick(s, true, {t == 0, false, false});
            if (o.burst && !last)
                rises.push_back(t);
            if (!o.burst && last)
                falls.push_back(t);
            if (o.active)
                activeEnd = t + 1;
            last = o.burst;
        }
        CHECK(rises == std::vector<uint64_t>(f.rises, f.rises + f.size));
        CHECK(falls.size() == rises.size());
        for (size_t i = 0; i < falls.size(); ++i)
            CHECK(falls[i] == rises[i] + 240);
        CHECK(activeEnd == f.end);
        CHECK(e.requestState() == f.request && e.generationState() == f.generation);
        CHECK(e.generationDraws() == static_cast<uint64_t>(f.draws));
        std::printf("golden,%s,edges=%zu,activeEnd=%llu,request=%u,generation=%u,draws=%llu\n",
                    f.name, rises.size(), static_cast<unsigned long long>(activeEnd),
                    e.requestState(), e.generationState(),
                    static_cast<unsigned long long>(e.generationDraws()));
    }
}
void restartAndCancel() {
    for (const auto termination : {Termination::Count, Termination::Duration}) {
        for (const bool retrigger : {false, true}) {
            for (const uint64_t offset : {0ull, 100ull, 239ull, 240ull, 250ull, 287ull, 288ull,
                                          4000ull, 4800ull, 9600ull, 9840ull}) {
                Settings s;
                s.count = 2;
                s.rateTenths = 100;
                s.irregularity = 0;
                s.contour = Contour::Flat;
                s.termination = termination;
                s.durationMs = 200;
                Engine e;
                bool previous = false;
                std::vector<uint64_t> rises;
                for (uint64_t t = 0; t < 26000; ++t) {
                    const auto o =
                        e.tick(s, retrigger, {t == 0 || t == 4800 + offset, false, false});
                    if (o.burst && !previous)
                        rises.push_back(t);
                    previous = o.burst;
                }
                CHECK(!rises.empty() &&
                      rises.front() == ((retrigger && offset == 0) ? 9600u : 4800u));
                for (size_t i = 1; i < rises.size(); ++i)
                    CHECK(rises[i] - rises[i - 1] >= 288);
                if (!retrigger && offset < 4800) {
                    CHECK(e.acceptedRequests() == 1);
                    CHECK(e.requestState() == 270369);
                }
                if (retrigger && offset == 100)
                    CHECK(rises[1] == 9888);
            }
        }
    }
    Settings s;
    s.count = 64;
    s.irregularity = 0;
    s.contour = Contour::Flat;
    s.rateTenths = 1000;
    for (const uint64_t cancelAt : {481ull, 719ull, 720ull, 800ull}) {
        Engine e;
        int rises = 0;
        bool prior = false;
        for (uint64_t t = 0; t < 2000; ++t) {
            const auto o = e.tick(s, true, {t == 0, t == cancelAt, false});
            if (o.burst && !prior)
                ++rises;
            if (t >= cancelAt)
                CHECK(o.burst == (t < 720));
            prior = o.burst;
        }
        CHECK(rises == 1 && e.acceptedRequests() == 1);
    }
    // Translate the contract's old rise0 to rise480, preserving all relative boundaries.
    Engine e;
    s.count = 2;
    for (uint64_t t = 0; t < 5600; ++t) {
        if (t == 580)
            s.rateTenths = 100;
        const auto o = e.tick(s, true, {t == 0 || t == 580 || t == 680, false, false});
        if (t >= 480 && t < 720)
            CHECK(o.active && o.burst);
        if (t >= 720 && t < 768)
            CHECK(!o.active && !o.burst);
        if (t == 768)
            CHECK(o.active && !o.burst);
        if (t == 5568)
            CHECK(o.burst);
    }
    CHECK(e.acceptedRequests() == 3);
}
void boundaries() {
    uint8_t limits = 0;
    CHECK(internalGap(287, false, 240, 48, limits) == 288 && (limits & PhysicalLimited));
    limits = 0;
    CHECK(internalGap(288, false, 240, 48, limits) == 288 && limits == 0);
    CHECK(internalGap(289, false, 240, 48, limits) == 289 && limits == 0);
    CHECK(internalGap(1, true, 240, 48, limits) == 1);
    CHECK(internalGap(1.001, true, 240, 48, limits) == 2);
    for (const uint32_t fs : {32000u, 44100u, 48000u, 96000u}) {
        for (const auto term : {Termination::Count, Termination::Duration}) {
            Settings s;
            s.count = 64;
            s.rateTenths = 1000;
            s.amount = 100;
            s.irregularity = 100;
            s.contour = Contour::Acceleration;
            s.termination = term;
            s.durationMs = 100;
            Engine e(fs, 17);
            uint64_t rise = 0, lastFall = 0;
            bool prev = false, seen = false;
            int count = 0;
            for (uint64_t t = 0; t < fs * 2; ++t) {
                const auto o = e.tick(s, true, {t == 0, false, false});
                if (o.burst && !prev) {
                    if (seen)
                        CHECK(t - lastFall >= e.gap());
                    rise = t;
                    seen = true;
                    ++count;
                }
                if (!o.burst && prev) {
                    CHECK(t - rise == e.width());
                    lastFall = t;
                }
                prev = o.burst;
            }
            CHECK(count > 0 && (term == Termination::Duration || count == 64));
        }
    }
    Settings s;
    s.irregularity = 0;
    s.contour = Contour::Flat;
    for (const int n : {0, 65}) {
        Engine e;
        s.count = n;
        CHECK(!e.tick(s, true, {true}).active);
        CHECK(e.acceptedRequests() == 0 && e.error() == Error::InvalidSettings);
    }
    s.count = 1;
    for (const int d : {99, 100, 30000, 30001}) {
        Engine e;
        s.termination = Termination::Duration;
        s.durationMs = d;
        const auto o = e.tick(s, true, {true});
        CHECK(o.active == (d == 100 || d == 30000));
    }
    Schmitt detector;
    for (float v : {5.0f, 1.5f, std::numeric_limits<float>::quiet_NaN(), 2.0f})
        CHECK(!detector.observe(v));
    CHECK(!detector.observe(1));
    CHECK(!detector.observe(1.9f));
    CHECK(detector.observe(2));
    CHECK(!detector.observe(std::numeric_limits<float>::infinity()));
    CHECK(!detector.observe(5));
    Engine e;
    e.tick(Settings{}, true, {true});
    const auto rng = e.requestState();
    e.tick(Settings{}, true, {false, false, true});
    CHECK(e.requestState() == rng && e.phase() == Phase::Idle);
    CHECK(e.tick(Settings{}, true, {true}).active && e.requestState() == 67634689);
    for (const double u : {0.0, 0.25, 0.5, 0.75}) {
        const double expected = u == 0 ? 2.0 : (u == 0.5 ? 0.5 : 1.0);
        CHECK(std::exp2(contourHeight(Contour::RiseFall, u)) == expected);
        CHECK(std::exp2(contourHeight(Contour::FallRise, u)) == 1.0 / expected);
    }
}
void statistics() {
    long double decEarly = 0, decLate = 0, accEarly = 0, accLate = 0;
    uint64_t riseMiddle = 0, riseOuter = 0, fallMiddle = 0, fallOuter = 0;
    int minCount = 10000, maxCount = 0;
    std::FILE *file = std::fopen("build/seed-distributions.csv", "w");
    CHECK(file);
    std::fprintf(file, "seed,contour,termination,count,earlySum,lateSum,outer,middle\n");
    for (uint32_t seed = 1; seed <= 1024; ++seed) {
        for (const auto c : {Contour::Flat, Contour::Acceleration, Contour::Deceleration,
                             Contour::RiseFall, Contour::FallRise}) {
            Settings s;
            s.count = 32;
            s.rateTenths = 100;
            s.irregularity = 50;
            s.amount = 100;
            s.contour = c;
            Engine e(48000, seed);
            uint64_t previous = 0, early = 0, late = 0;
            bool high = false;
            int n = 0;
            for (uint64_t t = 0; t < 48000 * 8; ++t) {
                const auto o = e.tick(s, true, {t == 0});
                if (o.burst && !high) {
                    if (n >= 1 && n <= 7)
                        early += t - previous;
                    if (n >= 25)
                        late += t - previous;
                    previous = t;
                    ++n;
                }
                high = o.burst;
                if (t > 0 && e.phase() == Phase::Idle)
                    break;
            }
            CHECK(n == 32);
            if (c == Contour::Deceleration) {
                decEarly += static_cast<long double>(early);
                decLate += static_cast<long double>(late);
            }
            if (c == Contour::Acceleration) {
                accEarly += static_cast<long double>(early);
                accLate += static_cast<long double>(late);
            }
            std::fprintf(file, "%u,%d,Count,%d,%llu,%llu,0,0\n", seed, static_cast<int>(c), n,
                         static_cast<unsigned long long>(early),
                         static_cast<unsigned long long>(late));
            s.termination = Termination::Duration;
            s.durationMs = 2000;
            s.rateTenths = 200;
            Engine d(48000, seed);
            n = 0;
            int outer = 0, middle = 0;
            high = false;
            for (uint64_t t = 0; t < 96240; ++t) {
                const auto o = d.tick(s, true, {t == 0});
                if (o.burst && !high) {
                    ++n;
                    if (t < 24000 || t >= 72000)
                        ++outer;
                    else
                        ++middle;
                }
                high = o.burst;
            }
            if (c == Contour::RiseFall) {
                riseMiddle += static_cast<uint64_t>(middle);
                riseOuter += static_cast<uint64_t>(outer);
            }
            if (c == Contour::FallRise) {
                fallMiddle += static_cast<uint64_t>(middle);
                fallOuter += static_cast<uint64_t>(outer);
            }
            minCount = std::min(minCount, n);
            maxCount = std::max(maxCount, n);
            std::fprintf(file, "%u,%d,Duration,%d,0,0,%d,%d\n", seed, static_cast<int>(c), n, outer,
                         middle);
        }
    }
    std::fclose(file);
    CHECK(decLate / decEarly > 2 && accLate / accEarly < 0.5);
    CHECK(riseMiddle > riseOuter && fallOuter > fallMiddle && minCount < maxCount);
    std::printf("statistics,decRatio=%.9Lf,accRatio=%.9Lf,riseMiddle=%llu,riseOuter=%llu,"
                "fallMiddle=%llu,fallOuter=%llu,minCount=%d,maxCount=%d\n",
                decLate / decEarly, accLate / accEarly, static_cast<unsigned long long>(riseMiddle),
                static_cast<unsigned long long>(riseOuter),
                static_cast<unsigned long long>(fallMiddle),
                static_cast<unsigned long long>(fallOuter), minCount, maxCount);
}
int main(int argc, char **) {
    golden();
    restartAndCancel();
    boundaries();
    if (argc == 1)
        statistics();
    std::puts("engine PASS");
}
