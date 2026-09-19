#include "core/ink/Stroke.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace phvikapen::core {
namespace {

constexpr float kHalf = 0.5F;
constexpr float kLightestTouch = 0.4F;
constexpr float kPressureCurve = 0.6F;

}

float widthAt(const StrokeStyle& style, float pressure) noexcept {
    const float firmness = std::pow(std::clamp(pressure, 0.0F, 1.0F), kPressureCurve);
    return style.width * (kLightestTouch + ((1.0F - kLightestTouch) * firmness));
}

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

}
