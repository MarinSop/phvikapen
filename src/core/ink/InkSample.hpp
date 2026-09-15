#pragma once

#include <chrono>

namespace phvikapen::core {

/// One raw pen sample as reported by an ink backend, before any filtering.
struct InkSample {
    /// Horizontal position in page coordinates (device-independent pixels).
    float x{};
    /// Vertical position in page coordinates; y grows downwards.
    float y{};
    /// Normalized pressure in [0, 1]. Devices without pressure sensing report 1.
    float pressure{1.0F};
    /// Pen tilt in degrees within [-90, 90]; positive values lean towards +x.
    float tiltX{};
    /// Pen tilt in degrees within [-90, 90]; positive values lean towards +y.
    float tiltY{};
    /// Time the sample was taken, on a monotonic clock.
    std::chrono::microseconds timestamp{};

    friend bool operator==(const InkSample&, const InkSample&) = default;
};

} // namespace phvikapen::core
