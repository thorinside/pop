#include "check.h"
#include "nt_host_driver.h"
#include <algorithm>
#include <cstdio>
using namespace pop;
int main() {
    for (uint32_t fs : {32000u, 44100u, 48000u, 96000u}) {
        host::sampleRate(fs);
        const uint64_t width = (5ull * fs + 999) / 1000, gap = (fs + 999) / 1000;
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
        h.factory->step(h.algorithm, h.buffers.data(), by4);
        for (float v : h.buffers)
            CHECK(v == 7);
    }
    host::sampleRate(48000);
    std::puts("adapter observed-Fs-reset and frame-boundary validation PASS; physical rate support "
              "NOT CLAIMED");
}
