#pragma once

#include "core/geometry/Distance.hpp"
#include "core/ink/Stroke.hpp"

#include <vector>

namespace phvikapen::core {

[[nodiscard]] std::vector<Point> strokeOutline(const Stroke& stroke);

}
