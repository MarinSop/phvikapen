#include "app/cpp/RecordingModels.hpp"

#include "core/model/Recording.hpp"

#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QString>
#include <QVariant>

#include <cstddef>
#include <utility>
#include <vector>

namespace phvikapen::app {

RecordingListModel::RecordingListModel(QObject* parent) : QAbstractListModel(parent) {}

int RecordingListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant RecordingListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const RecordingItem& item = m_items[static_cast<std::size_t>(index.row())];
    switch (role) {
    case Qt::DisplayRole:
    case kNameRole:
        return item.name;
    case kRecordingIdRole:
        return item.recordingId;
    case kLengthRole:
        return item.length;
    case kMadeAtRole:
        return item.madeAt;
    case kMarksRole:
        return item.marks;
    case kReadingRole:
        return static_cast<int>(item.reading);
    case kTroubleRole:
        return item.trouble;
    case kSayingsRole:
        return item.sayings;
    default:
        return {};
    }
}

QHash<int, QByteArray> RecordingListModel::roleNames() const {
    return {
        {kRecordingIdRole, "recordingId"}, {kNameRole, "name"},       {kLengthRole, "length"},
        {kMadeAtRole, "madeAt"},           {kMarksRole, "marks"},     {kReadingRole, "reading"},
        {kTroubleRole, "trouble"},         {kSayingsRole, "sayings"},
    };
}

void RecordingListModel::setItems(std::vector<RecordingItem> items) {
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

SayingListModel::SayingListModel(QObject* parent) : QAbstractListModel(parent) {}

int SayingListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant SayingListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const SayingItem& item = m_items[static_cast<std::size_t>(index.row())];
    switch (role) {
    case Qt::DisplayRole:
    case kTextRole:
        return item.text;
    case kFromRole:
        return item.from;
    case kToRole:
        return item.to;
    default:
        return {};
    }
}

QHash<int, QByteArray> SayingListModel::roleNames() const {
    return {
        {kFromRole, "from"},
        {kToRole, "to"},
        {kTextRole, "text"},
    };
}

void SayingListModel::setItems(std::vector<SayingItem> items) {
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

said_state::Reading readingOf(core::Reading reading) noexcept {
    switch (reading) {
    case core::Reading::Asked:
        return said_state::Reading::Asked;
    case core::Reading::Read:
        return said_state::Reading::Read;
    case core::Reading::Failed:
        return said_state::Reading::Failed;
    default:
        return said_state::Reading::Unasked;
    }
}

RecordingItem itemOfRecording(const core::Recording& recording, int marks) {
    return RecordingItem{
        .recordingId = QString::fromStdString(recording.id.toString()),
        .name = QString::fromStdString(recording.name),
        .length = recording.length,
        .madeAt = recording.madeAt,
        .marks = marks,
        .reading = readingOf(recording.said.reading),
        .trouble = QString::fromStdString(recording.said.trouble),
        .sayings = static_cast<int>(recording.said.sayings.size()),
    };
}

}
