#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QKeySequence>
#include <QModelIndex>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QtQmlIntegration>

#include <cstdint>
#include <span>

namespace phvikapen::app {

enum class CommandGroup : std::uint8_t {
    File,
    Edit,
    Arrange,
    View,
    Insert,
    Tools,
    Help,
};

// One command of the application, named once and answered to from the menus, the palette, the
// keyboard and the menu that opens under the pointer.
struct Command {
    QString id;
    // Kept as it was written rather than as it reads, so that changing the language changes what
    // the reader sees: the table is built once, and a name translated once would stay that way.
    const char* name{""};
    CommandGroup group{CommandGroup::Edit};
    QString keys;
    QKeySequence::StandardKey standard{QKeySequence::UnknownKey};
};

[[nodiscard]] std::span<const Command> commands();

// What a command is called, in the language the application is speaking just now.
[[nodiscard]] QString nameOf(const Command& command);

[[nodiscard]] const Command* commandOf(const QString& commandId);

// The keys a command answers to when nothing has been changed, always in portable form so that
// the toolkit reads them back the same way it wrote them.
[[nodiscard]] QString defaultKeysOf(const QString& commandId);

[[nodiscard]] QString nameOfGroup(CommandGroup group);

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
    static constexpr int kGroupRole = Qt::UserRole + 5;

    explicit ShortcutListModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setSequences(const QVariantMap& sequences);

    // The names of every command, in the order they are listed.
    Q_INVOKABLE [[nodiscard]] static QStringList ids();

signals:
    void countChanged();

private:
    QVariantMap m_sequences;
};

}
