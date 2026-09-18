#include "core/ink/StrokeShapes.hpp"

#include "core/geometry/Rect.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>

namespace phvikapen::core {
namespace {

constexpr float kEdgeSpacing = 6.0F;
constexpr std::size_t kEllipseCorners = 96;
constexpr std::size_t kSmallestCount = 2;

[[nodiscard]] std::optional<Rect> sampleBounds(const Stroke& stroke) {
    if (stroke.samples().empty()) {
        return std::nullopt;
    }
    const InkSample& first = stroke.samples().front();
    Rect bounds{.left = first.x, .top = first.y, .right = first.x, .bottom = first.y};
    for (const InkSample& sample : stroke.samples()) {
        bounds.left = std::min(bounds.left, sample.x);
        bounds.top = std::min(bounds.top, sample.y);
        bounds.right = std::max(bounds.right, sample.x);
        bounds.bottom = std::max(bounds.bottom, sample.y);
    }
    return bounds;
}

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

void appendEdge(Stroke& into, float fromX, float fromY, float toX, float toY, float pressure) {
    const std::size_t steps = stepsAlong(std::hypot(toX - fromX, toY - fromY));
    for (std::size_t i = 0; i <= steps; ++i) {
        const float part = static_cast<float>(i) / static_cast<float>(steps);
        into.append(InkSample{
            .x = fromX + ((toX - fromX) * part),
            .y = fromY + ((toY - fromY) * part),
            .pressure = pressure,
        });
    }
}

}

Stroke shaped(const Stroke& stroke, Shape shape) {
    const std::optional<Rect> bounds = sampleBounds(stroke);
    if (shape == Shape::Freehand || stroke.samples().size() < kSmallestCount || !bounds) {
        return stroke;
    }

    const float pressure = averagePressure(stroke);
    Stroke drawn{stroke.id(), stroke.style()};

    if (shape == Shape::Line) {
        const InkSample& from = stroke.samples().front();
        const InkSample& to = stroke.samples().back();
        appendEdge(drawn, from.x, from.y, to.x, to.y, pressure);
        return drawn;
    }

    if (shape == Shape::Rectangle) {
        appendEdge(drawn, bounds->left, bounds->top, bounds->right, bounds->top, pressure);
        appendEdge(drawn, bounds->right, bounds->top, bounds->right, bounds->bottom, pressure);
        appendEdge(drawn, bounds->right, bounds->bottom, bounds->left, bounds->bottom, pressure);
        appendEdge(drawn, bounds->left, bounds->bottom, bounds->left, bounds->top, pressure);
        return drawn;
    }

    const float centerX = (bounds->left + bounds->right) / 2.0F;
    const float centerY = (bounds->top + bounds->bottom) / 2.0F;
    const float radiusX = bounds->width() / 2.0F;
    const float radiusY = bounds->height() / 2.0F;
    for (std::size_t i = 0; i <= kEllipseCorners; ++i) {
        const float angle = 2.0F * std::numbers::pi_v<float>
                            * static_cast<float>(i) / static_cast<float>(kEllipseCorners);
        drawn.append(InkSample{
            .x = centerX + (radiusX * std::cos(angle)),
            .y = centerY + (radiusY * std::sin(angle)),
            .pressure = pressure,
        });
    }
    return drawn;
}

}
