#include "core/model/TextBox.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace phvikapen::core {

TextStyle normalized(TextStyle style) noexcept {
    if (!std::isfinite(style.size)) {
        style.size = TextStyle::kDefaultSize;
    }
    style.size = std::clamp(style.size, TextStyle::kSmallestSize, TextStyle::kLargestSize);
    if (!std::isfinite(style.lineHeight)) {
        style.lineHeight = TextStyle::kDefaultLineHeight;
    }
    style.lineHeight =
        std::clamp(style.lineHeight, TextStyle::kTightestLines, TextStyle::kLoosestLines);
    return style;
}

TextBox normalized(TextBox box) noexcept {
    if (!std::isfinite(box.at.x) || !std::isfinite(box.at.y)) {
        box.at = Point{};
    }
    if (!std::isfinite(box.width)) {
        box.width = TextBox::kDefaultWidth;
    }
    box.width = std::max(box.width, TextBox::kNarrowest);
    if (!std::isfinite(box.height) || box.height < 0.0F) {
        box.height = 0.0F;
    }
    box.style = normalized(std::move(box.style));
    return box;
}

}
