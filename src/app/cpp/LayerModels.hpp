#pragma once

#include "core/model/Layer.hpp"

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QString>
#include <QVariant>
#include <QtQmlIntegration>

#include <vector>

namespace phvikapen::app {

// One layer as the panel shows it. The list runs top first, the way a panel of layers is read,
// which is the other way round from the order they are drawn in.
struct LayerItem {
    QString layerId;
    QString name;
    bool shown{true};
    bool locked{false};
    int count{};

    friend bool operator==(const LayerItem&, const LayerItem&) = default;
};

class LayerListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by NotebookViewModel")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    static constexpr int kLayerIdRole = Qt::UserRole + 1;
    static constexpr int kNameRole = Qt::UserRole + 2;
    static constexpr int kShownRole = Qt::UserRole + 3;
    static constexpr int kLockedRole = Qt::UserRole + 4;
    static constexpr int kCountRole = Qt::UserRole + 5;

    explicit LayerListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setItems(std::vector<LayerItem> items);

signals:
    void countChanged();

private:
    std::vector<LayerItem> m_items;
};

[[nodiscard]] LayerItem itemOfLayer(const core::Layer& layer, int count);

}
