#include "app/cpp/Shortcuts.hpp"

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QKeySequence>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

#include <cstddef>
#include <span>
#include <vector>

namespace phvikapen::app {
namespace {

// Every command whose key can be changed, with the key it comes with. Built on the first ask,
// because the names are translated and the strings allocate. A command whose keys differ from one
// platform to the next names the standard it follows instead of spelling the keys out.
[[nodiscard]] const std::vector<Command>& table() {
    static const std::vector<Command> kCommands{
        Command{
            .id = "newNotebook",
            .name = QObject::tr("New notebook"),
            .group = CommandGroup::File,
            .keys = "Ctrl+N",
            .standard = QKeySequence::New,
        },
        Command{
            .id = "closeNotebook",
            .name = QObject::tr("Close notebook"),
            .group = CommandGroup::File,
            .keys = "Ctrl+W",
            .standard = QKeySequence::Close,
        },
        Command{
            .id = "save",
            .name = QObject::tr("Save"),
            .group = CommandGroup::File,
            .keys = "Ctrl+S",
            .standard = QKeySequence::Save,
        },
        Command{
            .id = "saveCopy",
            .name = QObject::tr("Save as"),
            .group = CommandGroup::File,
            .keys = "Ctrl+Shift+S",
            .standard = QKeySequence::SaveAs,
        },
        Command{
            .id = "exportPdf",
            .name = QObject::tr("Export as PDF"),
            .group = CommandGroup::File,
            .keys = "Ctrl+E",
        },
        Command{
            .id = "pageSetup",
            .name = QObject::tr("Page setup"),
            .group = CommandGroup::File,
            .keys = "Ctrl+Shift+U",
        },
        Command{
            .id = "settings",
            .name = QObject::tr("Settings"),
            .group = CommandGroup::File,
            .keys = "Ctrl+,",
            .standard = QKeySequence::Preferences,
        },
        Command{
            .id = "undo",
            .name = QObject::tr("Undo"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Z",
            .standard = QKeySequence::Undo,
        },
        Command{
            .id = "redo",
            .name = QObject::tr("Redo"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Shift+Z",
            .standard = QKeySequence::Redo,
        },
        Command{
            .id = "cut",
            .name = QObject::tr("Cut"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+X",
            .standard = QKeySequence::Cut,
        },
        Command{
            .id = "copy",
            .name = QObject::tr("Copy"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+C",
            .standard = QKeySequence::Copy,
        },
        Command{
            .id = "paste",
            .name = QObject::tr("Paste"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+V",
            .standard = QKeySequence::Paste,
        },
        Command{
            .id = "delete",
            .name = QObject::tr("Delete"),
            .group = CommandGroup::Edit,
            .keys = "Del",
        },
        Command{
            .id = "duplicate",
            .name = QObject::tr("Duplicate"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Shift+D",
        },
        Command{
            .id = "selectAll",
            .name = QObject::tr("Select everything on the page"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+A",
            .standard = QKeySequence::SelectAll,
        },
        Command{
            .id = "copyAsText",
            .name = QObject::tr("Copy as text"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Shift+C",
        },
        Command{
            .id = "convertToText",
            .name = QObject::tr("Convert to text"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Shift+R",
        },
        Command{
            .id = "clearPage",
            .name = QObject::tr("Clear page"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Shift+Del",
        },
        Command{
            .id = "find",
            .name = QObject::tr("Find in the notebook"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+F",
            .standard = QKeySequence::Find,
        },
        Command{
            .id = "trash",
            .name = QObject::tr("Deleted pages"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Shift+T",
        },
        Command{
            .id = "rotateLeft",
            .name = QObject::tr("Turn left"),
            .group = CommandGroup::Arrange,
            .keys = "Ctrl+[",
        },
        Command{
            .id = "rotateRight",
            .name = QObject::tr("Turn right"),
            .group = CommandGroup::Arrange,
            .keys = "Ctrl+]",
        },
        Command{
            .id = "resetShape",
            .name = QObject::tr("Undo turning and sizing"),
            .group = CommandGroup::Arrange,
            .keys = "Ctrl+Shift+0",
        },
        Command{
            .id = "zoomIn",
            .name = QObject::tr("Zoom in"),
            .group = CommandGroup::View,
            .keys = "Ctrl++",
            .standard = QKeySequence::ZoomIn,
        },
        Command{
            .id = "zoomOut",
            .name = QObject::tr("Zoom out"),
            .group = CommandGroup::View,
            .keys = "Ctrl+-",
            .standard = QKeySequence::ZoomOut,
        },
        Command{
            .id = "fitPage",
            .name = QObject::tr("Fit page"),
            .group = CommandGroup::View,
            .keys = "Ctrl+0",
        },
        Command{
            .id = "previousPage",
            .name = QObject::tr("Previous page"),
            .group = CommandGroup::View,
            .keys = "PgUp",
            .standard = QKeySequence::MoveToPreviousPage,
        },
        Command{
            .id = "nextPage",
            .name = QObject::tr("Next page"),
            .group = CommandGroup::View,
            .keys = "PgDown",
            .standard = QKeySequence::MoveToNextPage,
        },
        Command{
            .id = "continuousPages",
            .name = QObject::tr("Pages one below the other"),
            .group = CommandGroup::View,
            .keys = "Ctrl+Shift+B",
        },
        Command{
            .id = "sectionsList",
            .name = QObject::tr("Sections list"),
            .group = CommandGroup::View,
            .keys = "Ctrl+1",
        },
        Command{
            .id = "pagesList",
            .name = QObject::tr("Pages list"),
            .group = CommandGroup::View,
            .keys = "Ctrl+2",
        },
        Command{
            .id = "pagePanel",
            .name = QObject::tr("Page setup panel"),
            .group = CommandGroup::View,
            .keys = "Ctrl+3",
        },
        Command{
            .id = "addPage",
            .name = QObject::tr("New page"),
            .group = CommandGroup::Insert,
            .keys = "Ctrl+Shift+P",
        },
        Command{
            .id = "addSection",
            .name = QObject::tr("New section"),
            .group = CommandGroup::Insert,
            .keys = "Ctrl+Shift+N",
        },
        Command{
            .id = "duplicatePage",
            .name = QObject::tr("Duplicate page"),
            .group = CommandGroup::Insert,
            .keys = "Ctrl+D",
        },
        Command{
            .id = "import",
            .name = QObject::tr("Import a PDF or picture"),
            .group = CommandGroup::Insert,
            .keys = "Ctrl+I",
        },
        Command{
            .id = "selectTool",
            .name = QObject::tr("Pick tool"),
            .group = CommandGroup::Tools,
            .keys = "V",
        },
        Command{
            .id = "handTool",
            .name = QObject::tr("Hand tool"),
            .group = CommandGroup::Tools,
            .keys = "H",
        },
        Command{
            .id = "penTool",
            .name = QObject::tr("Pen tool"),
            .group = CommandGroup::Tools,
            .keys = "P",
        },
        Command{
            .id = "highlighterTool",
            .name = QObject::tr("Highlighter tool"),
            .group = CommandGroup::Tools,
            .keys = "M",
        },
        Command{
            .id = "shapeTool",
            .name = QObject::tr("Shape tool"),
            .group = CommandGroup::Tools,
            .keys = "U",
        },
        Command{
            .id = "eraserTool",
            .name = QObject::tr("Eraser tool"),
            .group = CommandGroup::Tools,
            .keys = "E",
        },
        Command{
            .id = "eraserMode",
            .name = QObject::tr("Switch what the eraser takes"),
            .group = CommandGroup::Tools,
            .keys = "Shift+E",
        },
        Command{
            .id = "colourTool",
            .name = QObject::tr("Color picker tool"),
            .group = CommandGroup::Tools,
            .keys = "K",
        },
        Command{
            .id = "textTool",
            .name = QObject::tr("Text tool"),
            .group = CommandGroup::Tools,
            .keys = "T",
        },
        Command{
            .id = "hints",
            .name = QObject::tr("Keys and hints"),
            .group = CommandGroup::Help,
            .keys = "F1",
        },
    };
    return kCommands;
}

[[nodiscard]] const QHash<QString, const Command*>& byId() {
    static const QHash<QString, const Command*> kIndex = [] {
        QHash<QString, const Command*> built;
        for (const Command& command : table()) {
            built.insert(command.id, &command);
        }
        return built;
    }();
    return kIndex;
}

[[nodiscard]] QString keysOf(const Command& command) {
    if (command.standard != QKeySequence::UnknownKey) {
        const QList<QKeySequence> bindings = QKeySequence::keyBindings(command.standard);
        if (!bindings.isEmpty()) {
            return bindings.first().toString(QKeySequence::PortableText);
        }
    }
    return command.keys;
}

}

std::span<const Command> commands() {
    return table();
}

const Command* commandOf(const QString& commandId) {
    return byId().value(commandId, nullptr);
}

QString defaultKeysOf(const QString& commandId) {
    const Command* const command = commandOf(commandId);
    return command == nullptr ? QString{} : keysOf(*command);
}

QString nameOfGroup(CommandGroup group) {
    switch (group) {
    case CommandGroup::File:
        return QObject::tr("File");
    case CommandGroup::Edit:
        return QObject::tr("Edit");
    case CommandGroup::Arrange:
        return QObject::tr("Arrange");
    case CommandGroup::View:
        return QObject::tr("View");
    case CommandGroup::Insert:
        return QObject::tr("Insert");
    case CommandGroup::Tools:
        return QObject::tr("Tools");
    case CommandGroup::Help:
        return QObject::tr("Help");
    }
    return {};
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
    const QString fallback = defaultKeysOf(command.id);
    switch (role) {
    case kIdRole:
        return command.id;
    case Qt::DisplayRole:
    case kNameRole:
        return command.name;
    case kSequenceRole:
        return m_sequences.value(command.id, fallback);
    case kChangedRole:
        return m_sequences.value(command.id, fallback).toString() != fallback;
    case kGroupRole:
        return nameOfGroup(command.group);
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
        {kGroupRole, QByteArrayLiteral("group")},
    };
}

QStringList ShortcutListModel::ids() {
    QStringList names;
    names.reserve(static_cast<qsizetype>(commands().size()));
    for (const Command& command : commands()) {
        names.append(command.id);
    }
    return names;
}

void ShortcutListModel::setSequences(const QVariantMap& sequences) {
    m_sequences = sequences;
    if (rowCount() > 0) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0), {kSequenceRole, kChangedRole});
    }
}

}
