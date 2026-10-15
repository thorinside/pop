#include "check.h"
#include "pop_engine.h"
#include <cmath>
#include <cstdio>
#include <vector>
using namespace pop;
Settings clockSettings() {
    Settings s;
    s.timing = Timing::Clock;
    s.count = 8;
    s.clockRate = 3;
    s.contour = Contour::Flat;
    s.irregularity = 0;
    return s;
}
void lattice() {
    const double equal = 24001.0 / 8;
    for (uint64_t m = 0; m < 128; m += 2) {
        for (int side : {-1,0,1}) {
            uint8_t limits = 0;
            const double l = side < 0 ? std::nextafter(equal, 0.0) : side > 0 ? std::nextafter(equal, INFINITY) : equal;
            const auto wait = clockWait(l, 24001, m, (m*24001+15)/16, m == 0, 240,48,limits);
            const uint64_t d = side > 0 ? 4 : 2;
            CHECK(wait.ticks == m+d && wait.offset == ((m+d)*24001+15)/16 && limits == 0);
        }
    }
    uint8_t limits = 0;
    uint64_t m = 0, previous = 0;
    for (uint64_t expected : {289ull,577ull,865ull,1153ull}) {
        const auto w = clockWait(4609.0/16,4609,m,previous,m==0,240,48,limits);
        CHECK(w.ticks == m+1 && w.offset == expected && limits == 0);
        m = w.ticks; previous=w.offset;
    }
    auto w = clockWait(4593.0/16,4593,0,0,true,240,48,limits);
    CHECK(w.ticks==1 && w.offset==288);
    w=clockWait(4593.0/16,4593,1,288,false,240,48,limits);
    CHECK(w.ticks==3 && w.offset==862 && (limits&PhysicalLimited));
    std::puts("clock,boundaries,nextafter L=P/8 at 64 phases,4609 d1,4593 physical promotion,PASS");
}
void tracker() {
    ClockTracker t;
    CHECK(!t.observe(0,48000) && t.state == ClockTracker::State::First);
    CHECK(!t.observe(24000,48000) && t.state == ClockTracker::State::Locked);
    CHECK(!t.expire(95999) && t.expire(96000));
    CHECK(!t.observe(96000,48000) && t.state == ClockTracker::State::First);
    CHECK(t.observe(96001,48000) && t.state == ClockTracker::State::Empty);
    CHECK(!t.observe(100000,48000) && !t.observe(102400,48000));
    CHECK(t.period == 2400 && t.state == ClockTracker::State::Locked);
    CHECK(t.observe(104799,48000) && t.state == ClockTracker::State::Empty);
    CHECK(!t.observe(110000,48000) && !t.observe(206000,48000));
    CHECK(t.period==96000);
    CHECK(t.observe(302001,48000));
    std::puts("clock,tracker,min/max/invalid/expiry priority,PASS");
}
void lifecycle() {
    for (const auto term : {Termination::Count,Termination::Duration}) {
        auto s=clockSettings(); s.count=64; s.termination=term; s.durationBeatsIndex=7;
        Engine e;
        std::vector<uint64_t> rises;
        bool previous=false;
        for(uint64_t t=0;t<200000;++t) {
            const bool edge=t==0 || t==24000 || t==96000 || t==120000 || t==144000 || t==168000;
            const auto o=e.tick(s,false,{t==100,false,false,edge});
            if(o.burst&&!previous)rises.push_back(t);
            if(t>=96000)CHECK(!o.active && !o.burst);
            previous=o.burst;
        }
        const std::vector<uint64_t> expected={27000,30000,33000,36000,39000,42000,45000,48000,51000,54000,57000,60000,63000,66000,69000,72000,75000,78000,81000,84000,87000,90000,93000};
        CHECK(rises==expected && e.error()==Error::ClockStopped);
        CHECK(e.acceptedRequests()==1);
        CHECK(e.phase()==Phase::Idle);
        Engine timeout;
        for(uint64_t t=0;t<=288000;++t) {
            const auto o=timeout.tick(s,false,{t==0,false,false,t==0||t==288000});
            CHECK(!o.burst&&!o.active);
        }
        CHECK(timeout.phase()==Phase::Idle && timeout.error()==Error::ClockTimeout);
        Engine none;
        CHECK(!none.tick(s,true,{true,false,false,false,false}).active);
        CHECK(none.acceptedRequests()==0 && none.error()==Error::NoClock);
        Engine coincident;
        for(uint64_t t=0;t<=51000;++t) {
            const auto o=coincident.tick(s,true,{t==24000,false,false,t%24000==0});
            CHECK(o.active==(t>=48000));
            CHECK(o.burst==(t==51000));
        }
        Engine snapshot;
        for(uint64_t t=0;t<=36000;++t) {
            const auto o=snapshot.tick(s,false,{t==100,false,false,t==0||t==24000||t==30000||t==36000});
            if(t==27000||t==33000)CHECK(o.burst);
            if(t==32000)CHECK(!o.burst);
        }
        s.clockRate=0;
        Engine drain;
        for(uint64_t t=0;t<=120000;++t) {
            const bool edge=t==100||t==24050||t==48000||t==71950||t==95900||t==96000;
            const auto o=drain.tick(s,false,{t==200||t==96001,false,false,edge});
            if(t>=95900) {
                CHECK(o.burst==(t<96140));
                CHECK(o.active==(t<96000));
            }
        }
        CHECK(drain.acceptedRequests()==2 && drain.phase()==Phase::Pending);
        // Long requested gap aborts on invalid period before its first output.
        s.clockRate=-3;
        Engine invalid;
        for(uint64_t t=0;t<=26401;++t) {
            const auto o=invalid.tick(s,false,{t==100,false,false,t==0||t==24000||t==26399});
            CHECK(!o.burst);
            CHECK(o.active==(t>=24000&&t<26399));
        }
        CHECK(invalid.error()==Error::ClockStopped);
    }
    std::puts("clock,lifecycle,both terminations,none/timeout/strict-later/snapshot/invalid before onset/no replay,PASS");
}
int main(){ lattice(); tracker(); lifecycle(); }
