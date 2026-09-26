#include "app/cpp/Shortcuts.hpp"

#include <QAbstractListModel>
#include <QByteArray>
#include <QCoreApplication>
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

// Every command whose key can be changed, with the key it comes with. Built on the first ask, and
// kept, because nothing in it depends on the language: a name is read out of it and translated when
// it is shown. A command whose keys differ from one platform to the next names the standard it
// follows instead of spelling the keys out.
[[nodiscard]] const std::vector<Command>& table() {
    static const std::vector<Command> kCommands{
        Command{
            .id = "newNotebook",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "New notebook"),
            .group = CommandGroup::File,
            .keys = "Ctrl+N",
            .standard = QKeySequence::New,
        },
        Command{
            .id = "closeNotebook",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Close notebook"),
            .group = CommandGroup::File,
            .keys = "Ctrl+W",
            .standard = QKeySequence::Close,
        },
        Command{
            .id = "save",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Save"),
            .group = CommandGroup::File,
            .keys = "Ctrl+S",
            .standard = QKeySequence::Save,
        },
        Command{
            .id = "saveCopy",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Save as"),
            .group = CommandGroup::File,
            .keys = "Ctrl+Shift+S",
            .standard = QKeySequence::SaveAs,
        },
        Command{
            .id = "exportPdf",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Export as PDF"),
            .group = CommandGroup::File,
            .keys = "Ctrl+E",
        },
        Command{
            .id = "pageSetup",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Page setup"),
            .group = CommandGroup::File,
            .keys = "Ctrl+Shift+U",
        },
        Command{
            .id = "settings",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Settings"),
            .group = CommandGroup::File,
            .keys = "Ctrl+,",
            .standard = QKeySequence::Preferences,
        },
        Command{
            .id = "undo",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Undo"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Z",
            .standard = QKeySequence::Undo,
        },
        Command{
            .id = "redo",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Redo"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Shift+Z",
            .standard = QKeySequence::Redo,
        },
        Command{
            .id = "cut",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Cut"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+X",
            .standard = QKeySequence::Cut,
        },
        Command{
            .id = "copy",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Copy"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+C",
            .standard = QKeySequence::Copy,
        },
        Command{
            .id = "paste",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Paste"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+V",
            .standard = QKeySequence::Paste,
        },
        Command{
            .id = "delete",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Delete"),
            .group = CommandGroup::Edit,
            .keys = "Del",
        },
        Command{
            .id = "duplicate",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Duplicate"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Shift+D",
        },
        Command{
            .id = "selectAll",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Select everything on the page"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+A",
            .standard = QKeySequence::SelectAll,
        },
        Command{
            .id = "copyAsText",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Copy as text"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Shift+C",
        },
        Command{
            .id = "convertToText",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Convert to text"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Shift+R",
        },
        Command{
            .id = "solve",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Work out what was written"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Shift+A",
        },
        Command{
            .id = "clearPage",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Clear page"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Shift+Del",
        },
        Command{
            .id = "find",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Find in the notebook"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+F",
            .standard = QKeySequence::Find,
        },
        Command{
            .id = "trash",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Deleted pages"),
            .group = CommandGroup::Edit,
            .keys = "Ctrl+Shift+T",
        },
        Command{
            .id = "rotateLeft",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Turn left"),
            .group = CommandGroup::Arrange,
            .keys = "Ctrl+[",
        },
        Command{
            .id = "rotateRight",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Turn right"),
            .group = CommandGroup::Arrange,
            .keys = "Ctrl+]",
        },
        Command{
            .id = "resetShape",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Undo turning and sizing"),
            .group = CommandGroup::Arrange,
            .keys = "Ctrl+Shift+0",
        },
        Command{
            .id = "layerUp",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Move layer up"),
            .group = CommandGroup::Arrange,
            .keys = "Ctrl+Shift+]",
        },
        Command{
            .id = "layerDown",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Move layer down"),
            .group = CommandGroup::Arrange,
            .keys = "Ctrl+Shift+[",
        },
        Command{
            .id = "showLayers",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Layers panel"),
            .group = CommandGroup::View,
            .keys = "Ctrl+Shift+L",
        },
        Command{
            .id = "zoomIn",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Zoom in"),
            .group = CommandGroup::View,
            .keys = "Ctrl++",
            .standard = QKeySequence::ZoomIn,
        },
        Command{
            .id = "zoomOut",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Zoom out"),
            .group = CommandGroup::View,
            .keys = "Ctrl+-",
            .standard = QKeySequence::ZoomOut,
        },
        Command{
            .id = "fitPage",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Fit page"),
            .group = CommandGroup::View,
            .keys = "Ctrl+0",
        },
        Command{
            .id = "previousPage",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Previous page"),
            .group = CommandGroup::View,
            .keys = "PgUp",
            .standard = QKeySequence::MoveToPreviousPage,
        },
        Command{
            .id = "nextPage",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Next page"),
            .group = CommandGroup::View,
            .keys = "PgDown",
            .standard = QKeySequence::MoveToNextPage,
        },
        Command{
            .id = "continuousPages",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Pages one below the other"),
            .group = CommandGroup::View,
            .keys = "Ctrl+Shift+B",
        },
        Command{
            .id = "sectionsList",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Sections list"),
            .group = CommandGroup::View,
            .keys = "Ctrl+1",
        },
        Command{
            .id = "pagesList",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Pages list"),
            .group = CommandGroup::View,
            .keys = "Ctrl+2",
        },
        Command{
            .id = "mathsPanel",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Maths panel"),
            .group = CommandGroup::View,
            .keys = "Ctrl+Shift+M",
        },
        Command{
            .id = "elementsPanel",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Elements panel"),
            .group = CommandGroup::View,
            .keys = "Ctrl+6",
        },
        Command{
            .id = "soundPanel",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Recordings panel"),
            .group = CommandGroup::View,
            .keys = "Ctrl+5",
        },
        Command{
            .id = "timePanel",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Time panel"),
            .group = CommandGroup::View,
            .keys = "Ctrl+4",
        },
        Command{
            .id = "pagePanel",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Page setup panel"),
            .group = CommandGroup::View,
            .keys = "Ctrl+3",
        },
        Command{
            .id = "addPage",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "New page"),
            .group = CommandGroup::Insert,
            .keys = "Ctrl+Shift+P",
        },
        Command{
            .id = "addSection",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "New section"),
            .group = CommandGroup::Insert,
            .keys = "Ctrl+Shift+N",
        },
        Command{
            .id = "duplicatePage",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Duplicate page"),
            .group = CommandGroup::Insert,
            .keys = "Ctrl+D",
        },
        Command{
            .id = "insertPicture",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Picture on the page"),
            .group = CommandGroup::Insert,
            .keys = "Ctrl+Shift+I",
        },
        Command{
            .id = "insertTable",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Table on the page"),
            .group = CommandGroup::Insert,
            .keys = "Ctrl+Shift+G",
        },
        Command{
            .id = "insertEquation",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Sum on the page"),
            .group = CommandGroup::Insert,
            .keys = "Ctrl+Shift+E",
        },
        Command{
            .id = "import",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Import a PDF or picture"),
            .group = CommandGroup::Insert,
            .keys = "Ctrl+I",
        },
        Command{
            .id = "selectTool",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Pick tool"),
            .group = CommandGroup::Tools,
            .keys = "V",
        },
        Command{
            .id = "handTool",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Hand tool"),
            .group = CommandGroup::Tools,
            .keys = "H",
        },
        Command{
            .id = "penTool",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Pen tool"),
            .group = CommandGroup::Tools,
            .keys = "P",
        },
        Command{
            .id = "highlighterTool",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Highlighter tool"),
            .group = CommandGroup::Tools,
            .keys = "M",
        },
        Command{
            .id = "shapeTool",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Shape tool"),
            .group = CommandGroup::Tools,
            .keys = "U",
        },
        Command{
            .id = "eraserTool",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Eraser tool"),
            .group = CommandGroup::Tools,
            .keys = "E",
        },
        Command{
            .id = "eraserMode",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Switch what the eraser takes"),
            .group = CommandGroup::Tools,
            .keys = "Shift+E",
        },
        Command{
            .id = "colourTool",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Color picker tool"),
            .group = CommandGroup::Tools,
            .keys = "K",
        },
        Command{
            .id = "textTool",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Text tool"),
            .group = CommandGroup::Tools,
            .keys = "T",
        },
        Command{
            .id = "hints",
            .name = QT_TRANSLATE_NOOP("Shortcuts", "Keys and hints"),
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

QString nameOf(const Command& command) {
    return QCoreApplication::translate("Shortcuts", command.name);
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
        return nameOf(command);
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
