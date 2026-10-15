#include "check.h"
#include "nt_host_driver.h"
#include "fixtures/clock/vectors.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
using namespace pop;

host::Trace runClock(host::Instance &h, int samples, int partition,
                     const std::vector<uint64_t> &starts, const std::vector<uint64_t> &clocks) {
    host::Trace trace;
    static unsigned receipt = 0;
    std::FILE *raw = nullptr;
    if(std::getenv("POP_CLOCK_TRACE")) {
        const auto path="build/clock-trace-"+std::to_string(receipt++)+".csv";
        raw=std::fopen(path.c_str(),"w"); CHECK(raw);
        std::fprintf(raw,"sample,triggerEdge,clockEdge,burstV,activeV\n");
    }
    bool previous = false;
    for (int base = 0; base < samples; base += partition) {
        const int frames = std::min(partition, samples - base);
        std::fill_n(h.buffers.data(), 64 * frames, 0.0f);
        for (int i = 0; i < frames; ++i) {
            const auto t = static_cast<uint64_t>(base + i);
            for (const auto &input : {std::make_pair(TriggerInputParam, &starts),
                                      std::make_pair(ClockInputParam, &clocks)}) {
                const int bus = h.values[static_cast<size_t>(input.first)];
                if (bus > 0 && std::find(input.second->begin(), input.second->end(), t) != input.second->end())
                    h.buffers[static_cast<size_t>((bus - 1) * frames + i)] = 5.0f;
            }
        }
        h.step(frames / 4);
        for (int i = 0; i < frames; ++i) {
            const auto t = static_cast<uint64_t>(base + i);
            const float b = h.buffers[static_cast<size_t>(12 * frames + i)];
            const float a = h.buffers[static_cast<size_t>(13 * frames + i)];
            CHECK((b == 0 || b == 5) && (a == 0 || a == 5));
            if (b == 5 && !previous) trace.rises.push_back(t);
            if (b == 0 && previous) trace.falls.push_back(t);
            trace.burst.push_back(b);
            trace.active.push_back(a);
            previous = b == 5;
            if(raw) std::fprintf(raw,"%llu,%d,%d,%.0f,%.0f\n",static_cast<unsigned long long>(t),
                std::find(starts.begin(),starts.end(),t)!=starts.end(),
                std::find(clocks.begin(),clocks.end(),t)!=clocks.end(),b,a);
        }
    }
    if(raw) std::fclose(raw);
    return trace;
}
void configure(host::Instance &h, int count = 8, int ratio = 3) {
    h.set(TimingParam, 1);
    h.set(ClockInputParam, 2);
    h.set(CountParam, count);
    h.set(RateParam, ratio);
    h.set(IrregularityParam, 0);
    h.set(ContourParam, 0);
    h.process(4);
    injectSeed(h.algorithm, 1);
}
void expect(const host::Trace &t, const std::vector<uint64_t> &rises, uint64_t activation, uint64_t end) {
    CHECK(t.rises == rises);
    std::vector<uint64_t> falls;
    for (auto r : rises) falls.push_back(r + 240);
    CHECK(t.falls == falls);
    for (size_t i = 0; i < t.active.size(); ++i)
        CHECK(t.active[i] == (i >= activation && i < end ? 5.0f : 0.0f));
    std::printf("clock,literal,A=%llu,end=%llu,rises", static_cast<unsigned long long>(activation), static_cast<unsigned long long>(end));
    for (auto r : rises) std::printf(",%llu", static_cast<unsigned long long>(r));
    std::puts(",PASS");
}
int main() {
    {
        host::Instance fresh;
        CHECK(fresh.factory->guid == 0x70506854u);
        CHECK(fresh.values[TimingParam] == 0 && fresh.values[TerminationParam] == 0 &&
              fresh.values[CountParam] == 4 && fresh.values[IrregularityParam] == 0 &&
              fresh.values[ContourParam] == 0 && fresh.values[AmountParam] == 100 &&
              fresh.values[RateParam] == 200);
        expect(fresh.run(11000, 28, {0}), {2400,4800,7200,9600}, 0,9840);
        fresh.set(CountParam,13); fresh.set(IrregularityParam,67);
        fresh.set(ContourParam,4); fresh.set(AmountParam,12);
        const auto saved=fresh.save();
        CHECK(fresh.load(saved)); fresh.process(4);
        CHECK(fresh.values[CountParam]==13 && fresh.values[IrregularityParam]==67 &&
              fresh.values[ContourParam]==4 && fresh.values[AmountParam]==12);
        for(int p : {TerminationParam,ContourParam}) {
            host::Instance malformed; malformed.set(p,256);
            CHECK(malformed.run(11000,128,{0}).rises.empty());
            CHECK(inspectEngine(malformed.algorithm).error()==Error::InvalidSettings);
        }
        std::puts("factory,ThPp,owner-defaults,four pulses,saved settings,raw enums,PASS");
    }
    const std::vector<std::vector<uint64_t>> expected = {
        {3000,6000,9000,12000,15000,18000,21000,23999},
        {3000,6000,9000,12000,15000,18000,21000,24000},
        {3001,6001,9001,12001,15001,18001,21001,24001},
        {3001,6001,9001,12001,15002,18002,21002,24002}};
    for (int p = 23999; p <= 24002; ++p) for (int partition : {4,28,64,128}) {
        host::Instance h;
        configure(h);
        const uint64_t period = static_cast<uint64_t>(p), a = 100 + period;
        auto rises = expected[static_cast<size_t>(p - 23999)];
        for (auto &r : rises) r += a;
        const auto t = runClock(h, 76000, partition, {200}, {100,a,a+period,a+2*period});
        expect(t, rises, a, rises.back()+240);
        CHECK(inspectEngine(h.algorithm).acceptedRequests() == 1);
        CHECK(inspectEngine(h.algorithm).generationDraws() == 8);
    }
    for (int ratio : {1,2}) {
        host::Instance h;
        configure(h, ratio == 1 ? 2 : 4, ratio);
        const std::vector<uint64_t> r = ratio == 1 ? std::vector<uint64_t>{36102,48102} : std::vector<uint64_t>{30102,36102,42102,48102};
        expect(runClock(h, 76000, 128, {200}, {100,24101,48102,72103}), r, 24101, 48342);
    }
    for (int duration : {0,1,2}) {
        host::Instance h;
        configure(h);
        h.set(TerminationParam, 1);
        h.set(DurationParam, duration);
        const std::vector<uint64_t> r = duration == 0 ? std::vector<uint64_t>{27102} : duration == 1 ? std::vector<uint64_t>{27102,30102,33102} : std::vector<uint64_t>{27102,30102,33102,36102,39102,42102,45102};
        const uint64_t end = duration == 0 ? 30102 : duration == 1 ? 36102 : 48102;
        expect(runClock(h, 76000, 28, {200}, {100,24101,48102,72103}), r, 24101, end);
    }
    for (int termination : {0,1}) for (int restart : {0,1}) {
        host::Instance h;
        configure(h,4);
        h.set(TerminationParam, termination);
        h.set(RetriggerParam, restart);
        h.set(ClockInputParam, 1);
        h.process(4);
        injectSeed(h.algorithm,1);
        const auto t = runClock(h, 120000, 128, {100,24100,48100,72100,96100}, {100,24100,48100,72100,96100});
        if (restart) CHECK(t.rises.empty());
        else CHECK(!t.rises.empty() && t.rises.front() == 27100);
        std::printf("clock,shared,termination=%d,restart=%d,rises=%zu,PASS\n",termination,restart,t.rises.size());
    }
    for(int term : {0,1}) {
        host::Instance high; configure(high,64,0);
        high.set(TerminationParam,term); high.set(DurationParam,7);
        expect(runClock(high,180000,128,{200},{100,24050,48000,71950,95900,96000,120000,144000,168000}),
               {48000,71950,95900},24050,96000);
        CHECK(inspectEngine(high.algorithm).phase()==Phase::Idle && inspectEngine(high.algorithm).error()==Error::ClockStopped);
        host::Instance lost; configure(lost,64,3);
        lost.set(TerminationParam,term); lost.set(DurationParam,7);
        expect(runClock(lost,180000,28,{200},{100,24100,96100,120100,144100,168100}),
               {27100,30100,33100,36100,39100,42100,45100,48100,51100,54100,57100,60100,63100,66100,69100,72100,75100,78100,81100,84100,87100,90100,93100},24100,96100);
        CHECK(inspectEngine(lost.algorithm).acceptedRequests()==1);
        std::puts("live,09-clock-loss,both terminations,due-suppression/high-drain/Active-low/recovery-no-replay,PASS");
    }
    for (const auto &f : clock_fixture::cases) for(int partition : {4,28,128}) {
        host::Instance h; configure(h,5,1);
        h.set(ContourParam,f.contour); h.set(AmountParam,100); h.set(IrregularityParam,50);
        h.set(TerminationParam,f.duration); h.set(DurationParam,3);
        injectSeed(h.algorithm,f.seed);
        auto r=std::vector<uint64_t>(f.rises,f.rises+f.size);
        for(auto &v:r) v+=24101;
        expect(runClock(h,220000,partition,{200},{100,24101,48102,72103,96104,120105,144106,168107,192108,216109}),r,24101,24101+f.end);
        const auto &e=inspectEngine(h.algorithm);
        CHECK(e.requestState()==f.request && e.generationState()==f.generation && e.generationDraws()==f.draws);
        std::printf("clock,Decimal80,seed=%u,contour=%d,Duration=%d,partition=%d,draws=%u,PASS\n",f.seed,f.contour,f.duration,partition,f.draws);
    }
    std::puts("Clock adapter PASS");
}
