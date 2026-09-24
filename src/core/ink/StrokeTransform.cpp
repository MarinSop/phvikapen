#include "core/ink/StrokeTransform.hpp"

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr float kHalfTurn = 180.0F;
constexpr float kNothingWorthDoing = 0.0001F;

[[nodiscard]] float radiansOf(float degrees) noexcept {
    return degrees * static_cast<float>(std::numbers::pi) / kHalfTurn;
}

[[nodiscard]] float awayFromZero(float scale) noexcept {
    if (std::abs(scale) < kSmallestScale) {
        return scale < 0.0F ? -kSmallestScale : kSmallestScale;
    }
    return scale;
}

}

Transform normalized(Transform transform) noexcept {
    transform.wide = awayFromZero(transform.wide);
    transform.tall = awayFromZero(transform.tall);
    transform.turn = std::fmod(transform.turn, kFullTurn);
    return transform;
}

bool leavesAsItWas(const Transform& transform) noexcept {
    return std::abs(transform.dx) < kNothingWorthDoing
           && std::abs(transform.dy) < kNothingWorthDoing
           && std::abs(transform.wide - 1.0F) < kNothingWorthDoing
           && std::abs(transform.tall - 1.0F) < kNothingWorthDoing
           && std::abs(std::fmod(transform.turn, kFullTurn)) < kNothingWorthDoing;
}

Point placed(const Transform& transform, Point point) noexcept {
    const float fromPivotX = (point.x - transform.pivot.x) * transform.wide;
    const float fromPivotY = (point.y - transform.pivot.y) * transform.tall;
    const float angle = radiansOf(transform.turn);
    const float cosine = std::cos(angle);
    const float sine = std::sin(angle);
    return Point{
        .x = transform.pivot.x + (fromPivotX * cosine) - (fromPivotY * sine) + transform.dx,
        .y = transform.pivot.y + (fromPivotX * sine) + (fromPivotY * cosine) + transform.dy,
    };
}

Rect placed(const Transform& transform, const Rect& area) noexcept {
    const std::array<Point, 4> corners{
        placed(transform, Point{.x = area.left, .y = area.top}),
        placed(transform, Point{.x = area.right, .y = area.top}),
        placed(transform, Point{.x = area.right, .y = area.bottom}),
        placed(transform, Point{.x = area.left, .y = area.bottom}),
    };
    Rect around{
        .left = corners.front().x,
        .top = corners.front().y,
        .right = corners.front().x,
        .bottom = corners.front().y,
    };
    for (const Point& corner : corners) {
        around.left = std::min(around.left, corner.x);
        around.top = std::min(around.top, corner.y);
        around.right = std::max(around.right, corner.x);
        around.bottom = std::max(around.bottom, corner.y);
    }
    return around;
}

float widthFactor(const Transform& transform) noexcept {
    return std::sqrt(std::abs(transform.wide) * std::abs(transform.tall));
}

Stroke transformed(const Stroke& stroke, const Transform& transform) {
    StrokeStyle style = stroke.style();
    style.width = std::max(style.width * widthFactor(transform), kSmallestScale);
    Stroke moved{stroke.id(), style};
    for (const InkSample& sample : stroke.samples()) {
        const Point at = placed(transform, Point{.x = sample.x, .y = sample.y});
        InkSample shifted = sample;
        shifted.x = at.x;
        shifted.y = at.y;
        moved.append(shifted);
    }
    return moved;
}

std::vector<Stroke> transformed(std::span<const Stroke> strokes, const Transform& transform) {
    std::vector<Stroke> moved;
    moved.reserve(strokes.size());
    for (const Stroke& stroke : strokes) {
        moved.push_back(transformed(stroke, transform));
    }
    return moved;
}

Transform sizingTo(const Rect& from, const Rect& to) noexcept {
    const float wide = from.width() < kNothingWorthDoing ? 1.0F : to.width() / from.width();
    const float tall = from.height() < kNothingWorthDoing ? 1.0F : to.height() / from.height();
    return normalized(Transform{
        .pivot = Point{.x = from.left, .y = from.top},
        .dx = to.left - from.left,
        .dy = to.top - from.top,
        .wide = wide,
        .tall = tall,
        .turn = 0.0F,
    });
}

}
