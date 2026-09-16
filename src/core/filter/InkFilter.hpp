#pragma once

#include "core/filter/OneEuroFilter.hpp"
#include "core/ink/InkSample.hpp"

namespace phvikapen::core {

/// Filter settings for a pen, one set per filtered signal.
struct InkFilterParameters {
    /// Pressure is smoothed more heavily than position by default.
    static constexpr float kPressureMinCutoff = 0.5F;

    /// Position in page units. The starting point for tuning on recordings from the target device.
    OneEuroParameters position{};

    /// Pressure in [0, 1]. Smoothed more gently than position, because pressure noise shows up as
    /// width wobble along the stroke rather than as a crooked line.
    OneEuroParameters pressure{.minCutoff = kPressureMinCutoff};

    friend constexpr bool operator==(const InkFilterParameters&,
                                     const InkFilterParameters&) = default;
};

/// Smooths the position and the pressure of raw pen samples, which removes the wobble that a
/// slowly drawn line shows. Tilt and timestamp are passed through untouched.
///
/// One filter belongs to one stroke: call reset() before each new stroke.
class InkFilter {
public:
    explicit InkFilter(InkFilterParameters parameters = {}) noexcept;

    [[nodiscard]] InkSample filter(const InkSample& sample) noexcept;

    void reset() noexcept;

    [[nodiscard]] const InkFilterParameters& parameters() const noexcept { return m_parameters; }

private:
    InkFilterParameters m_parameters;
    OneEuroFilter m_x;
    OneEuroFilter m_y;
    OneEuroFilter m_pressure;
};

} // namespace phvikapen::core
