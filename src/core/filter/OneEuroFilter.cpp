#include "core/filter/OneEuroFilter.hpp"

#include <chrono>
#include <cmath>
#include <numbers>

namespace phvikapen::core {
namespace {

constexpr float kMicrosecondsPerSecond = 1e6F;

/// Smoothing factor of an exponential filter with the given cutoff frequency, from the time
/// constant tau = 1 / (2 * pi * cutoff) and the sampling period.
[[nodiscard]] float smoothingFactor(float cutoff, float samplingPeriod) noexcept {
    const float timeConstant = 1.0F / (2.0F * std::numbers::pi_v<float> * cutoff);
    return 1.0F / (1.0F + (timeConstant / samplingPeriod));
}

[[nodiscard]] float smooth(float alpha, float value, float previous) noexcept {
    return (alpha * value) + ((1.0F - alpha) * previous);
}

} // namespace

OneEuroFilter::OneEuroFilter(OneEuroParameters parameters) noexcept : m_parameters{parameters} {}

float OneEuroFilter::filter(float value, std::chrono::microseconds timestamp) noexcept {
    if (!m_lastTimestamp) {
        m_lastTimestamp = timestamp;
        m_lastValue = value;
        m_lastFilteredValue = value;
        m_lastSpeed = 0.0F;
        return value;
    }

    const std::chrono::microseconds elapsed = timestamp - *m_lastTimestamp;
    if (elapsed <= std::chrono::microseconds::zero()) {
        return m_lastFilteredValue;
    }
    const float samplingPeriod = static_cast<float>(elapsed.count()) / kMicrosecondsPerSecond;

    const float speed = (value - m_lastValue) / samplingPeriod;
    const float smoothedSpeed =
        smooth(smoothingFactor(m_parameters.derivativeCutoff, samplingPeriod), speed, m_lastSpeed);

    // The faster the signal moves, the higher the cutoff and the less smoothing is applied.
    const float cutoff = m_parameters.minCutoff + (m_parameters.beta * std::abs(smoothedSpeed));
    const float filtered =
        smooth(smoothingFactor(cutoff, samplingPeriod), value, m_lastFilteredValue);

    m_lastTimestamp = timestamp;
    m_lastValue = value;
    m_lastSpeed = smoothedSpeed;
    m_lastFilteredValue = filtered;
    return filtered;
}

void OneEuroFilter::reset() noexcept {
    m_lastTimestamp.reset();
    m_lastValue = 0.0F;
    m_lastFilteredValue = 0.0F;
    m_lastSpeed = 0.0F;
}

} // namespace phvikapen::core
