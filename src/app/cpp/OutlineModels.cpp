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
    case kThumbnailRole:
        return item.thumbnail;
    default:
        return {};
    }
}

QHash<int, QByteArray> OutlineListModel::roleNames() const {
    return {
        {kTitleRole, "title"},
        {kCountRole, "count"},
        {kThumbnailRole, "thumbnail"},
    };
}

void OutlineListModel::setItems(std::vector<OutlineItem> items) {
    if (items == m_items) {
        return;
    }
    // As long as the rows are the same ones, only what changed is reported: a reset would throw
    // away every row, and a small picture arriving is no reason to build the list again.
    if (items.size() == m_items.size()) {
        const std::vector<OutlineItem> before = std::exchange(m_items, std::move(items));
        for (std::size_t row = 0; row < m_items.size(); ++row) {
            if (before[row] != m_items[row]) {
                const QModelIndex at = index(static_cast<int>(row), 0);
                emit dataChanged(at, at);
            }
        }
        return;
    }
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
    emit countChanged();
}

}

namespace phvikapen::app {

TrashListModel::TrashListModel(QObject* parent) : QAbstractListModel(parent) {}

int TrashListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant TrashListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0
        || std::cmp_greater_equal(index.row(), m_items.size())) {
        return {};
    }
    const TrashItem& item = m_items[static_cast<std::size_t>(index.row())];
    switch (role) {
    case kTitleRole:
    case Qt::DisplayRole:
        return item.title;
    case kSectionRole:
        return item.wholeSection;
    case kRestorableRole:
        return item.restorable;
    default:
        return {};
    }
}

QHash<int, QByteArray> TrashListModel::roleNames() const {
    return {
        {kTitleRole, QByteArrayLiteral("title")},
        {kSectionRole, QByteArrayLiteral("wholeSection")},
        {kRestorableRole, QByteArrayLiteral("restorable")},
    };
}

void TrashListModel::setItems(std::vector<TrashItem> items) {
    if (items == m_items) {
        return;
    }
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
    emit countChanged();
}

}
