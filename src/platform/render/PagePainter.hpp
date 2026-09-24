#pragma once

#include "core/geometry/Rect.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/model/TextBox.hpp"

#include <span>

class QImage;
class QPainter;

namespace phvikapen::platform::render {

struct PageContents {
    core::PageStyle style{};
    std::span<const core::PlacedStroke> strokes;
    std::span<const core::PlacedText> texts;
    const QImage* media{nullptr};
};

[[nodiscard]] core::Rect pageArea(const PageContents& page);

void paintPage(QPainter& painter, const PageContents& page, const core::Rect& area);

}
