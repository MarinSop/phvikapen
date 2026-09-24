#pragma once

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Color.hpp"
#include "core/model/PageStyle.hpp"

#include <cstdint>
#include <string>

namespace phvikapen::core {

enum class TextAlign : std::uint8_t {
    Left,
    Center,
    Right,
    Justify,
};

inline constexpr float kPointsPerInch = 72.0F;

// A point of type, in the units a page is measured in.
[[nodiscard]] constexpr float pageUnitsOfPoints(float points) noexcept {
    return points * kPageUnitsPerInch / kPointsPerInch;
}

[[nodiscard]] constexpr float pointsOfPageUnits(float units) noexcept {
    return units * kPointsPerInch / kPageUnitsPerInch;
}

struct TextStyle {
    static constexpr float kDefaultSize = 14.0F;
    static constexpr float kSmallestSize = 6.0F;
    static constexpr float kLargestSize = 144.0F;
    static constexpr float kDefaultLineHeight = 1.0F;
    static constexpr float kTightestLines = 0.6F;
    static constexpr float kLoosestLines = 3.0F;
    static constexpr Color kDefaultColor{.red = 0, .green = 0, .blue = 0, .alpha = Color::kOpaque};

    std::string font;
    float size{kDefaultSize};
    Color color{kDefaultColor};
    TextAlign align{TextAlign::Left};
    float lineHeight{kDefaultLineHeight};
    bool bold{};
    bool italic{};
    bool underline{};
    bool struckOut{};

    friend bool operator==(const TextStyle&, const TextStyle&) = default;
};

struct TextBox {
    static constexpr float kNarrowest = 24.0F;
    static constexpr float kDefaultWidth = 320.0F;

    Uuid id;
    Point at;
    float width{kDefaultWidth};
    float height{};
    std::string text;
    TextStyle style;

    friend bool operator==(const TextBox&, const TextBox&) = default;
};

struct PlacedText {
    std::int64_t ordinal{};
    TextBox box;

    friend bool operator==(const PlacedText&, const PlacedText&) = default;
};

[[nodiscard]] constexpr Rect areaOf(const TextBox& box) noexcept {
    return {
        .left = box.at.x,
        .top = box.at.y,
        .right = box.at.x + box.width,
        .bottom = box.at.y + box.height,
    };
}

[[nodiscard]] TextStyle normalized(TextStyle style) noexcept;

[[nodiscard]] TextBox normalized(TextBox box) noexcept;

}
