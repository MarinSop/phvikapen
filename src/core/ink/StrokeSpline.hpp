#pragma once

#include "core/ink/InkSample.hpp"

#include <span>
#include <vector>

namespace phvikapen::core {

inline constexpr float kSplineSpacing = 2.0F;

[[nodiscard]] std::vector<InkSample> fitSpline(std::span<const InkSample> samples,
                                               float spacing = kSplineSpacing);

}
