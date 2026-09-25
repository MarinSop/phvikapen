#pragma once

#include "core/model/Table.hpp"

#include <QAbstractListModel>
#include <QByteArray>
#include <QColor>
#include <QHash>
#include <QModelIndex>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QtQmlIntegration>
#include <QtTypes>

#include <vector>

namespace phvikapen::app {

// A table as the window shows it: where it stands in the column of sheets everything is drawn in,
// how wide its columns run, how tall its rows stand, and what is typed in its boxes, kept row by
// row so that the box at a row and a column is the one at row * columns + column.
struct TableItem {
    QString tableId;
    QString pageId;
    QVariantList widths;
    QVariantList heights;
    QVariantList words;
    QVariantList aligns;
    QVariantList acrosses;
    QVariantList downs;
    QVariantList fills;
    QVariantList inks;
    QVariantList bolds;
    QVariantList italics;
    QVariantList rises;
    QVariantMap style;
    QColor rule;
    qreal columnX{};
    qreal columnY{};
    qreal ruleWidth{};
    int sheet{};

    friend bool operator==(const TableItem&, const TableItem&) = default;
};

class TableListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by NotebookViewModel")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    static constexpr int kTableIdRole = Qt::UserRole + 1;
    static constexpr int kPageIdRole = Qt::UserRole + 2;
    static constexpr int kWidthsRole = Qt::UserRole + 3;
    static constexpr int kHeightsRole = Qt::UserRole + 4;
    static constexpr int kWordsRole = Qt::UserRole + 5;
    static constexpr int kAlignsRole = Qt::UserRole + 6;
    static constexpr int kStyleRole = Qt::UserRole + 7;
    static constexpr int kRuleRole = Qt::UserRole + 8;
    static constexpr int kColumnXRole = Qt::UserRole + 9;
    static constexpr int kColumnYRole = Qt::UserRole + 10;
    static constexpr int kRuleWidthRole = Qt::UserRole + 11;
    static constexpr int kSheetRole = Qt::UserRole + 12;
    static constexpr int kAcrossesRole = Qt::UserRole + 13;
    static constexpr int kDownsRole = Qt::UserRole + 14;
    static constexpr int kFillsRole = Qt::UserRole + 15;
    static constexpr int kInksRole = Qt::UserRole + 16;
    static constexpr int kBoldsRole = Qt::UserRole + 17;
    static constexpr int kItalicsRole = Qt::UserRole + 18;
    static constexpr int kRisesRole = Qt::UserRole + 19;

    explicit TableListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setItems(std::vector<TableItem> items);

signals:
    void countChanged();

private:
    std::vector<TableItem> m_items;
};

// Where a table stands in the column of sheets, told apart from the table itself so that one
// shape serves both what is listed and what is taken hold of.
struct TablePlace {
    QString pageId;
    qreal columnX{};
    qreal columnY{};
    int sheet{};
};

[[nodiscard]] TableItem itemOfTable(const core::Table& table, const TablePlace& place);

// The same table as QML hands one about, for the one being worked on.
[[nodiscard]] QVariantMap mapOfItem(const TableItem& item);

}
