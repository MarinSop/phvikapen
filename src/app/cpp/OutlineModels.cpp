#include "app/cpp/OutlineModels.hpp"

#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QVariant>

#include <cstddef>
#include <utility>
#include <vector>

namespace phvikapen::app {

OutlineListModel::OutlineListModel(QObject* parent) : QAbstractListModel(parent) {}

int OutlineListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant OutlineListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const OutlineItem& item = m_items[static_cast<std::size_t>(index.row())];
    switch (role) {
    case Qt::DisplayRole:
    case kTitleRole:
        return item.title;
    case kCountRole:
        return item.count;
    default:
        return {};
    }
}

QHash<int, QByteArray> OutlineListModel::roleNames() const {
    return {
        {kTitleRole, "title"},
        {kCountRole, "count"},
    };
}

void OutlineListModel::setItems(std::vector<OutlineItem> items) {
    if (items == m_items) {
        return;
    }
    const bool countChanges = items.size() != m_items.size();
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
    if (countChanges) {
        emit countChanged();
    }
}

}
