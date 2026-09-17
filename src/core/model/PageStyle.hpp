#pragma once

#include <cstdint>
#include <optional>

namespace phvikapen::core {

enum class Paper : std::uint8_t {
    Infinite,
    A3,
    A4,
    A5,
    Letter,
    Legal,
    Custom,
};

enum class Orientation : std::uint8_t {
    Portrait,
    Landscape,
};

enum class Background : std::uint8_t {
    Blank,
    Lined,
    Grid,
    Dotted,
};

struct PaperSize {
    float width{};
    float height{};

    friend constexpr bool operator==(const PaperSize&, const PaperSize&) = default;
};

inline constexpr float kPageUnitsPerInch = 96.0F;
inline constexpr float kMillimetersPerInch = 25.4F;

[[nodiscard]] constexpr float millimeters(float value) noexcept {
    return value * kPageUnitsPerInch / kMillimetersPerInch;
}

struct PageStyle {
    static constexpr float kDefaultSpacing = 7.0F * kPageUnitsPerInch / kMillimetersPerInch;
    static constexpr float kMinimumSpacing = 2.0F * kPageUnitsPerInch / kMillimetersPerInch;
    static constexpr float kMaximumSpacing = 30.0F * kPageUnitsPerInch / kMillimetersPerInch;

    Paper paper{Paper::A4};
    Orientation orientation{Orientation::Portrait};
    Background background{Background::Lined};
    float spacing{kDefaultSpacing};
    float customWidth{};
    float customHeight{};

    friend constexpr bool operator==(const PageStyle&, const PageStyle&) = default;
};

[[nodiscard]] std::optional<PaperSize> paperSize(const PageStyle& style) noexcept;

[[nodiscard]] PageStyle styleForPaper(PaperSize size) noexcept;

[[nodiscard]] PageStyle normalized(PageStyle style) noexcept;

}
