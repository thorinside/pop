#pragma once
#include <cmath>
#include <cstdint>

namespace pop {
// Split words avoid compiler runtime helpers that NT does not export. Binary64
// represents every integer in our <= 30 * UINT32_MAX duration domain exactly.
inline double sampleCountAsDouble(uint64_t samples) {
    return static_cast<double>(static_cast<uint32_t>(samples >> 32)) * 4294967296.0 +
           static_cast<double>(static_cast<uint32_t>(samples));
}
// Caller supplies a finite positive interval <= 4 * UINT32_MAX samples.
inline uint64_t ceilIntervalSamples(double length) {
    const double rounded = std::ceil(length);
    const auto upper = static_cast<uint32_t>(rounded / 4294967296.0);
    const auto lower = static_cast<uint32_t>(rounded - static_cast<double>(upper) * 4294967296.0);
    return (static_cast<uint64_t>(upper) << 32) | lower;
}
// milliseconds <= 30000. The remainder product is <= 29970000 and cannot
// overflow a word; quotient multiplication remains wide without wide division.
inline uint64_t millisecondsSamples(uint32_t fs, uint32_t milliseconds) {
    return static_cast<uint64_t>(fs / 1000) * milliseconds +
           ((fs % 1000) * milliseconds + 999) / 1000;
}
} // namespace pop
