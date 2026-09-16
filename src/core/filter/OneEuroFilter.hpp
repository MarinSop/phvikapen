#pragma once

#include <chrono>
#include <optional>

namespace phvikapen::core {

/// Parameters of the 1 euro filter (Casiez, Roussel and Vogel, CHI 2012).
///
/// Tuning, in this order: leave beta at zero and lower minCutoff until slow movement stops
/// jittering, then raise beta until fast movement stops lagging behind.
struct OneEuroParameters {
    /// Cutoff frequency in hertz used when the signal barely moves. Lower means smoother.
    float minCutoff{1.0F};

    /// How strongly the cutoff follows the speed of the signal, the beta of the paper. Zero means
    /// no adaptation, so slow and fast movement are smoothed alike. Its scale depends on the unit
    /// of the filtered signal, which is why it is tuned per signal on real recordings.
    float beta{0.0F};

    /// Cutoff frequency in hertz for the speed estimate itself.
    float derivativeCutoff{1.0F};

    friend constexpr bool operator==(const OneEuroParameters&, const OneEuroParameters&) = default;
};

/// One scalar channel of the 1 euro filter.
///
/// The filter is an exponential smoother whose cutoff frequency rises with the measured speed of
/// the signal: slow movement, where jitter is visible, is smoothed heavily, while fast movement,
/// where lag is visible, is barely smoothed at all.
///
/// Samples must arrive with increasing timestamps. A sample that does not advance the clock
/// changes nothing and returns the previous result, because its sampling period is unknown.
class OneEuroFilter {
public:
    explicit OneEuroFilter(OneEuroParameters parameters = {}) noexcept;

    /// Returns the filtered value of @p value taken at @p timestamp.
    /// The first sample of a stroke passes through unchanged.
    [[nodiscard]] float filter(float value, std::chrono::microseconds timestamp) noexcept;

    /// Forgets the history, so that the next sample starts a new stroke.
    void reset() noexcept;

    [[nodiscard]] const OneEuroParameters& parameters() const noexcept { return m_parameters; }

private:
    OneEuroParameters m_parameters;
    std::optional<std::chrono::microseconds> m_lastTimestamp;
    float m_lastValue{};
    float m_lastFilteredValue{};
    float m_lastSpeed{};
};

} // namespace phvikapen::core
