#pragma once

#include "core/filter/OneEuroFilter.hpp"
#include "core/ink/InkSample.hpp"

namespace phvikapen::core {

struct InkFilterParameters {
    static constexpr float kPressureMinCutoff = 0.5F;

    // Provisional: not yet tuned on recordings from the real pen.
    static constexpr float kPositionBeta = 0.05F;

    OneEuroParameters position{.beta = kPositionBeta};

    OneEuroParameters pressure{.minCutoff = kPressureMinCutoff};

    friend constexpr bool operator==(const InkFilterParameters&,
                                     const InkFilterParameters&) = default;
};

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

}
