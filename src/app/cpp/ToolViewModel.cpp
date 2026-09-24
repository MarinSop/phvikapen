#include "app/cpp/ToolViewModel.hpp"

#include "app/cpp/TextModels.hpp"
#include "core/model/TextBox.hpp"

#include <QColor>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
#include <array>
#include <cstddef>

namespace phvikapen::app {
namespace {

constexpr auto kToolSetting = "tools/tool";
constexpr auto kPenSetting = "tools/pen";
constexpr auto kEraserSetting = "tools/eraser";
constexpr auto kShapeSetting = "tools/shape";
constexpr auto kCornerSetting = "tools/corner";
constexpr auto kTextFontSetting = "tools/text/font";
constexpr auto kTextSizeSetting = "tools/text/size";
constexpr auto kTextColorSetting = "tools/text/color";
constexpr auto kTextAlignSetting = "tools/text/align";
constexpr auto kTextBoldSetting = "tools/text/bold";
constexpr auto kTextItalicSetting = "tools/text/italic";
constexpr auto kTextUnderlineSetting = "tools/text/underline";
constexpr auto kTextStruckSetting = "tools/text/struck";
constexpr auto kColorPrefix = "tools/color/";
constexpr auto kWidthPrefix = "tools/width/";
constexpr auto kHighlighterKey = "highlighter";

constexpr int kHighlighterAlpha = 90;
constexpr int kOpaqueAlpha = 255;
constexpr qreal kHighlighterWidth = 14.0;
constexpr qreal kFirstPenWidth = 2.0;
constexpr qreal kSecondPenWidth = 3.0;
constexpr qreal kThirdPenWidth = 1.5;

constexpr std::array kPaletteColors{
    "#1a1a1a", "#2f5bea", "#d13438", "#107c10", "#f7a80d", "#8661c5", "#8a5a2b", "#6b6b6b",
};

[[nodiscard]] QString penKey(int index) {
    return QString::number(index);
}

}

ToolViewModel::ToolViewModel(QObject* parent)
    : QObject(parent),
      m_pens{
          Nib{.color = QColor{"#1a1a1a"}, .width = kFirstPenWidth},
          Nib{.color = QColor{"#2f5bea"}, .width = kSecondPenWidth},
          Nib{.color = QColor{"#d13438"}, .width = kThirdPenWidth},
      },
      m_highlighter{.color = QColor{"#f7a80d"}, .width = kHighlighterWidth} {
    m_highlighter.color.setAlpha(kHighlighterAlpha);
}

void ToolViewModel::componentComplete() {
    restore();
    m_completed = true;
}

ToolViewModel::Nib& ToolViewModel::activeNib() {
    if (m_currentTool == Tool::Highlighter) {
        return m_highlighter;
    }
    return m_pens.at(static_cast<std::size_t>(m_pen));
}

const ToolViewModel::Nib& ToolViewModel::activeNib() const {
    if (m_currentTool == Tool::Highlighter) {
        return m_highlighter;
    }
    return m_pens.at(static_cast<std::size_t>(m_pen));
}

void ToolViewModel::setCurrentTool(Tool tool) {
    if (tool == m_currentTool) {
        return;
    }
    if (tool == Tool::ColourPicker) {
        m_beforePicking = m_currentTool;
    }
    m_currentTool = tool;
    emit currentToolChanged();
    emit toolChanged();
    remember();
}

void ToolViewModel::setPen(int index) {
    if (index < 0 || index >= penCount() || index == m_pen) {
        return;
    }
    m_pen = index;
    emit penChanged();
    if (m_currentTool != Tool::Highlighter) {
        emit toolChanged();
    }
    remember();
}

QColor ToolViewModel::strokeColor() const {
    return activeNib().color;
}

void ToolViewModel::setStrokeColor(const QColor& color) {
    Nib& nib = activeNib();
    QColor wanted = color;
    if (m_currentTool == Tool::Highlighter) {
        wanted.setAlpha(kHighlighterAlpha);
    }
    if (wanted == nib.color) {
        return;
    }
    nib.color = wanted;
    emit toolChanged();
    remember();
}

qreal ToolViewModel::strokeWidth() const {
    return activeNib().width;
}

void ToolViewModel::setStrokeWidth(qreal width) {
    Nib& nib = activeNib();
    const qreal wanted = std::clamp(width, kMinimumWidth, kMaximumWidth);
    if (qFuzzyCompare(wanted, nib.width)) {
        return;
    }
    nib.width = wanted;
    emit toolChanged();
    remember();
}

bool ToolViewModel::pressureSensitive() const {
    return m_currentTool != Tool::Highlighter;
}

void ToolViewModel::setEraserRadius(qreal radius) {
    const qreal wanted = std::clamp(radius, kMinimumEraser, kMaximumEraser);
    if (qFuzzyCompare(wanted, m_eraserRadius)) {
        return;
    }
    m_eraserRadius = wanted;
    emit eraserChanged();
    remember();
}

QVariantList ToolViewModel::palette() {
    QVariantList colors;
    colors.reserve(static_cast<qsizetype>(kPaletteColors.size()));
    for (const auto* name : kPaletteColors) {
        colors.append(QColor{name});
    }
    return colors;
}

void ToolViewModel::usePickedColour(const QColor& colour) {
    if (!colour.isValid()) {
        return;
    }
    QColor opaque = colour;
    opaque.setAlpha(kOpaqueAlpha);
    m_pens.at(static_cast<std::size_t>(m_pen)).color = opaque;
    QColor marker = opaque;
    marker.setAlpha(kHighlighterAlpha);
    m_highlighter.color = marker;
    setCurrentTool(m_beforePicking == Tool::ColourPicker ? Tool::Pen : m_beforePicking);
    emit toolChanged();
    remember();
}

QString ToolViewModel::textFont() const {
    return QString::fromStdString(m_text.font);
}

void ToolViewModel::setTextFont(const QString& font) {
    const std::string wanted = font.toStdString();
    if (wanted == m_text.font) {
        return;
    }
    m_text.font = wanted;
    emit textChanged();
    remember();
}

qreal ToolViewModel::textSize() const {
    return m_text.size;
}

void ToolViewModel::setTextSize(qreal size) {
    const auto wanted =
        static_cast<float>(std::clamp(size, static_cast<qreal>(core::TextStyle::kSmallestSize),
                                      static_cast<qreal>(core::TextStyle::kLargestSize)));
    if (qFuzzyCompare(wanted, m_text.size)) {
        return;
    }
    m_text.size = wanted;
    emit textChanged();
    remember();
}

QColor ToolViewModel::textColor() const {
    return QColor::fromRgb(m_text.color.red, m_text.color.green, m_text.color.blue,
                           m_text.color.alpha);
}

void ToolViewModel::setTextColor(const QColor& color) {
    if (!color.isValid() || color == textColor()) {
        return;
    }
    m_text.color = core::Color{
        .red = static_cast<std::uint8_t>(color.red()),
        .green = static_cast<std::uint8_t>(color.green()),
        .blue = static_cast<std::uint8_t>(color.blue()),
        .alpha = static_cast<std::uint8_t>(color.alpha()),
    };
    emit textChanged();
    remember();
}

int ToolViewModel::textAlign() const {
    return static_cast<int>(m_text.align);
}

void ToolViewModel::setTextAlign(int align) {
    if (align < 0 || align > static_cast<int>(core::TextAlign::Justify) || align == textAlign()) {
        return;
    }
    m_text.align = static_cast<core::TextAlign>(align);
    emit textChanged();
    remember();
}

void ToolViewModel::setTextBold(bool bold) {
    if (bold == m_text.bold) {
        return;
    }
    m_text.bold = bold;
    emit textChanged();
    remember();
}

void ToolViewModel::setTextItalic(bool italic) {
    if (italic == m_text.italic) {
        return;
    }
    m_text.italic = italic;
    emit textChanged();
    remember();
}

void ToolViewModel::setTextUnderline(bool underline) {
    if (underline == m_text.underline) {
        return;
    }
    m_text.underline = underline;
    emit textChanged();
    remember();
}

void ToolViewModel::setTextStruckOut(bool struckOut) {
    if (struckOut == m_text.struckOut) {
        return;
    }
    m_text.struckOut = struckOut;
    emit textChanged();
    remember();
}

QVariantMap ToolViewModel::textStyle() const {
    return mapOfStyle(m_text);
}

void ToolViewModel::useTextStyle(const QVariantMap& style) {
    const core::TextStyle wanted = styleOfMap(style);
    if (wanted == m_text) {
        return;
    }
    m_text = wanted;
    emit textChanged();
    remember();
}

QVariantList ToolViewModel::penColors() const {
    QVariantList colors;
    colors.reserve(static_cast<qsizetype>(m_pens.size()));
    for (const Nib& nib : m_pens) {
        colors.append(nib.color);
    }
    return colors;
}

QVariantList ToolViewModel::penWidths() const {
    QVariantList widths;
    widths.reserve(static_cast<qsizetype>(m_pens.size()));
    for (const Nib& nib : m_pens) {
        widths.append(nib.width);
    }
    return widths;
}

QColor ToolViewModel::colorOfPen(int index) const {
    if (index < 0 || index >= penCount()) {
        return {};
    }
    return m_pens.at(static_cast<std::size_t>(index)).color;
}

qreal ToolViewModel::widthOfPen(int index) const {
    if (index < 0 || index >= penCount()) {
        return 0.0;
    }
    return m_pens.at(static_cast<std::size_t>(index)).width;
}

void ToolViewModel::setShape(Shape shape) {
    if (shape == m_shape) {
        return;
    }
    m_shape = shape;
    emit shapeChanged();
    remember();
}

void ToolViewModel::setCorner(qreal corner) {
    const qreal wanted = std::clamp(corner, 0.0, kMaximumCorner);
    if (qFuzzyCompare(wanted + 1.0, m_corner + 1.0)) {
        return;
    }
    m_corner = wanted;
    emit shapeChanged();
    remember();
}

void ToolViewModel::remember() const {
    if (!m_completed) {
        return;
    }
    QSettings settings;
    settings.setValue(kToolSetting, static_cast<int>(m_currentTool));
    settings.setValue(kPenSetting, m_pen);
    settings.setValue(kEraserSetting, m_eraserRadius);
    settings.setValue(kShapeSetting, static_cast<int>(m_shape));
    settings.setValue(kCornerSetting, m_corner);
    for (int index = 0; index < penCount(); ++index) {
        const Nib& nib = m_pens.at(static_cast<std::size_t>(index));
        settings.setValue(kColorPrefix + penKey(index), nib.color.name(QColor::HexArgb));
        settings.setValue(kWidthPrefix + penKey(index), nib.width);
    }
    settings.setValue(QString{kColorPrefix} + kHighlighterKey,
                      m_highlighter.color.name(QColor::HexArgb));
    settings.setValue(QString{kWidthPrefix} + kHighlighterKey, m_highlighter.width);
    settings.setValue(kTextFontSetting, textFont());
    settings.setValue(kTextSizeSetting, textSize());
    settings.setValue(kTextColorSetting, textColor().name(QColor::HexArgb));
    settings.setValue(kTextAlignSetting, textAlign());
    settings.setValue(kTextBoldSetting, m_text.bold);
    settings.setValue(kTextItalicSetting, m_text.italic);
    settings.setValue(kTextUnderlineSetting, m_text.underline);
    settings.setValue(kTextStruckSetting, m_text.struckOut);
}

void ToolViewModel::restore() {
    const QSettings settings;
    const auto readNib = [&settings](Nib& nib, const QString& key) {
        const QColor color{
            settings.value(kColorPrefix + key, nib.color.name(QColor::HexArgb)).toString()};
        if (color.isValid()) {
            nib.color = color;
        }
        nib.width = std::clamp(settings.value(kWidthPrefix + key, nib.width).toDouble(),
                               kMinimumWidth, kMaximumWidth);
    };
    for (int index = 0; index < penCount(); ++index) {
        readNib(m_pens.at(static_cast<std::size_t>(index)), penKey(index));
    }
    readNib(m_highlighter, kHighlighterKey);

    const int tool = settings.value(kToolSetting, static_cast<int>(m_currentTool)).toInt();
    if (tool >= static_cast<int>(Tool::Pen) && tool <= static_cast<int>(Tool::Text)) {
        m_currentTool = static_cast<Tool>(tool);
    }
    const int shape = settings.value(kShapeSetting, static_cast<int>(m_shape)).toInt();
    if (shape > static_cast<int>(Shape::Freehand) && shape <= static_cast<int>(Shape::Ellipse)) {
        m_shape = static_cast<Shape>(shape);
    }
    m_corner = std::clamp(settings.value(kCornerSetting, m_corner).toDouble(), 0.0, kMaximumCorner);
    m_pen = std::clamp(settings.value(kPenSetting, m_pen).toInt(), 0, penCount() - 1);
    m_eraserRadius = std::clamp(settings.value(kEraserSetting, m_eraserRadius).toDouble(),
                                kMinimumEraser, kMaximumEraser);

    m_text.font = settings.value(kTextFontSetting, textFont()).toString().toStdString();
    m_text.size =
        static_cast<float>(std::clamp(settings.value(kTextSizeSetting, textSize()).toDouble(),
                                      static_cast<qreal>(core::TextStyle::kSmallestSize),
                                      static_cast<qreal>(core::TextStyle::kLargestSize)));
    if (const QColor colour{
            settings.value(kTextColorSetting, textColor().name(QColor::HexArgb)).toString()};
        colour.isValid()) {
        setTextColor(colour);
    }
    const int align = settings.value(kTextAlignSetting, textAlign()).toInt();
    if (align >= 0 && align <= static_cast<int>(core::TextAlign::Justify)) {
        m_text.align = static_cast<core::TextAlign>(align);
    }
    m_text.bold = settings.value(kTextBoldSetting, m_text.bold).toBool();
    m_text.italic = settings.value(kTextItalicSetting, m_text.italic).toBool();
    m_text.underline = settings.value(kTextUnderlineSetting, m_text.underline).toBool();
    m_text.struckOut = settings.value(kTextStruckSetting, m_text.struckOut).toBool();

    emit currentToolChanged();
    emit shapeChanged();
    emit penChanged();
    emit toolChanged();
    emit textChanged();
    emit eraserChanged();
}

}
