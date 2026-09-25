#pragma once

#include "core/ink/InkSample.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace phvikapen::core {

// How far either side of a sample the line is looked at to tell how it turns there. A hand moving
// slowly leaves samples a fraction of a page unit apart, and the angle between three of those says
// nothing about the line: it says how coarsely the pen reports where it is.
inline constexpr float kTurnReach = 2.0F;

// How sharply a line must turn to be a corner rather than a curve, in radians. A curve of the
// tightness handwriting is made of turns by well under this over the stretch looked at.
inline constexpr float kCornerTurn = 1.05F;

// How far along the line each sample lies from the first, which is the measure everything here
// works in: a hand that slows down leaves more samples over the same stretch, not a different line.
[[nodiscard]] std::vector<float> alongOf(std::span<const InkSample> samples);

// How sharply the line turns at one sample, in radians, measured over a stretch of it either side.
// A sample too near either end to have a stretch on both sides does not turn at all.
[[nodiscard]] float turnAt(std::span<const InkSample> samples, std::span<const float> along,
                           std::size_t index, float reach) noexcept;

// Whether the line turns sharply enough at a sample, and more sharply than anywhere close by, for
// it to stand at a corner. The second half of that is what keeps one corner to one sample instead
// of a run of them, so that the line either side of it can still be evened out.
[[nodiscard]] bool isCornerAt(std::span<const float> turns, std::span<const float> along,
                              std::size_t index, float reach) noexcept;

// Which samples stand at a corner, worked out in one pass.
[[nodiscard]] std::vector<bool> cornersAlong(std::span<const InkSample> samples, float reach);

}
