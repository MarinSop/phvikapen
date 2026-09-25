#include "platform/render/PagePainter.hpp"

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeOutline.hpp"
#include "core/math/Drawing.hpp"
#include "core/math/Equation.hpp"
#include "core/math/Reading.hpp"
#include "core/model/Layer.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/model/Table.hpp"
#include "core/model/TextBox.hpp"
#include "platform/render/PaperLook.hpp"

#include <QAbstractTextDocumentLayout>
#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QTextDocument>
#include <QTextOption>

#include <algorithm>
#include <cmath>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace phvikapen::platform::render {
namespace {

constexpr float kInkMargin = core::millimeters(10.0F);
constexpr float kMaximumRules = 4096.0F;
constexpr float kCellPadding = 3.0F;

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

[[nodiscard]] QFont fontOf(const core::TextStyle& style) {
    QFont font;
    if (!style.font.empty()) {
        font.setFamily(QString::fromStdString(style.font));
    }
    font.setPixelSize(
        std::max(1, static_cast<int>(std::lround(core::pageUnitsOfPoints(style.size)))));
    font.setBold(style.bold);
    font.setItalic(style.italic);
    font.setUnderline(style.underline);
    font.setStrikeOut(style.struckOut);
    return font;
}

[[nodiscard]] Qt::Alignment alignmentOf(core::TextAlign align) {
    switch (align) {
    case core::TextAlign::Center:
        return Qt::AlignHCenter;
    case core::TextAlign::Right:
        return Qt::AlignRight;
    case core::TextAlign::Justify:
        return Qt::AlignJustify;
    case core::TextAlign::Left:
    default:
        return Qt::AlignLeft;
    }
}

// Type is laid out the same way the window lays it out: the same font, the same width to run in
// and no margin of its own, so that what is printed is what was seen.
void layOutWords(QTextDocument& document, const core::TextStyle& style, core::TextAlign align,
                 const std::string& words, float width) {
    document.setDocumentMargin(0.0);
    document.setDefaultFont(fontOf(style));
    QTextOption option;
    option.setAlignment(alignmentOf(align));
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    document.setDefaultTextOption(option);
    document.setPlainText(QString::fromStdString(words));
    document.setTextWidth(width);
}

void drawWords(QPainter& painter, const QTextDocument& document, const core::Color& color) {
    QAbstractTextDocumentLayout::PaintContext context;
    context.palette.setColor(QPalette::Text, toColor(color));
    document.documentLayout()->draw(&painter, context);
}

// A sum is drawn piece by piece where it was laid out, with the bars of its fractions and the
// roofs of its roots filled in as lines rather than written as letters.
void paintDrawing(QPainter& painter, const core::Drawing& drawing, const core::TextStyle& style,
                  const core::Point& at) {
    painter.save();
    painter.translate(at.x, at.y);
    const QColor ink = toColor(style.color);
    for (const core::Bar& bar : drawing.bars) {
        painter.fillRect(toRect(bar.area), ink);
    }
    painter.setPen(QPen{ink});
    for (const core::Glyph& glyph : drawing.glyphs) {
        core::TextStyle set = style;
        set.size = glyph.size;
        const QFont font = fontOf(set);
        painter.setFont(font);
        const QFontMetricsF metrics{font};
        const auto line =
            static_cast<double>(core::pageUnitsOfPoints(glyph.size) * core::kLineRoom);
        const double baseline = glyph.at.y + ((line + metrics.ascent() - metrics.descent()) / 2.0);
        painter.drawText(QPointF{glyph.at.x, baseline}, QString::fromStdString(glyph.text));
    }
    painter.restore();
}

// Which layer is being drawn just now. A page with no layers draws everything in one pass, which
// is what every page did before there were layers.
struct LayerPass {
    std::span<const core::Layer> layers;
    core::Uuid only;
    bool everything{false};

