#include "app/cpp/SettingsViewModel.hpp"

#include "app/cpp/PageDefaults.hpp"
#include "core/model/PageStyle.hpp"

#include <QDesktopServices>
#include <QDir>
#include <QKeySequence>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QUrl>
#include <QVariant>

namespace phvikapen::app {
namespace {

constexpr auto kLookForUpdatesSetting = "updates/lookAtStart";
constexpr auto kShortcutPrefix = "shortcuts/";

[[nodiscard]] QString keysOf(const QString& sequence) {
    return QKeySequence{sequence, QKeySequence::PortableText}.toString(QKeySequence::PortableText);
}

}

SettingsViewModel::SettingsViewModel(QObject* parent)
    : QObject(parent),
      m_folder{QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/notebooks"} {
    const QSettings settings;
    m_lookForUpdates = settings.value(kLookForUpdatesSetting, true).toBool();
    m_style = defaults::pageStyle();
    for (const Command& command : commands()) {
        const QString kept = settings.value(kShortcutPrefix + command.id).toString();
        if (!kept.isEmpty()) {
            m_shortcuts.insert(command.id, kept);
        }
    }
    m_shortcutList.setSequences(m_shortcuts);
}

void SettingsViewModel::setLookForUpdates(bool wanted) {
    if (m_lookForUpdates == wanted) {
        return;
    }
    m_lookForUpdates = wanted;
    QSettings settings;
    settings.setValue(kLookForUpdatesSetting, wanted);
    emit lookForUpdatesChanged();
}

QString SettingsViewModel::notebookFolder() const {
    return QDir::toNativeSeparators(m_folder);
}

void SettingsViewModel::showNotebookFolder() {
    if (!QDir{}.mkpath(m_folder)) {
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_folder));
}

}

namespace phvikapen::app {

page_options::Paper SettingsViewModel::paper() const {
    return static_cast<page_options::Paper>(m_style.paper);
}

void SettingsViewModel::setPaper(page_options::Paper paper) {
    core::PageStyle style = m_style;
    style.paper = static_cast<core::Paper>(paper);
    changeStyle(style);
}

page_options::Background SettingsViewModel::background() const {
    return static_cast<page_options::Background>(m_style.background);
}

void SettingsViewModel::setBackground(page_options::Background background) {
    core::PageStyle style = m_style;
    style.background = static_cast<core::Background>(background);
    changeStyle(style);
}

bool SettingsViewModel::landscape() const {
    return m_style.orientation == core::Orientation::Landscape;
}

void SettingsViewModel::setLandscape(bool landscape) {
    core::PageStyle style = m_style;
    style.orientation = landscape ? core::Orientation::Landscape : core::Orientation::Portrait;
    changeStyle(style);
}

void SettingsViewModel::changeStyle(const core::PageStyle& style) {
    if (style == m_style) {
        return;
    }
    m_style = style;
    defaults::setPageStyle(style);
    emit pageStyleChanged();
}

}

namespace phvikapen::app {

QString SettingsViewModel::conflictWith(const QString& commandId, const QString& sequence) const {
    const QString wanted = keysOf(sequence);
    if (wanted.isEmpty()) {
        return {};
    }
    for (const Command& command : commands()) {
        if (command.id == commandId) {
            continue;
        }
        const QString theirs = keysOf(m_shortcuts.value(command.id, command.fallback).toString());
        if (theirs == wanted) {
            return command.name;
        }
    }
    return {};
}

bool SettingsViewModel::changeShortcut(const QString& commandId, const QString& sequence) {
    const QString wanted = keysOf(sequence);
    if (wanted.isEmpty() || !conflictWith(commandId, wanted).isEmpty()) {
        return false;
    }

    QSettings settings;
    m_shortcuts.insert(commandId, wanted);
    settings.setValue(kShortcutPrefix + commandId, wanted);
    m_shortcutList.setSequences(m_shortcuts);
    emit shortcutsChanged();
    return true;
}

void SettingsViewModel::resetShortcut(const QString& commandId) {
    if (m_shortcuts.remove(commandId) == 0) {
        return;
    }
    QSettings settings;
    settings.remove(kShortcutPrefix + commandId);
    m_shortcutList.setSequences(m_shortcuts);
    emit shortcutsChanged();
}

}
