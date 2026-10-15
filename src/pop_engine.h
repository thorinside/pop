#pragma once
#include <cstdint>

namespace pop {
enum class Termination : uint8_t { Count, Duration, Invalid = 255 };
enum class Timing : uint8_t { Internal, Clock };
enum class Contour : uint8_t { Flat, Acceleration, Deceleration, RiseFall, FallRise, Invalid = 255 };
enum class Phase : uint8_t { Idle, Pending, Generating, Draining, AbortedDraining, FinalDraining };
enum Limit : uint8_t { BaseLimited = 1, DurationLimited = 2, PhysicalLimited = 4 };
enum class Error : uint8_t { None, InvalidSettings, InvalidRouting, NoClock, ClockStopped, ClockTimeout };
struct Settings {
    Termination termination = Termination::Count;
    Timing timing = Timing::Internal;
    int count = 4;
    int durationMs = 500;
    int rateTenths = 200;
    int clockRate = 0;
    int durationBeatsIndex = 2;
    int irregularity = 0;
    Contour contour = Contour::Flat;
    int amount = 100;
};
struct Events {
    bool start = false;
    bool cancel = false;
    bool reset = false;
    bool clock = false;
    bool clockConnected = true;
};
struct Output {
    bool burst;
    bool active;
};
struct Schmitt {
    bool armed = false;
    bool observe(float voltage);
};
uint32_t advance(uint32_t x);
double contourHeight(Contour contour, double u);
uint64_t internalGap(double length, bool onset, uint32_t width, uint32_t gap, uint8_t &limits);
bool valid(const Settings &settings);

struct ClockTracker {
    enum class State : uint8_t { Empty, First, Locked };
    State state = State::Empty;
    uint64_t last = 0, period = 0;
    bool expire(uint64_t now);
    bool observe(uint64_t now, uint32_t fs);
};
struct ClockWait {
    uint64_t ticks;
    uint64_t offset;
};
ClockWait clockWait(double length, uint64_t period, uint64_t ticks, uint64_t previousOffset,
                    bool onset, uint32_t width, uint32_t gap, uint8_t &limits);

class Engine {
  public:
    explicit Engine(uint32_t sampleRate = 48000, uint32_t seed = 1);
    Output tick(const Settings &current, bool retrigger, Events events = {});
    void safetyReset(uint32_t sampleRate);
    uint64_t sample() const { return now_; }
    Phase phase() const { return phase_; }
    Error error() const { return error_; }
    void setError(Error value) { error_ = value; }
    uint8_t limits() const { return limits_; }
    uint32_t requestState() const { return requestRng_; }
    uint32_t generationState() const { return generationRng_; }
    uint64_t acceptedRequests() const { return accepted_; }
    uint64_t generationDraws() const { return draws_; }
    uint32_t emitted() const { return emitted_; }
    uint32_t width() const { return width_; }
    uint32_t gap() const { return gap_; }
#ifdef POP_TESTING
    void injectSeed(uint32_t seed);
#endif
  private:
    void accept(const Settings &settings);
    void activate();
    void abortClock();
    void schedule(bool onset);
    bool high() const { return now_ < fall_; }
    uint32_t fs_, width_, gap_, requestRng_, generationRng_ = 0;
    uint64_t now_ = 0, fall_ = 0, ready_ = 0, activation_ = 0, deadline_ = 0, due_ = 0;
    uint64_t accepted_ = 0, draws_ = 0;
    uint32_t emitted_ = 0;
    Phase phase_ = Phase::Idle;
    Settings snapshot_{};
    Error error_ = Error::None;
    uint8_t limits_ = 0;
    bool dueValid_ = false, musicalHigh_ = false;
    ClockTracker tracker_{};
    uint64_t requestSample_ = 0, period_ = 0, ticks_ = 0;
};
} // namespace pop
