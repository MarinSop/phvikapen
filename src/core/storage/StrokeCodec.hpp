#pragma once

#include "core/Error.hpp"
#include "core/ink/Stroke.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace phvikapen::core {

inline constexpr std::uint8_t kStrokeFormatVersion = 2;
// From this version on, a stroke says whether its ends are round.
inline constexpr std::uint8_t kSquareEndsVersion = 2;

inline constexpr float kStoredPositionsPerUnit = 32.0F;
inline constexpr float kStoredPressureSteps = 65535.0F;
inline constexpr float kStoredTiltStepsPerDegree = 100.0F;

[[nodiscard]] std::vector<std::byte> encodeStroke(const Stroke& stroke);

[[nodiscard]] Result<Stroke> decodeStroke(std::span<const std::byte> bytes);

}
