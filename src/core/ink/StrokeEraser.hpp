#pragma once

#include "core/id/Uuid7Generator.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeHitTest.hpp"

#include <span>
#include <vector>

namespace phvikapen::core {

// What is left of a stroke after the eraser has been over it: the whole stroke when it was not
// touched, nothing when all of it was rubbed out, otherwise the pieces between the rubbed parts.
[[nodiscard]] std::vector<Stroke> erased(const Stroke& stroke, std::span<const EraserSweep> sweeps,
                                         Uuid7Generator& ids);

[[nodiscard]] bool wholeStrokeSurvives(const Stroke& stroke, std::span<const Stroke> pieces) noexcept;

}
