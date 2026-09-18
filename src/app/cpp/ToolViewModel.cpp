#include "app/cpp/ToolViewModel.hpp"

#include <QColor>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include <algorithm>
#include <array>
#include <cstddef>

namespace phvikapen::app {
namespace {

constexpr auto kToolSetting = "tools/tool";
constexpr auto kPenSetting = "tools/pen";
constexpr auto kEraserSetting = "tools/eraser";
constexpr auto kShapeSetting = "tools/shape";
constexpr auto kColorPrefix = "tools/color/";
constexpr auto kWidthPrefix = "tools/width/";
constexpr auto kHighlighterKey = "highlighter";

constexpr int kHighlighterAlpha = 90;
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

void ToolViewModel::remember() const {
    if (!m_completed) {
        return;
    }
    QSettings settings;
    settings.setValue(kToolSetting, static_cast<int>(m_currentTool));
    settings.setValue(kPenSetting, m_pen);
    settings.setValue(kEraserSetting, m_eraserRadius);
    settings.setValue(kShapeSetting, static_cast<int>(m_shape));
    for (int index = 0; index < penCount(); ++index) {
        const Nib& nib = m_pens.at(static_cast<std::size_t>(index));
        settings.setValue(kColorPrefix + penKey(index), nib.color.name(QColor::HexArgb));
        settings.setValue(kWidthPrefix + penKey(index), nib.width);
    }
    settings.setValue(QString{kColorPrefix} + kHighlighterKey,
                      m_highlighter.color.name(QColor::HexArgb));
    settings.setValue(QString{kWidthPrefix} + kHighlighterKey, m_highlighter.width);
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
    if (tool >= static_cast<int>(Tool::Pen) && tool <= static_cast<int>(Tool::Shape)) {
        m_currentTool = static_cast<Tool>(tool);
    }
    const int shape = settings.value(kShapeSetting, static_cast<int>(m_shape)).toInt();
    if (shape > static_cast<int>(Shape::Freehand) && shape <= static_cast<int>(Shape::Ellipse)) {
        m_shape = static_cast<Shape>(shape);
    }
    m_pen = std::clamp(settings.value(kPenSetting, m_pen).toInt(), 0, penCount() - 1);
    m_eraserRadius = std::clamp(settings.value(kEraserSetting, m_eraserRadius).toDouble(),
                                kMinimumEraser, kMaximumEraser);

    emit currentToolChanged();
    emit shapeChanged();
    emit penChanged();
    emit toolChanged();
    emit eraserChanged();
}

}
