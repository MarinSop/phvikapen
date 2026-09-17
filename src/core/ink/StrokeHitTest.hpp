#pragma once

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/ink/Stroke.hpp"

namespace phvikapen::core {

struct EraserSweep {
    Point from;
    Point to;
    float radius{};

    [[nodiscard]] Rect bounds() const noexcept;
};

[[nodiscard]] bool touches(const Stroke& stroke, const EraserSweep& sweep) noexcept;

}
