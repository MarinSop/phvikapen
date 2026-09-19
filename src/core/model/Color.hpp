#pragma once

#include <cstdint>

namespace phvikapen::core {

struct Color {
    static constexpr std::uint8_t kOpaque = 255;

    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};
    std::uint8_t alpha{kOpaque};

    friend constexpr bool operator==(const Color&, const Color&) = default;
};

// A colour as one number, the way it is written down: 0xAARRGGBB.
inline constexpr unsigned int kAlphaShift = 24U;
inline constexpr unsigned int kRedShift = 16U;
inline constexpr unsigned int kGreenShift = 8U;
inline constexpr std::uint32_t kChannel = 0xFFU;

[[nodiscard]] constexpr std::uint32_t packed(Color color) noexcept {
    return (static_cast<std::uint32_t>(color.alpha) << kAlphaShift)
           | (static_cast<std::uint32_t>(color.red) << kRedShift)
           | (static_cast<std::uint32_t>(color.green) << kGreenShift)
           | static_cast<std::uint32_t>(color.blue);
}

[[nodiscard]] constexpr Color unpacked(std::uint32_t value) noexcept {
    return Color{
        .red = static_cast<std::uint8_t>((value >> kRedShift) & kChannel),
        .green = static_cast<std::uint8_t>((value >> kGreenShift) & kChannel),
        .blue = static_cast<std::uint8_t>(value & kChannel),
        .alpha = static_cast<std::uint8_t>((value >> kAlphaShift) & kChannel),
    };
}

}
