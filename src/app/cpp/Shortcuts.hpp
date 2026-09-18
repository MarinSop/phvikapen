#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QtQmlIntegration>

#include <span>
#include <vector>

namespace phvikapen::app {

struct Command {
    QString id;
    QString name;
    QString fallback;
};

[[nodiscard]] std::span<const Command> commands();

class ShortcutListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by SettingsViewModel")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    static constexpr int kIdRole = Qt::UserRole + 1;
    static constexpr int kNameRole = Qt::UserRole + 2;
    static constexpr int kSequenceRole = Qt::UserRole + 3;
    static constexpr int kChangedRole = Qt::UserRole + 4;

    explicit ShortcutListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setSequences(const QVariantMap& sequences);

signals:
    void countChanged();

private:
    QVariantMap m_sequences;
};

}
