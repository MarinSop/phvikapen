#include "core/ink/StrokeHitTest.hpp"

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>

namespace phvikapen::core {
namespace {

constexpr float kHalf = 0.5F;

[[nodiscard]] bool segmentTouches(const InkSample& start, const InkSample& end,
                                  const StrokeStyle& style, const EraserSweep& sweep) noexcept {
    const float halfWidth = widthAt(style, std::max(start.pressure, end.pressure)) * kHalf;
    const float reach = sweep.radius + halfWidth;
    return squaredDistanceBetweenSegments({.x = start.x, .y = start.y}, {.x = end.x, .y = end.y},
                                          sweep.from, sweep.to)
           <= reach * reach;
}

}

Rect EraserSweep::bounds() const noexcept {
    return Rect{
        .left = std::min(from.x, to.x),
        .top = std::min(from.y, to.y),
        .right = std::max(from.x, to.x),
        .bottom = std::max(from.y, to.y),
    }
        .inflated(radius);
}

bool touches(const Stroke& stroke, const EraserSweep& sweep) noexcept {
    const std::optional<Rect> strokeBounds = stroke.boundingBox();
    if (!strokeBounds || !strokeBounds->intersects(sweep.bounds())) {
        return false;
    }

    const std::span<const InkSample> samples = stroke.samples();
    const StrokeStyle& style = stroke.style();
    if (samples.size() == 1) {
        return segmentTouches(samples.front(), samples.front(), style, sweep);
    }
    for (std::size_t i = 1; i < samples.size(); ++i) {
        if (segmentTouches(samples[i - 1], samples[i], style, sweep)) {
            return true;
        }
    }
    return false;
}

}
