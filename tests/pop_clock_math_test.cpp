#include "pop_clock_math.h"
#include "check.h"
#include <cmath>
#include <cstdio>
#include <initializer_list>
using namespace pop;
int main() {
    for (uint64_t m = 0; m < 128; m += 2) for (int side : {-1, 0, 1}) {
        const double scale = side < 0 ? std::nextafter(0.5, 0.0) : side > 0 ? std::nextafter(0.5, 1.0) : 0.5;
        auto length = ClockLength::fromBase(32016, 100, scale, 100, 14680064);
        uint8_t limits = 0;
        length.clamp(32016, limits);
        const auto w = clockWait(length, 2001, m, (m * 2001 + 15) / 16, m == 0, 161, 33, limits);
        CHECK(w.ticks == m + (side > 0 ? 4u : 2u) && limits == 0);
    }
    uint32_t state = 1;
    for (uint32_t fs : {1u, 19u, 100u, 799u, 800u, 32015u, 32016u, 32017u, 44100u, 48000u, 96000u, 0xffffffffu}) {
        const uint64_t periods[] = {(static_cast<uint64_t>(fs) + 19) / 20, fs, 2ull * fs};
        for (uint64_t period : periods) for (uint32_t denominator : {1u, 2u, 4u, 8u, 100u})
            for (int irreg : {0, 1, 37, 100}) for (int scaleCase = 0; scaleCase < 8; ++scaleCase) {
                state = advance(state);
                const double scales[] = {0.5, std::nextafter(0.5, 0.0), std::nextafter(0.5, 1.0),
                    1.0, std::nextafter(1.0, 0.0), std::nextafter(1.0, 2.0), std::exp2(0.37), 2.0};
                const double scale = scales[scaleCase];
                const uint64_t base = denominator == 100 ? fs : period;
                auto length = ClockLength::fromBase(base, denominator, scale, irreg, state >> 8);
                uint8_t limits = 0;
                length.clamp(fs, limits);
                const auto w = clockWait(length, period, 0, 0, true, 1, 1, limits);
                std::printf("%u %llu %llu %u %a %d %u %llu %llu %u\n", fs,
                    static_cast<unsigned long long>(period), static_cast<unsigned long long>(base), denominator,
                    scale, irreg, state >> 8, static_cast<unsigned long long>(w.ticks),
                    static_cast<unsigned long long>(w.offset), static_cast<unsigned>(limits));
            }
    }
}
