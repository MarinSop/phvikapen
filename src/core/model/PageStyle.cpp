#include "core/model/PageStyle.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace phvikapen::core {
namespace {

[[nodiscard]] constexpr PaperSize inches(float width, float height) noexcept {
    return {.width = width * kPageUnitsPerInch, .height = height * kPageUnitsPerInch};
}

[[nodiscard]] constexpr PaperSize metric(float width, float height) noexcept {
    return {.width = millimeters(width), .height = millimeters(height)};
}

constexpr PaperSize kA3 = metric(297.0F, 420.0F);
constexpr PaperSize kA4 = metric(210.0F, 297.0F);
constexpr PaperSize kA5 = metric(148.0F, 210.0F);
constexpr PaperSize kLetter = inches(8.5F, 11.0F);
constexpr PaperSize kLegal = inches(8.5F, 14.0F);

[[nodiscard]] std::optional<PaperSize> portraitSize(Paper paper) noexcept {
    switch (paper) {
    case Paper::Infinite:
        return std::nullopt;
    case Paper::A3:
        return kA3;
    case Paper::A4:
        return kA4;
    case Paper::A5:
        return kA5;
    case Paper::Letter:
        return kLetter;
    case Paper::Legal:
        return kLegal;
    }
    return std::nullopt;
}

}

std::optional<PaperSize> paperSize(Paper paper, Orientation orientation) noexcept {
    std::optional<PaperSize> size = portraitSize(paper);
    if (size && orientation == Orientation::Landscape) {
        std::swap(size->width, size->height);
    }
    return size;
}

PageStyle normalized(PageStyle style) noexcept {
    if (!std::isfinite(style.spacing)) {
        style.spacing = PageStyle::kDefaultSpacing;
    }
    style.spacing =
        std::clamp(style.spacing, PageStyle::kMinimumSpacing, PageStyle::kMaximumSpacing);
    return style;
}

}
