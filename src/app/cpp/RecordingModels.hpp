#pragma once

#include "core/model/Recording.hpp"

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

namespace said_state {
Q_NAMESPACE
QML_NAMED_ELEMENT(SaidState)

// How far the reading of a recording has got, as the window shows it.
enum class Reading : quint8 {
    Unasked,
    Asked,
    Read,
    Failed,
};
Q_ENUM_NS(Reading)
}

// One recording as the panel shows it.
struct RecordingItem {
    QString recordingId;
    QString name;
    qint64 length{};
    qint64 madeAt{};
    int marks{};
    said_state::Reading reading{said_state::Reading::Unasked};
    QString trouble;
    int sayings{};

    friend bool operator==(const RecordingItem&, const RecordingItem&) = default;
};

class RecordingListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by NotebookViewModel")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    static constexpr int kRecordingIdRole = Qt::UserRole + 1;
    static constexpr int kNameRole = Qt::UserRole + 2;
    static constexpr int kLengthRole = Qt::UserRole + 3;
    static constexpr int kMadeAtRole = Qt::UserRole + 4;
    static constexpr int kMarksRole = Qt::UserRole + 5;
    static constexpr int kReadingRole = Qt::UserRole + 6;
    static constexpr int kTroubleRole = Qt::UserRole + 7;
    static constexpr int kSayingsRole = Qt::UserRole + 8;

    explicit RecordingListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setItems(std::vector<RecordingItem> items);

signals:
    void countChanged();

private:
    std::vector<RecordingItem> m_items;
};

// What was said in one recording, run by run, as the panel shows it.
struct SayingItem {
    qint64 from{};
    qint64 to{};
    QString text;

    friend bool operator==(const SayingItem&, const SayingItem&) = default;
};

class SayingListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by NotebookViewModel")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    static constexpr int kFromRole = Qt::UserRole + 1;
    static constexpr int kToRole = Qt::UserRole + 2;
    static constexpr int kTextRole = Qt::UserRole + 3;

    explicit SayingListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setItems(std::vector<SayingItem> items);

signals:
    void countChanged();

private:
    std::vector<SayingItem> m_items;
};

[[nodiscard]] said_state::Reading readingOf(core::Reading reading) noexcept;

[[nodiscard]] RecordingItem itemOfRecording(const core::Recording& recording, int marks);

}
