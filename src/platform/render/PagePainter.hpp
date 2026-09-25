#pragma once

#include "core/geometry/Rect.hpp"
#include "core/math/Drawing.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/model/Picture.hpp"
#include "core/model/Table.hpp"
#include "core/model/TextBox.hpp"

#include <span>
#include <string>

class QImage;
class QPainter;

namespace phvikapen::platform::render {

// A picture standing on a page, with what it is made of already decoded.
struct DrawnPicture {
    core::Picture placed;
    const QImage* picture{nullptr};
};

struct PageContents {
    core::PageStyle style{};
    std::span<const core::PlacedStroke> strokes;
    std::span<const core::PlacedText> texts;
    std::span<const DrawnPicture> pictures;
    std::span<const core::PlacedTable> tables;
    const QImage* media{nullptr};
};

// A sum laid out to be drawn, measured with the window's own reckoning of how wide type runs.
// Nothing comes back where what is typed is not arithmetic, and it is then shown as plain words.
[[nodiscard]] core::Drawing drawnFormula(const std::string& said, const core::TextStyle& style);

[[nodiscard]] core::Rect pageArea(const PageContents& page);

void paintPage(QPainter& painter, const PageContents& page, const core::Rect& area);

}
