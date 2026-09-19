#include "platform/render/PagePainter.hpp"

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeOutline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "platform/render/PaperLook.hpp"

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRectF>

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace phvikapen::platform::render {
namespace {

constexpr float kInkMargin = core::millimeters(10.0F);
constexpr float kMaximumRules = 4096.0F;

[[nodiscard]] QColor toColor(const Rgba& color) {
    return QColor::fromRgbF(color[0], color[1], color[2], color[3]);
}

[[nodiscard]] QColor toColor(const core::Color& color) {
    return QColor{color.red, color.green, color.blue, color.alpha};
}

[[nodiscard]] QRectF toRect(const core::Rect& rect) {
    return QRectF{rect.left, rect.top, rect.width(), rect.height()};
}

[[nodiscard]] std::optional<core::Rect> paperRect(const core::PageStyle& style) {
    const std::optional<core::PaperSize> paper = core::paperSize(style);
    if (!paper) {
        return std::nullopt;
    }
    return core::Rect{
        .left = 0.0F,
        .top = 0.0F,
        .right = paper->width,
        .bottom = paper->height,
    };
}

struct Rules {
    int first{};
    int last{};
    float spacing{};

    [[nodiscard]] float at(int index) const { return static_cast<float>(index) * spacing; }
};

[[nodiscard]] Rules rulesBetween(float from, float to, float spacing) {
    return Rules{
        .first = static_cast<int>(std::ceil(from / spacing)),
        .last = static_cast<int>(std::floor(to / spacing)),
        .spacing = spacing,
    };
}

[[nodiscard]] bool tooManyRules(const core::Rect& area, float spacing) {
    return spacing <= 0.0F || std::max(area.width(), area.height()) / spacing > kMaximumRules;
}

void paintRuling(QPainter& painter, const core::PageStyle& style, const core::Rect& area,
                 bool hasPaper) {
    if (style.background == core::Background::Blank || tooManyRules(area, style.spacing)) {
        return;
    }

    const QColor color = toColor(lineColorOf(style));
    const float width = lineWidthOf(style);
    const bool lined = style.background == core::Background::Lined;
    const float firstRow = lined && hasPaper ? std::max(area.top, kLinedTopMargin) : area.top;
    const Rules rows = rulesBetween(firstRow, area.bottom, style.spacing);
    const Rules columns = rulesBetween(area.left, area.right, style.spacing);

    if (style.background == core::Background::Dotted) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        for (int row = rows.first; row <= rows.last; ++row) {
            for (int column = columns.first; column <= columns.last; ++column) {
                painter.drawEllipse(QPointF{columns.at(column), rows.at(row)}, width * kDotsPerRule,
                                    width * kDotsPerRule);
            }
        }
        painter.setBrush(Qt::NoBrush);
        return;
    }

    painter.setPen(QPen{color, width});
    for (int row = rows.first; row <= rows.last; ++row) {
        painter.drawLine(QPointF{area.left, rows.at(row)}, QPointF{area.right, rows.at(row)});
    }
    if (style.background == core::Background::Grid) {
        for (int column = columns.first; column <= columns.last; ++column) {
            painter.drawLine(QPointF{columns.at(column), area.top},
                             QPointF{columns.at(column), area.bottom});
        }
        return;
    }
    if (lined && hasPaper && style.margin) {
        painter.setPen(QPen{toColor(marginColorOf(style)), width});
        painter.drawLine(QPointF{style.marginAt, area.top}, QPointF{style.marginAt, area.bottom});
    }
}

void paintStrokes(QPainter& painter, std::span<const core::PlacedStroke> strokes) {
    painter.setPen(Qt::NoPen);
    for (const core::PlacedStroke& placed : strokes) {
        const core::Stroke& stroke = placed.stroke;
        const std::vector<core::Point> outline = core::strokeOutline(stroke);
        if (outline.empty()) {
            continue;
        }
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        path.moveTo(outline.front().x, outline.front().y);
        for (const core::Point& point : outline) {
            path.lineTo(point.x, point.y);
        }
        path.closeSubpath();
        painter.fillPath(path, toColor(stroke.style().color));
    }
}

}

core::Rect pageArea(const PageContents& page) {
    if (const std::optional<core::Rect> sheet = paperRect(page.style)) {
        return *sheet;
    }

    std::optional<core::Rect> ink;
    for (const core::PlacedStroke& placed : page.strokes) {
        if (const std::optional<core::Rect> bounds = placed.stroke.boundingBox()) {
            ink = ink ? ink->united(*bounds) : *bounds;
        }
    }
    if (!ink) {
        return paperRect(core::PageStyle{}).value_or(core::Rect{});
    }
    return ink->inflated(kInkMargin);
}

void paintPage(QPainter& painter, const PageContents& page, const core::Rect& area) {
    const std::optional<core::Rect> paper = paperRect(page.style);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.translate(-area.left, -area.top);
    painter.fillRect(toRect(area), toColor(paperColorOf(page.style)));

    paintRuling(painter, page.style, area, paper.has_value());

    if (page.media != nullptr && !page.media->isNull()) {
        const QRectF target = paper ? toRect(*paper) : toRect(area);
        painter.drawImage(target, *page.media);
    }

    paintStrokes(painter, page.strokes);
    painter.restore();
}

}