    [[nodiscard]] bool holds(const core::Uuid& stands) const {
        if (everything) {
            return true;
        }
        const core::Layer* const found = core::layerOf(layers, stands);
        return found != nullptr && found->id == only;
    }
};

void paintTexts(QPainter& painter, std::span<const core::PlacedText> texts, const LayerPass& pass) {
    for (const core::PlacedText& placed : texts) {
        const core::TextBox& box = placed.box;
        if (box.text.empty() || !pass.holds(placed.layer)) {
            continue;
        }
        if (box.formula) {
            const core::Drawing drawn = drawnFormula(box.text, box.style);
            if (!drawn.glyphs.empty()) {
                paintDrawing(painter, drawn, box.style, box.at);
                continue;
            }
        }
        QTextDocument document;
        layOutWords(document, box.style, box.style.align, box.text, box.width);

        painter.save();
        painter.translate(box.at.x, box.at.y);
        drawWords(painter, document, box.style.color);
        painter.restore();
    }
}

// What stands behind the boxes that were given a colour of their own, which is what a heading row
// is made of. It goes down before the ruling, so that the rules are drawn over it rather than half
// covered by the box beside them.
void paintTableFills(QPainter& painter, const core::Table& table) {
    for (int row = 0; row < core::rowsOf(table); ++row) {
        for (int column = 0; column < core::columnsOf(table); ++column) {
            const core::CellAt at{.row = row, .column = column};
            const core::TableCell* const cell = core::cellAt(table, at);
            if (cell == nullptr || core::isCovered(*cell) || !core::isShown(cell->fill)) {
                continue;
            }
            painter.fillRect(toRect(core::areaOfCell(table, at)), toColor(cell->fill));
        }
    }
}

// Every box is ruled round on its own rather than the whole grid being drawn line by line, so that
// a box reaching over others has no ruling running through the middle of it.
void paintTableRuling(QPainter& painter, const core::Table& table) {
    painter.setPen(QPen{toColor(table.rule), table.ruleWidth});
    painter.setBrush(Qt::NoBrush);
    for (int row = 0; row < core::rowsOf(table); ++row) {
        for (int column = 0; column < core::columnsOf(table); ++column) {
            const core::Rect box =
                core::areaOfCell(table, core::CellAt{.row = row, .column = column});
            if (box.width() <= 0.0F || box.height() <= 0.0F) {
                continue;
            }
            painter.drawRect(toRect(box));
        }
    }
}

// What is typed in a box runs within that box and no further, so that too many words are cut off
// at the ruling rather than written over what stands beside them.
// The face a box is shown in: what the whole table is shown in, with whatever the box asks of its
// own put on top of it.
[[nodiscard]] core::TextStyle faceOfCell(const core::Table& table, const core::TableCell& cell) {
    core::TextStyle set = table.style;
    set.bold = set.bold || cell.bold;
    set.italic = set.italic || cell.italic;
    if (core::isShown(cell.ink)) {
        set.color = cell.ink;
    }
    return set;
}

constexpr double kHalfway = 0.5;

// How far down its room what is typed in a box begins, so that it may stand at the top, at the
// middle or at the foot.
[[nodiscard]] double riseOf(core::CellRise rise, double room, double tall) {
    const double spare = std::max(room - tall, 0.0);
    switch (rise) {
    case core::CellRise::Middle:
        return spare * kHalfway;
    case core::CellRise::Bottom:
        return spare;
    case core::CellRise::Top:
    default:
        return 0.0;
    }
}

void paintCell(QPainter& painter, const core::Table& table, const core::TableCell& cell,
               const core::Rect& box) {
    const float room = std::max(box.width() - (2.0F * kCellPadding), 1.0F);
    const core::TextStyle face = faceOfCell(table, cell);
    QTextDocument document;
    layOutWords(document, face, cell.align, cell.text, room);

    painter.save();
    painter.setClipRect(toRect(box));
    const double down = riseOf(cell.rise, static_cast<double>(box.height()) - (2.0 * kCellPadding),
                               document.size().height());
    painter.translate(box.left + kCellPadding, box.top + kCellPadding + down);
    drawWords(painter, document, face.color);
    painter.restore();
}

// Tables stand over the ink, as typed text does, but only their ruling is drawn: what is written
// by hand inside a box is seen through it, so a table can be ruled first and filled in by hand.
void paintTables(QPainter& painter, std::span<const core::PlacedTable> tables,
                 const LayerPass& pass) {
    for (const core::PlacedTable& placed : tables) {
        if (!pass.holds(placed.layer)) {
            continue;
        }
        const core::Table& table = placed.table;
        paintTableFills(painter, table);
        paintTableRuling(painter, table);
        for (int row = 0; row < core::rowsOf(table); ++row) {
            for (int column = 0; column < core::columnsOf(table); ++column) {
                const core::CellAt at{.row = row, .column = column};
                const core::TableCell* const cell = core::cellAt(table, at);
                if (cell != nullptr && !core::isCovered(*cell) && !cell->text.empty()) {
                    paintCell(painter, table, *cell, core::areaOfCell(table, at));
                }
            }
        }
    }
}

void paintStrokes(QPainter& painter, std::span<const core::PlacedStroke> strokes,
                  const LayerPass& pass) {
    painter.setPen(Qt::NoPen);
    for (const core::PlacedStroke& placed : strokes) {
        if (!pass.holds(placed.layer)) {
            continue;
        }
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

core::Drawing drawnFormula(const std::string& said, const core::TextStyle& style) {
    const core::Result<core::Equation> equation = core::equationOf(core::tidied(said));
    if (!equation) {
        return {};
    }
    const core::Measure measure = [&style](std::string_view piece, float size) {
        core::TextStyle set = style;
        set.size = size;
        const QFontMetricsF metrics{fontOf(set)};
        return static_cast<float>(metrics.horizontalAdvance(
            QString::fromUtf8(piece.data(), static_cast<qsizetype>(piece.size()))));
    };
    return core::laidOut(*equation, style.size, measure);
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

// Pictures stand over the document a page was made from and under everything written on it, each
// turned about its own middle.
void paintPictures(QPainter& painter, std::span<const DrawnPicture> pictures,
                   const LayerPass& pass) {
    for (const DrawnPicture& drawn : pictures) {
        if (drawn.picture == nullptr || drawn.picture->isNull() || !pass.holds(drawn.layer)) {
            continue;
        }
        const core::Rect where = core::areaOf(drawn.placed);
        const QRectF target = toRect(where);
        painter.save();
        painter.translate(target.center());
        painter.rotate(static_cast<qreal>(drawn.placed.turn));
        painter.translate(-target.center());
        painter.drawImage(target, *drawn.picture);
        painter.restore();
    }
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

    // Layer by layer from the bottom up. Within one layer a picture stands under the ink written
    // over it, and a table or a box of type over both, which is the order a page has always been
    // drawn in; between layers, everything on a higher one stands over everything on a lower one.
    const auto drawOne = [&](const LayerPass& pass) {
        paintPictures(painter, page.pictures, pass);
        paintStrokes(painter, page.strokes, pass);
        paintTables(painter, page.tables, pass);
        paintTexts(painter, page.texts, pass);
    };
    if (page.layers.empty()) {
        drawOne(LayerPass{.layers = page.layers, .only = {}, .everything = true});
    } else {
        for (const core::Layer& layer : page.layers) {
            if (layer.shown) {
                drawOne(LayerPass{.layers = page.layers, .only = layer.id, .everything = false});
            }
        }
    }
    painter.restore();
}

}
