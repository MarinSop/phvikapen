#include "core/filter/InkFilter.hpp"

namespace phvikapen::core {

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

} // namespace phvikapen::core
