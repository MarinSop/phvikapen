#pragma once

#include "core/id/Uuid7Generator.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeHitTest.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>

namespace phvikapen::core {

// What the eraser takes: the part of a line it was rubbed over, or the whole line the moment it
// is touched.
enum class EraseMode : std::uint8_t {
    Touched,
    WholeStroke,
};

// A whole line goes at the lightest touch, so the eraser reaches far less than its own width;
// otherwise every line near the one that was meant would go with it.
inline constexpr float kWholeLineReach = 0.25F;
inline constexpr float kNarrowestReach = 1.0F;

[[nodiscard]] constexpr float reachOf(float radius, EraseMode mode) noexcept {
    return mode == EraseMode::WholeStroke ? std::max(radius * kWholeLineReach, kNarrowestReach)
                                          : radius;
}

// What is left of a stroke after the eraser has been over it: the whole stroke when it was not
// touched, nothing when all of it was rubbed out, otherwise the pieces between the rubbed parts.
[[nodiscard]] std::vector<Stroke> erased(const Stroke& stroke, std::span<const EraserSweep> sweeps,
                                         Uuid7Generator& ids);

[[nodiscard]] bool wholeStrokeSurvives(const Stroke& stroke,
                                       std::span<const Stroke> pieces) noexcept;

}
