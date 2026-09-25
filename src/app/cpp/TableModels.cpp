#include "app/cpp/TableModels.hpp"

#include "app/cpp/TextModels.hpp"
#include "core/model/Color.hpp"
#include "core/model/Table.hpp"

#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QModelIndex>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include <cstddef>
#include <utility>
#include <vector>

namespace phvikapen::app {

namespace {

// A colour the window can use. A box that asks for no colour of its own hands back one with nothing
// in it rather than no colour at all: a colour the window cannot read is shown as black, and a box
// that said nothing would be filled in with it.
[[nodiscard]] QColor paintOf(core::Color color) {
    return core::isShown(color) ? QColor::fromRgb(color.red, color.green, color.blue, color.alpha)
                                : QColor::fromRgb(0, 0, 0, 0);
}

}

TableListModel::TableListModel(QObject* parent) : QAbstractListModel(parent) {}

int TableListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant TableListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const TableItem& item = m_items[static_cast<std::size_t>(index.row())];
    switch (role) {
    case Qt::DisplayRole:
    case kTableIdRole:
        return item.tableId;
    case kPageIdRole:
        return item.pageId;
    case kWidthsRole:
        return item.widths;
    case kHeightsRole:
        return item.heights;
    case kWordsRole:
        return item.words;
    case kAlignsRole:
        return item.aligns;
    case kAcrossesRole:
        return item.acrosses;
    case kDownsRole:
        return item.downs;
    case kFillsRole:
        return item.fills;
    case kInksRole:
        return item.inks;
    case kBoldsRole:
        return item.bolds;
    case kItalicsRole:
        return item.italics;
    case kRisesRole:
        return item.rises;
    case kStyleRole:
        return item.style;
    case kRuleRole:
        return item.rule;
    case kColumnXRole:
        return item.columnX;
    case kColumnYRole:
        return item.columnY;
    case kRuleWidthRole:
        return item.ruleWidth;
    case kSheetRole:
        return item.sheet;
    default:
        return {};
    }
}

QHash<int, QByteArray> TableListModel::roleNames() const {
    return {
        {kTableIdRole, "tableId"},   {kPageIdRole, "pageId"},       {kWidthsRole, "widths"},
        {kHeightsRole, "heights"},   {kWordsRole, "words"},         {kAlignsRole, "aligns"},
        {kStyleRole, "style"},       {kRuleRole, "rule"},           {kColumnXRole, "columnX"},
        {kColumnYRole, "columnY"},   {kRuleWidthRole, "ruleWidth"}, {kSheetRole, "sheet"},
        {kAcrossesRole, "acrosses"}, {kDownsRole, "downs"},         {kFillsRole, "fills"},
        {kInksRole, "inks"},         {kBoldsRole, "bolds"},         {kItalicsRole, "italics"},
        {kRisesRole, "rises"},
    };
}

void TableListModel::setItems(std::vector<TableItem> items) {
    if (items == m_items) {
        return;
    }
    // While the same tables are shown, only what changed is reported, so that a box being typed in
    // is not thrown away and made again with every letter.
    if (items.size() == m_items.size()) {
        const std::vector<TableItem> before = std::exchange(m_items, std::move(items));
        const bool sameTables = [&] {
            for (std::size_t row = 0; row < m_items.size(); ++row) {
                if (before[row].tableId != m_items[row].tableId) {
                    return false;
                }
            }
            return true;
        }();
        if (sameTables) {
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

TableItem itemOfTable(const core::Table& table, const TablePlace& place) {
    QVariantList widths;
    widths.reserve(static_cast<qsizetype>(table.columns.size()));
    for (const float width : table.columns) {
        widths.append(static_cast<qreal>(width));
    }
    QVariantList heights;
    heights.reserve(static_cast<qsizetype>(table.rows.size()));
    for (const float height : table.rows) {
        heights.append(static_cast<qreal>(height));
    }
    QVariantList words;
    QVariantList aligns;
    QVariantList acrosses;
    QVariantList downs;
    QVariantList fills;
    QVariantList inks;
    QVariantList bolds;
    QVariantList italics;
    QVariantList rises;
    const auto room = static_cast<qsizetype>(table.cells.size());
    words.reserve(room);
    aligns.reserve(room);
    acrosses.reserve(room);
    downs.reserve(room);
    fills.reserve(room);
    inks.reserve(room);
    bolds.reserve(room);
    italics.reserve(room);
    rises.reserve(room);
    for (const core::TableCell& cell : table.cells) {
        words.append(QString::fromStdString(cell.text));
        aligns.append(static_cast<int>(cell.align));
        acrosses.append(cell.across);
        downs.append(cell.down);
        fills.append(paintOf(cell.fill));
        inks.append(paintOf(cell.ink));
        bolds.append(cell.bold);
        italics.append(cell.italic);
        rises.append(static_cast<int>(cell.rise));
    }
    const core::Color rule = table.rule;
    return TableItem{
        .tableId = QString::fromStdString(table.id.toString()),
        .pageId = place.pageId,
        .widths = std::move(widths),
        .heights = std::move(heights),
        .words = std::move(words),
        .aligns = std::move(aligns),
        .acrosses = std::move(acrosses),
        .downs = std::move(downs),
        .fills = std::move(fills),
        .inks = std::move(inks),
        .bolds = std::move(bolds),
        .italics = std::move(italics),
        .rises = std::move(rises),
        .style = mapOfStyle(table.style),
        .rule = QColor::fromRgb(rule.red, rule.green, rule.blue, rule.alpha),
        .columnX = place.columnX,
        .columnY = place.columnY,
        .ruleWidth = static_cast<qreal>(table.ruleWidth),
        .sheet = place.sheet,
    };
}

QVariantMap mapOfItem(const TableItem& item) {
    return QVariantMap{
        {"tableId", item.tableId},     {"pageId", item.pageId},   {"widths", item.widths},
        {"heights", item.heights},     {"words", item.words},     {"aligns", item.aligns},
        {"acrosses", item.acrosses},   {"downs", item.downs},     {"style", item.style},
        {"rule", item.rule},           {"columnX", item.columnX}, {"columnY", item.columnY},
        {"ruleWidth", item.ruleWidth}, {"sheet", item.sheet},     {"fills", item.fills},
        {"inks", item.inks},           {"bolds", item.bolds},     {"italics", item.italics},
        {"rises", item.rises},
    };
}

}
