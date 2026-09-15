#include "core/ink/Stroke.hpp"

#include <optional>

namespace phvikapen::core {
namespace {

constexpr float kHalf = 0.5F;

} // namespace

Stroke::Stroke(Uuid id, StrokeStyle style) noexcept : m_id{id}, m_style{style} {}

void Stroke::append(const InkSample& sample) {
    m_samples.push_back(sample);

    const Rect point{.left = sample.x, .top = sample.y, .right = sample.x, .bottom = sample.y};
    m_sampleBounds = m_sampleBounds ? m_sampleBounds->united(point) : point;
}

std::optional<Rect> Stroke::boundingBox() const noexcept {
    if (!m_sampleBounds) {
        return std::nullopt;
    }
    return m_sampleBounds->inflated(m_style.width * kHalf);
}

} // namespace phvikapen::core
