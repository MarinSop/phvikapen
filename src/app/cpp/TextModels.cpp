#include "app/cpp/TextModels.hpp"

#include "core/model/Color.hpp"
#include "core/model/TextBox.hpp"

#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QModelIndex>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <cstddef>
#include <utility>
#include <vector>

namespace phvikapen::app {
namespace {

[[nodiscard]] core::TextAlign alignOf(int value) {
    return value >= 0 && value <= static_cast<int>(core::TextAlign::Justify)
               ? static_cast<core::TextAlign>(value)
               : core::TextAlign::Left;
}

}

TextListModel::TextListModel(QObject* parent) : QAbstractListModel(parent) {}

int TextListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant TextListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const TextItem& item = m_items[static_cast<std::size_t>(index.row())];
    switch (role) {
    case Qt::DisplayRole:
    case kTextRole:
        return item.text;
    case kTextIdRole:
        return item.textId;
    case kPageIdRole:
        return item.pageId;
    case kFontRole:
        return item.font;
    case kColorRole:
        return item.color;
    case kColumnXRole:
        return item.columnX;
    case kColumnYRole:
        return item.columnY;
    case kWidthRole:
        return item.width;
    case kHeightRole:
        return item.height;
    case kSizeRole:
        return item.size;
    case kLineHeightRole:
        return item.lineHeight;
    case kAlignRole:
        return item.align;
    case kSheetRole:
        return item.sheet;
    case kBoldRole:
        return item.bold;
    case kItalicRole:
        return item.italic;
    case kUnderlineRole:
        return item.underline;
    case kStruckOutRole:
        return item.struckOut;
    default:
        return {};
    }
}

QHash<int, QByteArray> TextListModel::roleNames() const {
    return {
        {kTextIdRole, "textId"},
        {kPageIdRole, "pageId"},
        {kTextRole, "text"},
        {kFontRole, "font"},
        {kColorRole, "color"},
        {kColumnXRole, "columnX"},
        {kColumnYRole, "columnY"},
        {kWidthRole, "boxWidth"},
        {kHeightRole, "boxHeight"},
        {kSizeRole, "size"},
        {kLineHeightRole, "lineHeight"},
        {kAlignRole, "align"},
        {kSheetRole, "sheet"},
        {kBoldRole, "bold"},
        {kItalicRole, "italic"},
        {kUnderlineRole, "underline"},
        {kStruckOutRole, "struckOut"},
    };
}

void TextListModel::setItems(std::vector<TextItem> items) {
    if (items == m_items) {
        return;
    }
    // While the same boxes are shown, only what changed is reported, so that a box being typed in
    // is not thrown away and made again with every letter.
    if (items.size() == m_items.size()) {
        const std::vector<TextItem> before = std::exchange(m_items, std::move(items));
        const bool sameBoxes = [&] {
            for (std::size_t row = 0; row < m_items.size(); ++row) {
                if (before[row].textId != m_items[row].textId) {
                    return false;
                }
            }
            return true;
        }();
        if (sameBoxes) {
            for (std::size_t row = 0; row < m_items.size(); ++row) {
                if (before[row] != m_items[row]) {
                    const QModelIndex at = index(static_cast<int>(row), 0);
                    emit dataChanged(at, at);
                }
            }
            return;
        }
        beginResetModel();
        endResetModel();
        return;
    }
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
    emit countChanged();
}

QVariantMap mapOfStyle(const core::TextStyle& style) {
    const core::Color colour = style.color;
    return QVariantMap{
        {"font", QString::fromStdString(style.font)},
        {"size", static_cast<qreal>(style.size)},
        {"color", QColor::fromRgb(colour.red, colour.green, colour.blue, colour.alpha)},
        {"align", static_cast<int>(style.align)},
        {"lineHeight", static_cast<qreal>(style.lineHeight)},
        {"bold", style.bold},
        {"italic", style.italic},
        {"underline", style.underline},
        {"struckOut", style.struckOut},
    };
}

core::TextStyle styleOfMap(const QVariantMap& style) {
    const core::TextStyle fallback;
    const auto colour =
        style
            .value("color", QColor::fromRgb(fallback.color.red, fallback.color.green,
                                            fallback.color.blue, fallback.color.alpha))
            .value<QColor>();
    return core::normalized(core::TextStyle{
        .font = style.value("font").toString().toStdString(),
        .size = style.value("size", static_cast<qreal>(fallback.size)).toFloat(),
        .color =
            core::Color{
                .red = static_cast<std::uint8_t>(colour.red()),
                .green = static_cast<std::uint8_t>(colour.green()),
                .blue = static_cast<std::uint8_t>(colour.blue()),
                .alpha = static_cast<std::uint8_t>(colour.alpha()),
            },
        .align = alignOf(style.value("align", 0).toInt()),
        .lineHeight = style.value("lineHeight", static_cast<qreal>(fallback.lineHeight)).toFloat(),
        .bold = style.value("bold", false).toBool(),
        .italic = style.value("italic", false).toBool(),
        .underline = style.value("underline", false).toBool(),
        .struckOut = style.value("struckOut", false).toBool(),
    });
}

}
