#pragma once

#include "core/ink/Stroke.hpp"

#include <cstdint>

namespace phvikapen::core {

enum class Shape : std::uint8_t {
    Freehand,
    Line,
    Rectangle,
    Ellipse,
};

[[nodiscard]] Stroke shaped(const Stroke& stroke, Shape shape);

}
