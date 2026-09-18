#include "core/filter/InkFilter.hpp"

#include "core/filter/OneEuroFilter.hpp"

#include <algorithm>
#include <cmath>

namespace phvikapen::core {
namespace {

constexpr float kRawCutoff = 30.0F;
constexpr float kSmoothestCutoff = 0.4F;
constexpr float kSlowestPressureCutoff = 0.5F;

}

InkFilterParameters smoothingOf(float amount) noexcept {
    const float wanted = std::clamp(amount, 0.0F, 1.0F);
    const float cutoff = kRawCutoff * std::pow(kSmoothestCutoff / kRawCutoff, wanted);
    return InkFilterParameters{
        .position =
            OneEuroParameters{.minCutoff = cutoff, .beta = InkFilterParameters::kPositionBeta},
        .pressure = OneEuroParameters{.minCutoff = std::max(cutoff, kSlowestPressureCutoff)},
    };
}

void InkFilter::setParameters(InkFilterParameters parameters) noexcept {
    m_parameters = parameters;
    m_x = OneEuroFilter{parameters.position};
    m_y = OneEuroFilter{parameters.position};
    m_pressure = OneEuroFilter{parameters.pressure};
}

InkFilter::InkFilter(InkFilterParameters parameters) noexcept
    : m_parameters{parameters}, m_x{parameters.position}, m_y{parameters.position},
      m_pressure{parameters.pressure} {}

InkSample InkFilter::filter(const InkSample& sample) noexcept {
    InkSample filtered = sample;
    filtered.x = m_x.filter(sample.x, sample.timestamp);
    filtered.y = m_y.filter(sample.y, sample.timestamp);
    filtered.pressure = m_pressure.filter(sample.pressure, sample.timestamp);
    return filtered;
}

void InkFilter::reset() noexcept {
    m_x.reset();
    m_y.reset();
    m_pressure.reset();
}

}
