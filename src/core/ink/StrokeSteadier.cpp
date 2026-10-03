#include "core/ink/StrokeSteadier.hpp"

#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

namespace phvikapen::core {
namespace {

// A sample closer than this to the one before it adds nothing a reader could see.
constexpr float kSettle = 0.05F;
constexpr float kNothing = 1e-6F;
constexpr float kHalf = 0.5F;
// The weights reach three standard deviations out, where a Gaussian has all but faded.
constexpr float kSpread = 3.0F;

[[nodiscard]] float falloff(float offset, float half) noexcept {
    const float closeness = 1.0F - ((offset / half) * (offset / half));
    return closeness * closeness * closeness;
}

}

StrokeSteadier::StrokeSteadier(float reach) noexcept
    : m_reach{std::isfinite(reach) ? std::max(reach, 0.0F) : 0.0F} {}

void StrokeSteadier::append(const InkSample& sample) {
    const float before = m_along.empty() ? 0.0F : m_along.back();
    if (!m_drawn.empty()
        && std::hypot(sample.x - m_drawn.back().x, sample.y - m_drawn.back().y) < kSettle) {
        InkSample& last = m_drawn.back();
        last.pressure = sample.pressure;
        last.tiltX = sample.tiltX;
        last.tiltY = sample.tiltY;
        last.timestamp = std::max(last.timestamp, sample.timestamp);
    } else {
        const float along =
            m_drawn.empty()
                ? 0.0F
                : before + std::hypot(sample.x - m_drawn.back().x, sample.y - m_drawn.back().y);
        m_drawn.push_back(sample);
        m_along.push_back(along);
        m_steadied.push_back(sample);
    }

    // Only the samples close enough to the old end to have felt it are worked out again.
    std::size_t first = m_drawn.size() - 1;
    while (first > 0 && before - m_along[first - 1] <= kSpread * m_reach) {
        --first;
    }
    m_unsettled = std::min(m_unsettled, first);
}

std::span<const InkSample> StrokeSteadier::settled() {
    for (std::size_t i = m_unsettled; i < m_drawn.size(); ++i) {
        steady(i);
    }
    m_unsettled = m_drawn.size();
    return m_steadied;
}

void StrokeSteadier::steady(std::size_t index) {
    const InkSample& here = m_drawn[index];
    const float at = m_along[index];
    const float half = std::min({kSpread * m_reach, at, m_along.back() - at});
    if (half <= kNothing) {
        m_steadied[index] = here;
        return;
    }

    const std::size_t last = m_drawn.size() - 1;
    // A sample weighs as much of the line as lies halfway to its neighbours.
    const auto stretch = [&](std::size_t i) {
        const float from = m_along[i == 0 ? 0 : i - 1];
        const float to = m_along[std::min(i + 1, last)];
        return std::max((to - from) * kHalf, kNothing);
    };

    float x = 0.0F;
    float y = 0.0F;
    float pressure = 0.0F;
    float total = 0.0F;
    const auto gather = [&](std::size_t i) {
        const float weight = falloff(m_along[i] - at, half) * stretch(i);
        x += m_drawn[i].x * weight;
        y += m_drawn[i].y * weight;
        pressure += m_drawn[i].pressure * weight;
        total += weight;
    };
    for (std::size_t i = index; i > 0 && at - m_along[i - 1] < half; --i) {
        gather(i - 1);
    }
    for (std::size_t i = index; i <= last && m_along[i] - at < half; ++i) {
        gather(i);
    }

    InkSample result = here;
    result.x = x / total;
    result.y = y / total;
    result.pressure = pressure / total;
    m_steadied[index] = result;
}

std::vector<InkSample> steadied(std::span<const InkSample> samples, float reach) {
    StrokeSteadier steadier{reach};
    for (const InkSample& sample : samples) {
        steadier.append(sample);
    }
    const std::span<const InkSample> result = steadier.settled();
    return {result.begin(), result.end()};
}

Stroke steadied(const Stroke& stroke, float reach) {
    Stroke result{stroke.id(), stroke.style()};
    for (const InkSample& sample : steadied(stroke.samples(), reach)) {
        result.append(sample);
    }
    return result;
}

}
