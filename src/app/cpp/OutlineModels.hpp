#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QString>
#include <QVariant>
#include <QtQmlIntegration>
#include <QtTypes>

#include <vector>

namespace phvikapen::app {

namespace page_options {
Q_NAMESPACE
QML_NAMED_ELEMENT(PageOptions)

enum class Paper : quint8 {
    Infinite,
    A3,
    A4,
    A5,
    Letter,
    Legal,
    Custom,
};
Q_ENUM_NS(Paper)

enum class Orientation : quint8 {
    Portrait,
    Landscape,
};
Q_ENUM_NS(Orientation)

enum class Background : quint8 {
    Blank,
    Lined,
    Grid,
    Dotted,
};
Q_ENUM_NS(Background)
}

struct OutlineItem {
    QString title;
    int count{0};

    friend bool operator==(const OutlineItem&, const OutlineItem&) = default;
};

struct TrashItem {
    QString title;
    bool wholeSection{};
    bool restorable{};

    friend bool operator==(const TrashItem&, const TrashItem&) = default;
};

class TrashListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by NotebookViewModel")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    static constexpr int kTitleRole = Qt::UserRole + 1;
    static constexpr int kSectionRole = Qt::UserRole + 2;
    static constexpr int kRestorableRole = Qt::UserRole + 3;

    explicit TrashListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setItems(std::vector<TrashItem> items);

signals:
    void countChanged();

private:
    std::vector<TrashItem> m_items;
};

class OutlineListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by NotebookViewModel")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    static constexpr int kTitleRole = Qt::UserRole + 1;
    static constexpr int kCountRole = Qt::UserRole + 2;

    explicit OutlineListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setItems(std::vector<OutlineItem> items);

signals:
    void countChanged();

private:
    std::vector<OutlineItem> m_items;
};

}
