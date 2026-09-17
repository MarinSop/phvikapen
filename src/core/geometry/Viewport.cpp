#include "core/geometry/Viewport.hpp"

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/model/PageStyle.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace phvikapen::core {
namespace {

constexpr float kHalf = 0.5F;
constexpr float kBothSides = 2.0F;

[[nodiscard]] float clampedScale(float scale) noexcept {
    if (!std::isfinite(scale)) {
        return 1.0F;
    }
    return std::clamp(scale, Viewport::kMinimumScale, Viewport::kMaximumScale);
}

[[nodiscard]] float keepAxisInView(float origin, float visible, float paper) noexcept {
    const float margin = Viewport::kPaperMargin;
    if (paper + (kBothSides * margin) <= visible) {
        return (paper - visible) * kHalf;
    }
    return std::clamp(origin, -margin, paper + margin - visible);
}

}

Point Viewport::toPage(Point view) const noexcept {
    return {.x = m_origin.x + (view.x / m_scale), .y = m_origin.y + (view.y / m_scale)};
}

Point Viewport::toView(Point page) const noexcept {
    return {.x = (page.x - m_origin.x) * m_scale, .y = (page.y - m_origin.y) * m_scale};
}

Rect Viewport::visiblePage(ViewSize view) const noexcept {
    return {
        .left = m_origin.x,
        .top = m_origin.y,
        .right = m_origin.x + (view.width / m_scale),
        .bottom = m_origin.y + (view.height / m_scale),
    };
}

void Viewport::panBy(float viewDeltaX, float viewDeltaY) noexcept {
    m_origin.x -= viewDeltaX / m_scale;
    m_origin.y -= viewDeltaY / m_scale;
}

void Viewport::zoomAround(Point view, float factor) noexcept {
    const Point anchor = toPage(view);
    m_scale = clampedScale(m_scale * factor);
    m_origin = {.x = anchor.x - (view.x / m_scale), .y = anchor.y - (view.y / m_scale)};
}

void Viewport::setScale(float scale) noexcept {
    m_scale = clampedScale(scale);
}

void Viewport::fit(ViewSize view, std::optional<PaperSize> paper) noexcept {
    m_scale = 1.0F;
    m_origin = {};
    if (!paper || view.width <= 0.0F) {
        return;
    }
    const float available = view.width - (kBothSides * kPaperMargin);
    if (available > 0.0F && paper->width > available) {
        m_scale = clampedScale(available / paper->width);
    }
    m_origin.y = -kPaperMargin / m_scale;
    keepPaperInView(view, paper);
}

void Viewport::keepPaperInView(ViewSize view, std::optional<PaperSize> paper) noexcept {
    if (!paper) {
        return;
    }
    m_origin.x = keepAxisInView(m_origin.x, view.width / m_scale, paper->width);
    const float halfVisibleHeight = view.height / m_scale * kHalf;
    m_origin.y = std::clamp(m_origin.y, -halfVisibleHeight, paper->height - halfVisibleHeight);
}

}
