#pragma once

#include "core/Error.hpp"
#include "core/ink/Stroke.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace phvikapen::core {

/// Version of the binary stroke format. Readers refuse anything newer than they know.
inline constexpr std::uint8_t kStrokeFormatVersion = 1;

/// Grid the stored values are snapped to, all finer than a pen can resolve.
inline constexpr float kStoredPositionsPerUnit = 32.0F;    ///< 1/32 of a page unit.
inline constexpr float kStoredPressureSteps = 65535.0F;    ///< One part in 65535.
inline constexpr float kStoredTiltStepsPerDegree = 100.0F; ///< One hundredth of a degree.

/// Encodes @p stroke into the binary form a notebook stores.
///
/// Layout: the format version, the 16 identifier bytes, the style, the number of samples, and
/// then the samples. Every sample field is quantized to the grid above and written as the zigzag
/// varint difference to the field of the previous sample, which keeps a smooth stroke down to a
/// few bytes per sample. Timestamps survive exactly; the other fields survive to within one step
/// of their grid.
[[nodiscard]] std::vector<std::byte> encodeStroke(const Stroke& stroke);

/// Returns the stroke stored in @p bytes.
///
/// Fails with ErrorCode::InvalidArgument when the data is truncated or malformed, and with
/// ErrorCode::Unsupported when it was written by a newer version of the format.
[[nodiscard]] Result<Stroke> decodeStroke(std::span<const std::byte> bytes);

} // namespace phvikapen::core
