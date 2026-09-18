#pragma once

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/ink/Stroke.hpp"

#include <cstdint>

namespace phvikapen::core {

enum class Shape : std::uint8_t {
    Freehand,
    Line,
    Rectangle,
    Ellipse,
};

struct ShapeKeys {
    bool even{};       // a square, a circle, or a line at a whole eighth of a turn
    bool fromCentre{}; // the shape grows out of where the stroke started
};

[[nodiscard]] Rect shapedBox(Point from, Point to, ShapeKeys keys = {});

[[nodiscard]] Stroke shaped(const Stroke& stroke, Shape shape, ShapeKeys keys = {});

}
