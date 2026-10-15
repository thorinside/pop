#pragma once
#include "pop_engine.h"
#include <cstdint>
#include <cstring>

namespace pop {
// Five limbs cover the < 2^139 cross-products over nonzero uint32 Fs.
struct ClockProduct {
    uint32_t words[5]{};
    explicit ClockProduct(uint64_t value = 0) {
        words[0] = static_cast<uint32_t>(value);
        words[1] = static_cast<uint32_t>(value >> 32);
    }
    ClockProduct times(uint64_t factor) const {
        ClockProduct result;
        for (unsigned j = 0; j < 2; ++j) {
            const uint32_t part = j == 0 ? static_cast<uint32_t>(factor)
                                         : static_cast<uint32_t>(factor >> 32);
            uint64_t carry = 0;
            for (unsigned i = 0; i + j < 5; ++i) {
                const uint64_t product = static_cast<uint64_t>(words[i]) * part + result.words[i + j] + carry;
                result.words[i + j] = static_cast<uint32_t>(product);
                carry = product >> 32;
            }
        }
        return result;
    }
    ClockProduct shifted(unsigned bits) const {
        ClockProduct result;
        const unsigned whole = bits / 32, remainder = bits % 32;
        for (unsigned i = whole; i < 5; ++i) {
            result.words[i] = words[i - whole] << remainder;
            if (remainder && i > whole)
                result.words[i] |= words[i - whole - 1] >> (32 - remainder);
        }
        return result;
    }
    bool less(const ClockProduct &other) const {
        for (unsigned i = 5; i > 0; --i)
            if (words[i - 1] != other.words[i - 1])
                return words[i - 1] < other.words[i - 1];
        return false;
    }
};

struct ClockLength {
    ClockProduct numerator, denominator;
    static ClockLength fromBase(uint64_t baseNumerator, uint32_t baseDenominator,
                                double scale, int irregularity, uint32_t random24) {
        const int64_t jitterNumerator = 400ll * (1 << 24) +
            3ll * irregularity * (2ll * random24 - (1 << 24));
        uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(scale), "binary64 Clock scale required");
        std::memcpy(&bits, &scale, sizeof(bits));
        const uint64_t mantissa = (bits & ((uint64_t{1} << 52) - 1)) | (uint64_t{1} << 52);
        const unsigned denominatorShift = 1075u - static_cast<unsigned>((bits >> 52) & 2047u);
        return {ClockProduct(baseNumerator).times(static_cast<uint64_t>(jitterNumerator)).times(mantissa),
                ClockProduct(static_cast<uint64_t>(baseDenominator) * 400 * (1 << 24)).shifted(denominatorShift)};
    }
    void clamp(uint32_t fs, uint8_t &limits) {
        if (numerator.less(denominator)) {
            numerator = denominator;
            limits |= DurationLimited;
        }
        const auto maximum = denominator.times(4ull * fs);
        if (maximum.less(numerator)) {
            numerator = maximum;
            limits |= DurationLimited;
        }
    }
    bool exceeds(uint64_t ticksTimesPeriod) const {
        return denominator.times(ticksTimesPeriod).less(numerator.shifted(4));
    }
};
ClockWait clockWait(const ClockLength &length, uint64_t period, uint64_t ticks, uint64_t previousOffset,
                    bool onset, uint32_t width, uint32_t gap, uint8_t &limits);
} // namespace pop
