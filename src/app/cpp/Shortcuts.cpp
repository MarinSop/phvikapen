#include "app/cpp/Shortcuts.hpp"

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <cstddef>
#include <span>
#include <utility>
#include <vector>

namespace phvikapen::app {
namespace {

// Every command whose key can be changed, with the key it comes with. Built on the first ask,
// because the names are translated and the strings allocate.
[[nodiscard]] const std::vector<Command>& table() {
    static const std::vector<Command> kCommands{
        Command{.id = "undo", .name = QObject::tr("Undo"), .fallback = "Ctrl+Z"},
        Command{.id = "redo", .name = QObject::tr("Redo"), .fallback = "Ctrl+Shift+Z"},
        Command{.id = "copy", .name = QObject::tr("Copy"), .fallback = "Ctrl+C"},
        Command{.id = "paste", .name = QObject::tr("Paste"), .fallback = "Ctrl+V"},
        Command{.id = "delete", .name = QObject::tr("Delete"), .fallback = "Del"},
        Command{.id = "newNotebook", .name = QObject::tr("New notebook"), .fallback = "Ctrl+N"},
        Command{.id = "closeNotebook", .name = QObject::tr("Close notebook"), .fallback = "Ctrl+W"},
        Command{.id = "saveCopy", .name = QObject::tr("Save a copy"), .fallback = "Ctrl+Shift+S"},
        Command{.id = "exportPdf", .name = QObject::tr("Export as PDF"), .fallback = "Ctrl+E"},
        Command{
            .id = "import",
            .name = QObject::tr("Import a PDF or picture"),
            .fallback = "Ctrl+I",
        },
        Command{.id = "settings", .name = QObject::tr("Settings"), .fallback = "Ctrl+,"},
        Command{.id = "zoomIn", .name = QObject::tr("Zoom in"), .fallback = "Ctrl++"},
        Command{.id = "zoomOut", .name = QObject::tr("Zoom out"), .fallback = "Ctrl+-"},
        Command{.id = "fitPage", .name = QObject::tr("Fit page"), .fallback = "Ctrl+0"},
        Command{.id = "previousPage", .name = QObject::tr("Previous page"), .fallback = "PgUp"},
        Command{.id = "nextPage", .name = QObject::tr("Next page"), .fallback = "PgDown"},
        Command{.id = "addPage", .name = QObject::tr("New page"), .fallback = "Ctrl+Shift+P"},
        Command{.id = "selectTool", .name = QObject::tr("Pick tool"), .fallback = "V"},
        Command{.id = "handTool", .name = QObject::tr("Hand tool"), .fallback = "H"},
        Command{.id = "penTool", .name = QObject::tr("Pen tool"), .fallback = "P"},
    };
    return kCommands;
}

}

std::span<const Command> commands() {
    return table();
}

ShortcutListModel::ShortcutListModel(QObject* parent) : QAbstractListModel(parent) {}

int ShortcutListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(commands().size());
}

QVariant ShortcutListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const Command& command = commands()[static_cast<std::size_t>(index.row())];
    switch (role) {
    case kIdRole:
        return command.id;
    case Qt::DisplayRole:
    case kNameRole:
        return command.name;
    case kSequenceRole:
        return m_sequences.value(command.id, command.fallback);
    case kChangedRole:
        return m_sequences.value(command.id, command.fallback).toString() != command.fallback;
    default:
        return {};
    }
}

QHash<int, QByteArray> ShortcutListModel::roleNames() const {
    return {
        {kIdRole, QByteArrayLiteral("commandId")},
        {kNameRole, QByteArrayLiteral("name")},
        {kSequenceRole, QByteArrayLiteral("sequence")},
        {kChangedRole, QByteArrayLiteral("changed")},
    };
}

void ShortcutListModel::setSequences(const QVariantMap& sequences) {
    m_sequences = sequences;
    if (rowCount() > 0) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0), {kSequenceRole, kChangedRole});
    }
}

}
