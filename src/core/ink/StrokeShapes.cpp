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

Stroke shaped(const Stroke& stroke, Shape shape, ShapeKeys keys) {
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
        appendEdge(drawn, Point{.x = box.left, .y = box.top}, Point{.x = box.right, .y = box.top},
                   pressure);
        appendEdge(drawn, Point{.x = box.right, .y = box.top},
                   Point{.x = box.right, .y = box.bottom}, pressure);
        appendEdge(drawn, Point{.x = box.right, .y = box.bottom},
                   Point{.x = box.left, .y = box.bottom}, pressure);
        appendEdge(drawn, Point{.x = box.left, .y = box.bottom}, Point{.x = box.left, .y = box.top},
                   pressure);
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
