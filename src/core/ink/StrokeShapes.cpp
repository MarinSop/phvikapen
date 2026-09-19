#include "core/ink/StrokeShapes.hpp"

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace phvikapen::core {
namespace {

constexpr float kEdgeSpacing = 6.0F;
constexpr std::size_t kEllipseCorners = 96;
constexpr std::size_t kSmallestCount = 2;
constexpr float kEighthTurn = std::numbers::pi_v<float> / 4.0F;
constexpr float kQuarterTurn = std::numbers::pi_v<float> / 2.0F;
constexpr float kHalfTurn = std::numbers::pi_v<float>;
constexpr float kThreeQuarterTurn = 3.0F * kQuarterTurn;
constexpr float kHalf = 0.5F;

[[nodiscard]] float averagePressure(const Stroke& stroke) {
    if (stroke.samples().empty()) {
        return 1.0F;
    }
    float total = 0.0F;
    for (const InkSample& sample : stroke.samples()) {
        total += sample.pressure;
    }
    return total / static_cast<float>(stroke.samples().size());
}

[[nodiscard]] std::size_t stepsAlong(float length) {
    return std::max(kSmallestCount, static_cast<std::size_t>(length / kEdgeSpacing));
}

void appendArc(Stroke& into, Point centre, float radius, float from, float to, float pressure) {
    const auto steps = std::max<std::size_t>(
        kSmallestCount, static_cast<std::size_t>(std::abs(to - from) * radius / kEdgeSpacing));
    for (std::size_t i = 0; i <= steps; ++i) {
        const float angle =
            from + ((to - from) * static_cast<float>(i) / static_cast<float>(steps));
        into.append(InkSample{
            .x = centre.x + (radius * std::cos(angle)),
            .y = centre.y + (radius * std::sin(angle)),
            .pressure = pressure,
        });
    }
}

void appendEdge(Stroke& into, Point from, Point to, float pressure) {
    const std::size_t steps = stepsAlong(std::hypot(to.x - from.x, to.y - from.y));
    for (std::size_t i = 0; i <= steps; ++i) {
        const float part = static_cast<float>(i) / static_cast<float>(steps);
        into.append(InkSample{
            .x = from.x + ((to.x - from.x) * part),
            .y = from.y + ((to.y - from.y) * part),
            .pressure = pressure,
        });
    }
}

// Where the pointer went, once the keys that are held have had their say.
[[nodiscard]] Point reachOf(Point from, Point to, Shape shape, ShapeKeys keys) {
    float dx = to.x - from.x;
    float dy = to.y - from.y;

    if (keys.even && shape == Shape::Line) {
        const float length = std::hypot(dx, dy);
        const float turn = std::round(std::atan2(dy, dx) / kEighthTurn) * kEighthTurn;
        dx = length * std::cos(turn);
        dy = length * std::sin(turn);
    } else if (keys.even) {
        const float reach = std::max(std::abs(dx), std::abs(dy));
        dx = std::copysign(reach, dx == 0.0F ? 1.0F : dx);
        dy = std::copysign(reach, dy == 0.0F ? 1.0F : dy);
    }

    return Point{.x = from.x + dx, .y = from.y + dy};
}

}

Rect shapedBox(Point from, Point to, ShapeKeys keys) {
    const Point reach = reachOf(from, to, Shape::Rectangle, keys);
    if (keys.fromCentre) {
        const float dx = std::abs(reach.x - from.x);
        const float dy = std::abs(reach.y - from.y);
        return Rect{
            .left = from.x - dx,
            .top = from.y - dy,
            .right = from.x + dx,
            .bottom = from.y + dy,
        };
    }
    return Rect{
        .left = std::min(from.x, reach.x),
        .top = std::min(from.y, reach.y),
        .right = std::max(from.x, reach.x),
        .bottom = std::max(from.y, reach.y),
    };
}

namespace {

[[nodiscard]] Rect boxOf(Point from, Point to, ShapeKeys keys) {
    if (keys.fromCentre) {
        const float dx = std::abs(to.x - from.x);
        const float dy = std::abs(to.y - from.y);
        return Rect{
            .left = from.x - dx,
            .top = from.y - dy,
            .right = from.x + dx,
            .bottom = from.y + dy,
        };
    }
    return Rect{
        .left = std::min(from.x, to.x),
        .top = std::min(from.y, to.y),
        .right = std::max(from.x, to.x),
        .bottom = std::max(from.y, to.y),
    };
}

}

Stroke shaped(const Stroke& stroke, Shape shape, ShapeKeys keys, float corner) {
    if (shape == Shape::Freehand || stroke.samples().size() < kSmallestCount) {
        return stroke;
    }

    const float pressure = averagePressure(stroke);
    const InkSample& first = stroke.samples().front();
    const InkSample& last = stroke.samples().back();
    const Point from{.x = first.x, .y = first.y};
    const Point to = reachOf(from, Point{.x = last.x, .y = last.y}, shape, keys);
    Stroke drawn{stroke.id(), stroke.style()};

    if (shape == Shape::Line) {
        const Point start =
            keys.fromCentre ? Point{.x = from.x - (to.x - from.x), .y = from.y - (to.y - from.y)}
                            : from;
        appendEdge(drawn, start, to, pressure);
        return drawn;
    }

    const Rect box = boxOf(from, to, keys);
    if (box.width() <= 0.0F && box.height() <= 0.0F) {
        return drawn;
    }

    if (shape == Shape::Rectangle) {
        const float round = std::clamp(corner, 0.0F, std::min(box.width(), box.height()) * kHalf);
        const float left = box.left;
        const float right = box.right;
        const float top = box.top;
        const float bottom = box.bottom;
        appendEdge(drawn, Point{.x = left + round, .y = top}, Point{.x = right - round, .y = top},
                   pressure);
        if (round > 0.0F) {
            appendArc(drawn, Point{.x = right - round, .y = top + round}, round, -kQuarterTurn,
                      0.0F, pressure);
        }
        appendEdge(drawn, Point{.x = right, .y = top + round},
                   Point{.x = right, .y = bottom - round}, pressure);
        if (round > 0.0F) {
            appendArc(drawn, Point{.x = right - round, .y = bottom - round}, round, 0.0F,
                      kQuarterTurn, pressure);
        }
        appendEdge(drawn, Point{.x = right - round, .y = bottom},
                   Point{.x = left + round, .y = bottom}, pressure);
        if (round > 0.0F) {
            appendArc(drawn, Point{.x = left + round, .y = bottom - round}, round, kQuarterTurn,
                      kHalfTurn, pressure);
        }
        appendEdge(drawn, Point{.x = left, .y = bottom - round}, Point{.x = left, .y = top + round},
                   pressure);
        if (round > 0.0F) {
            appendArc(drawn, Point{.x = left + round, .y = top + round}, round, kHalfTurn,
                      kThreeQuarterTurn, pressure);
        }
        return drawn;
    }

    const float centreX = (box.left + box.right) / 2.0F;
    const float centreY = (box.top + box.bottom) / 2.0F;
    const float radiusX = box.width() / 2.0F;
    const float radiusY = box.height() / 2.0F;
    for (std::size_t i = 0; i <= kEllipseCorners; ++i) {
        const float angle = 2.0F * std::numbers::pi_v<float>
                            * static_cast<float>(i) / static_cast<float>(kEllipseCorners);
        drawn.append(InkSample{
            .x = centreX + (radiusX * std::cos(angle)),
            .y = centreY + (radiusY * std::sin(angle)),
            .pressure = pressure,
        });
    }
    return drawn;
}

}
