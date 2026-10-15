#include "pop_engine.h"
#include <cmath>

namespace pop {
uint32_t advance(uint32_t x) {
    x ^= x << 13;
    x ^= x >> 17;
    return x ^ (x << 5);
}
bool Schmitt::observe(float voltage) {
    if (!std::isfinite(voltage))
        return false;
    if (voltage <= 1.0f)
        armed = true;
    if (voltage >= 2.0f && armed) {
        armed = false;
        return true;
    }
    return false;
}
double contourHeight(Contour contour, double u) {
    switch (contour) {
    case Contour::Flat:
        return 0.0;
    case Contour::Acceleration:
        return 1.0 - 2.0 * u;
    case Contour::Deceleration:
        return 2.0 * u - 1.0;
    case Contour::RiseFall:
        return 2.0 * std::abs(2.0 * u - 1.0) - 1.0;
    case Contour::FallRise:
        return 1.0 - 2.0 * std::abs(2.0 * u - 1.0);
    }
    return 0.0;
}
uint64_t internalGap(double length, bool onset, uint32_t width, uint32_t gap, uint8_t &limits) {
    const auto rounded = static_cast<uint64_t>(std::ceil(length));
    const uint64_t minimum = onset ? 1 : static_cast<uint64_t>(width) + gap;
    if (rounded < minimum) {
        limits |= PhysicalLimited;
        return minimum;
    }
    return rounded;
}
bool valid(const Settings &s) {
    return (s.termination == Termination::Count || s.termination == Termination::Duration) &&
           (s.timing == Timing::Internal || s.timing == Timing::Clock) &&
           (s.termination != Termination::Count || (s.count >= 1 && s.count <= 64)) &&
           (s.termination != Termination::Duration ||
            (s.durationMs >= 100 && s.durationMs <= 30000)) &&
           s.rateTenths >= 10 && s.rateTenths <= 1000 && s.irregularity >= 0 &&
           s.irregularity <= 100 && s.amount >= 0 && s.amount <= 100 &&
           static_cast<unsigned>(s.contour) <= static_cast<unsigned>(Contour::FallRise);
}
Engine::Engine(uint32_t sampleRate, uint32_t seed)
    : fs_(sampleRate), width_(0), gap_(0), requestRng_(seed ? seed : 0x6d2b79f5u) {
    safetyReset(sampleRate);
}
void Engine::safetyReset(uint32_t sampleRate) {
    fs_ = sampleRate ? sampleRate : 48000;
    width_ = static_cast<uint32_t>((static_cast<uint64_t>(fs_) * 5 + 999) / 1000);
    gap_ = static_cast<uint32_t>((static_cast<uint64_t>(fs_) + 999) / 1000);
    phase_ = Phase::Idle;
    fall_ = ready_ = now_;
    dueValid_ = false;
    emitted_ = 0;
    generationRng_ = 0;
    limits_ = 0;
    error_ = Error::None;
}
#ifdef POP_TESTING
void Engine::injectSeed(uint32_t seed) {
    safetyReset(fs_);
    requestRng_ = seed ? seed : 0x6d2b79f5u;
    accepted_ = draws_ = 0;
}
#endif
void Engine::accept(const Settings &settings) {
    snapshot_ = settings;
    requestRng_ = advance(requestRng_);
    generationRng_ = requestRng_;
    ++accepted_;
    emitted_ = 0;
    limits_ = 0;
    error_ = Error::None;
    dueValid_ = false;
    phase_ = Phase::Pending;
}
void Engine::activate() {
    activation_ = now_;
    deadline_ = now_ + (static_cast<uint64_t>(snapshot_.durationMs) * fs_ + 999) / 1000;
    phase_ = Phase::Generating;
    schedule(true);
}
void Engine::schedule(bool onset) {
    double u = 0.5;
    if (snapshot_.termination == Termination::Duration) {
        u = static_cast<double>(now_ - activation_) / static_cast<double>(deadline_ - activation_);
    } else if (snapshot_.count > 2) {
        u = static_cast<double>(emitted_) / static_cast<double>(snapshot_.count - 1);
    }
    double base = 10.0 / static_cast<double>(snapshot_.rateTenths);
    if (base < 0.010) {
        base = 0.010;
        limits_ |= BaseLimited;
    }
    if (base > 1.000) {
        base = 1.000;
        limits_ |= BaseLimited;
    }
    generationRng_ = advance(generationRng_);
    ++draws_;
    const double r = static_cast<double>(generationRng_ >> 8) / 16777216.0;
    const double scale =
        std::exp2((snapshot_.amount / 100.0) * contourHeight(snapshot_.contour, u));
    const double jitter = 1.0 + 0.75 * (snapshot_.irregularity / 100.0) * (2.0 * r - 1.0);
    double length = static_cast<double>(fs_) * base * scale * jitter;
    if (length < 1.0) {
        length = 1.0;
        limits_ |= DurationLimited;
    }
    if (length > 4.0 * fs_) {
        length = 4.0 * fs_;
        limits_ |= DurationLimited;
    }
    due_ = now_ + internalGap(length, onset, width_, gap_, limits_);
    dueValid_ = snapshot_.termination == Termination::Count || due_ < deadline_;
}
Output Engine::tick(const Settings &current, bool retrigger, Events events) {
    if (events.reset) {
        safetyReset(fs_);
    } else if (events.cancel) {
        dueValid_ = false;
        phase_ = high() ? Phase::Draining : Phase::Idle;
    } else {
        if (phase_ == Phase::Generating && snapshot_.termination == Termination::Duration &&
            now_ >= deadline_) {
            dueValid_ = false;
            phase_ = high() ? Phase::Draining : Phase::Idle;
        }
        if (phase_ == Phase::Draining && !high())
            phase_ = Phase::Idle;
        if (events.start && (phase_ == Phase::Idle || retrigger)) {
            if (!valid(current))
                error_ = Error::InvalidSettings;
            else if (current.timing == Timing::Clock)
                error_ = Error::ClockIncomplete;
            else
                accept(current);
        }
        if (phase_ == Phase::Pending && now_ >= ready_)
            activate();
        if (phase_ == Phase::Generating && dueValid_ && now_ == due_) {
            fall_ = now_ + width_;
            ready_ = fall_ + gap_;
            ++emitted_;
            if (snapshot_.termination == Termination::Count &&
                emitted_ == static_cast<uint32_t>(snapshot_.count)) {
                dueValid_ = false;
                phase_ = Phase::Draining;
            } else
                schedule(false);
        }
    }
    const bool burst = high();
    const bool active = phase_ == Phase::Generating ||
                        ((phase_ == Phase::Pending || phase_ == Phase::Draining) && burst);
    ++now_;
    return {burst, active};
}
} // namespace pop
