#include "app/cpp/LayerModels.hpp"

#include "core/model/Layer.hpp"

#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QString>
#include <QVariant>

#include <cstddef>
#include <utility>
#include <vector>

namespace phvikapen::app {

LayerListModel::LayerListModel(QObject* parent) : QAbstractListModel(parent) {}

int LayerListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant LayerListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const LayerItem& item = m_items[static_cast<std::size_t>(index.row())];
    switch (role) {
    case Qt::DisplayRole:
    case kNameRole:
        return item.name;
    case kLayerIdRole:
        return item.layerId;
    case kShownRole:
        return item.shown;
    case kLockedRole:
        return item.locked;
    case kCountRole:
        return item.count;
    default:
        return {};
    }
}

QHash<int, QByteArray> LayerListModel::roleNames() const {
    return {
        {kLayerIdRole, "layerId"}, {kNameRole, "name"},   {kShownRole, "shown"},
        {kLockedRole, "locked"},   {kCountRole, "count"},
    };
}

void LayerListModel::setItems(std::vector<LayerItem> items) {
    if (items == m_items) {
        return;
    }
    const bool sameCount = items.size() == m_items.size();
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
    if (!sameCount) {
        emit countChanged();
    }
}

LayerItem itemOfLayer(const core::Layer& layer, int count) {
    return LayerItem{
        .layerId = QString::fromStdString(layer.id.toString()),
        .name = QString::fromStdString(layer.name),
        .shown = layer.shown,
        .locked = layer.locked,
        .count = count,
    };
}

}
