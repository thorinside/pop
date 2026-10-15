#include "check.h"
#include "pop_sample_math.h"
#include "pop_engine.h"
#include <cmath>
#include <initializer_list>
#include <limits>
using namespace pop;

int main() {
    static_assert(std::numeric_limits<double>::digits >= 53);
    for (const uint64_t integer : {1ull, 287ull, 288ull, 289ull, 384000ull,
                                  4294967295ull, 4294967296ull, 4294967297ull,
                                  17179869180ull}) {
        const double value = static_cast<double>(integer);
        CHECK(sampleCountAsDouble(integer) == value);
        CHECK(ceilIntervalSamples(value) == integer);
        if (integer > 1)
            CHECK(ceilIntervalSamples(std::nextafter(value, 0.0)) == integer);
        if (integer < 17179869180ull)
            CHECK(ceilIntervalSamples(std::nextafter(value, value * 2)) == integer + 1);
    }
    for (const uint32_t fs : {1u, 999u, 1000u, 32000u, 44100u, 48000u, 96000u,
                              4294967295u}) {
        for (uint32_t ms = 1; ms <= 30000; ++ms) {
            const uint64_t expected = (static_cast<uint64_t>(fs) * ms + 999) / 1000;
            CHECK(millisecondsSamples(fs, ms) == expected);
            CHECK(sampleCountAsDouble(expected) == static_cast<double>(expected));
        }
        Engine e(fs);
        CHECK(e.width() == (static_cast<uint64_t>(fs) * 5 + 999) / 1000);
        CHECK(e.gap() == (static_cast<uint64_t>(fs) + 999) / 1000);
    }
    CHECK(millisecondsSamples(44100, 101) == 4455);
    CHECK(millisecondsSamples(4294967295u, 30000) == 128849018850ull);
    CHECK(sampleCountAsDouble(128849018850ull) == 128849018850.0);
    std::puts("sample math PASS: positive ceil neighbours, full uint32 Fs, all ms1..30000, binary64 exactness");
}
