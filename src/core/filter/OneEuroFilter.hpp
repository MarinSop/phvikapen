#pragma once

#include <chrono>
#include <optional>

namespace phvikapen::core {

struct OneEuroParameters {
    float minCutoff{1.0F};

    float beta{0.0F};

    float derivativeCutoff{1.0F};

    friend constexpr bool operator==(const OneEuroParameters&, const OneEuroParameters&) = default;
};

class OneEuroFilter {
public:
    explicit OneEuroFilter(OneEuroParameters parameters = {}) noexcept;

    [[nodiscard]] float filter(float value, std::chrono::microseconds timestamp) noexcept;

    void reset() noexcept;

    [[nodiscard]] const OneEuroParameters& parameters() const noexcept { return m_parameters; }

private:
    OneEuroParameters m_parameters;
    std::optional<std::chrono::microseconds> m_lastTimestamp;
    float m_lastValue{};
    float m_lastFilteredValue{};
    float m_lastSpeed{};
};

}
