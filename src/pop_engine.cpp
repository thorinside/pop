#include "pop_engine.h"
#include "pop_sample_math.h"
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
    const auto rounded = ceilIntervalSamples(length);
    const uint64_t minimum = onset ? 1 : static_cast<uint64_t>(width) + gap;
    if (rounded < minimum) {
        limits |= PhysicalLimited;
        return minimum;
    }
    return rounded;
}
bool ClockTracker::expire(uint64_t now) {
    if (state != State::Locked || now < last + 3 * period)
        return false;
    state = State::Empty;
    return true;
}
bool ClockTracker::observe(uint64_t now, uint32_t fs) {
    if (state == State::Empty) {
        last = now;
        state = State::First;
        return false;
    }
    const uint64_t measured = now - last;
    if (measured < millisecondsSamples(fs, 50) || measured > 2ull * fs) {
        state = State::Empty;
        return true;
    }
    last = now;
    period = measured;
    state = State::Locked;
    return false;
}
ClockWait clockWait(double length, uint64_t period, uint64_t ticks, uint64_t previousOffset,
                    bool onset, uint32_t width, uint32_t gap, uint8_t &limits) {
    for (unsigned shift = 0; shift < 12; ++shift) {
        const uint64_t increment = uint64_t{1} << shift;
        // Division by 16 is exact in binary64. Selection never uses ceil(length).
        if (sampleCountAsDouble(period << shift) / 16.0 < length)
            continue;
        const uint64_t next = ticks + increment;
        const uint64_t offset = (next * period + 15) >> 4;
        const uint64_t minimum = onset ? 1 : previousOffset + width + gap;
        if (offset >= minimum)
            return {next, offset};
        limits |= PhysicalLimited;
    }
    // The fixed set covers valid P >= ceil(0.05 Fs), L <= 4 Fs and W+G.
    return {ticks, previousOffset};
}
bool valid(const Settings &s) {
    return (s.termination == Termination::Count || s.termination == Termination::Duration) &&
           (s.timing == Timing::Internal || s.timing == Timing::Clock) &&
           (s.termination != Termination::Count || (s.count >= 1 && s.count <= 64)) &&
           (s.termination != Termination::Duration ||
            (s.durationMs >= 100 && s.durationMs <= 30000)) &&
           s.rateTenths >= 10 && s.rateTenths <= 1000 &&
           s.clockRate >= -3 && s.clockRate <= 3 &&
           s.durationBeatsIndex >= 0 && s.durationBeatsIndex <= 7 && s.irregularity >= 0 &&
           s.irregularity <= 100 && s.amount >= 0 && s.amount <= 100 &&
           static_cast<unsigned>(s.contour) <= static_cast<unsigned>(Contour::FallRise);
}
Engine::Engine(uint32_t sampleRate, uint32_t seed)
    : fs_(sampleRate), width_(0), gap_(0), requestRng_(seed ? seed : 0x6d2b79f5u) {
    safetyReset(sampleRate);
}
void Engine::safetyReset(uint32_t sampleRate) {
    fs_ = sampleRate ? sampleRate : 48000;
    width_ = static_cast<uint32_t>(millisecondsSamples(fs_, 5));
    gap_ = static_cast<uint32_t>(millisecondsSamples(fs_, 1));
    phase_ = Phase::Idle;
    fall_ = ready_ = now_;
    dueValid_ = false;
    emitted_ = 0;
    generationRng_ = 0;
    limits_ = 0;
    error_ = Error::None;
    musicalHigh_ = false;
    tracker_ = {};
    period_ = ticks_ = 0;
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
    requestSample_ = now_;
}
void Engine::abortClock() {
    if (phase_ == Phase::Generating && snapshot_.timing == Timing::Clock) {
        dueValid_ = false;
        phase_ = high() ? Phase::AbortedDraining : Phase::Idle;
        error_ = Error::ClockStopped;
        musicalHigh_ = false;
    }
}
void Engine::activate() {
    activation_ = now_;
    period_ = tracker_.period;
    ticks_ = 0;
    const uint64_t duration = snapshot_.timing == Timing::Internal
        ? millisecondsSamples(fs_, static_cast<uint32_t>(snapshot_.durationMs))
        : snapshot_.durationBeatsIndex < 2
            ? (period_ + ((uint64_t{1} << (2 - snapshot_.durationBeatsIndex)) - 1)) >> (2 - snapshot_.durationBeatsIndex)
            : period_ << (snapshot_.durationBeatsIndex - 2);
    deadline_ = now_ + duration;
    phase_ = Phase::Generating;
    schedule(true);
}
void Engine::schedule(bool onset) {
    double u = 0.5;
    if (snapshot_.termination == Termination::Duration) {
        u = sampleCountAsDouble(now_ - activation_) / sampleCountAsDouble(deadline_ - activation_);
    } else if (snapshot_.count > 2) {
        u = static_cast<double>(emitted_) / static_cast<double>(snapshot_.count - 1);
    }
    double base = snapshot_.timing == Timing::Clock
        ? sampleCountAsDouble(period_) * std::exp2(-snapshot_.clockRate) / static_cast<double>(fs_)
        : 10.0 / static_cast<double>(snapshot_.rateTenths);
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
    // Preserve unclamped Clock algebra exactly, avoiding P/Fs*Fs roundoff.
    const double baseSamples = snapshot_.timing == Timing::Clock && !(limits_ & BaseLimited)
        ? sampleCountAsDouble(period_) * std::exp2(-snapshot_.clockRate)
        : static_cast<double>(fs_) * base;
    double length = baseSamples * scale * jitter;
    if (length < 1.0) {
        length = 1.0;
        limits_ |= DurationLimited;
    }
    if (length > 4.0 * fs_) {
        length = 4.0 * fs_;
        limits_ |= DurationLimited;
    }
    if (snapshot_.timing == Timing::Clock) {
        const auto wait = clockWait(length, period_, ticks_, now_ - activation_, onset, width_, gap_, limits_);
        ticks_ = wait.ticks;
        due_ = activation_ + wait.offset;
    } else {
        due_ = now_ + internalGap(length, onset, width_, gap_, limits_);
    }
    dueValid_ = snapshot_.termination == Termination::Count || due_ < deadline_;
}
Output Engine::tick(const Settings &current, bool retrigger, Events events) {
    if (events.reset) {
        safetyReset(fs_);
    } else if (events.cancel) {
        dueValid_ = false;
        phase_ = high() ? Phase::Draining : Phase::Idle;
    } else {
        if (tracker_.expire(now_))
            abortClock();
        if (phase_ == Phase::Pending && snapshot_.timing == Timing::Clock &&
            now_ >= requestSample_ + 6ull * fs_) {
            dueValid_ = false;
            phase_ = high() ? Phase::Draining : Phase::Idle;
            error_ = Error::ClockTimeout;
        }
        if (phase_ == Phase::Generating && snapshot_.termination == Termination::Duration &&
            now_ >= deadline_) {
            dueValid_ = false;
            phase_ = high() ? Phase::Draining : Phase::Idle;
        }
        if ((phase_ == Phase::Draining || phase_ == Phase::AbortedDraining) && !high())
            phase_ = Phase::Idle;
        if (events.clock && tracker_.observe(now_, fs_))
            abortClock();
        if (events.start && (phase_ == Phase::Idle || phase_ == Phase::AbortedDraining || retrigger)) {
            if (!valid(current))
                error_ = Error::InvalidSettings;
            else if (current.timing == Timing::Clock && !events.clockConnected)
                error_ = Error::NoClock;
            else
                accept(current);
        }
        if (phase_ == Phase::Pending && now_ >= ready_ &&
            (snapshot_.timing == Timing::Internal ||
             (events.clock && tracker_.state == ClockTracker::State::Locked && now_ > requestSample_)))
            activate();
        if (phase_ == Phase::Generating && dueValid_ && now_ == due_) {
            musicalHigh_ = true;
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
                        ((phase_ == Phase::Pending || phase_ == Phase::Draining) && burst && musicalHigh_);
    ++now_;
    return {burst, active};
}
} // namespace pop
