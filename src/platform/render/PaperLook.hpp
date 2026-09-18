#pragma once

#include "core/model/PageStyle.hpp"

#include <array>

namespace phvikapen::platform::render {

using Rgba = std::array<float, 4>;

inline constexpr float kRuleWidth = 1.0F;
inline constexpr float kDotRadius = 1.25F;
inline constexpr float kLinedTopMargin = core::millimeters(20.0F);
inline constexpr float kLinedLeftMargin = core::millimeters(25.0F);

inline constexpr Rgba kDeskColor{0.89F, 0.90F, 0.91F, 1.0F};
inline constexpr Rgba kPaperColor{1.0F, 1.0F, 1.0F, 1.0F};
inline constexpr Rgba kRuleColor{0.70F, 0.80F, 0.91F, 1.0F};
inline constexpr Rgba kGridColor{0.82F, 0.86F, 0.91F, 1.0F};
inline constexpr Rgba kDotColor{0.60F, 0.65F, 0.72F, 1.0F};
inline constexpr Rgba kMarginColor{0.93F, 0.60F, 0.60F, 1.0F};

[[nodiscard]] constexpr Rgba patternColor(core::Background background) noexcept {
    switch (background) {
    case core::Background::Grid:
        return kGridColor;
    case core::Background::Dotted:
        return kDotColor;
    case core::Background::Blank:
    case core::Background::Lined:
        break;
    }
    return kRuleColor;
}

}
