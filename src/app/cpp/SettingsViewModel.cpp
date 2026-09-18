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

#include <algorithm>

namespace phvikapen::app {
namespace {

constexpr auto kLookForUpdatesSetting = "updates/lookAtStart";
constexpr auto kShortcutPrefix = "shortcuts/";
constexpr auto kSmoothingSetting = "ink/smoothing";
constexpr auto kExportScopeSetting = "export/scope";
constexpr auto kContinuousPagesSetting = "view/continuous";
constexpr auto kPagesPanelSetting = "view/pagesPanel";
constexpr auto kPagePanelSetting = "view/pagePanel";
constexpr int kExportScopes = 3;
constexpr float kOwnPaperWidth = core::millimeters(210.0F);
constexpr float kOwnPaperHeight = core::millimeters(297.0F);

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
    m_smoothing = std::clamp(settings.value(kSmoothingSetting, m_smoothing).toDouble(), 0.0, 1.0);
    m_exportScope = std::clamp(settings.value(kExportScopeSetting, m_exportScope).toInt(), 0,
                               kExportScopes - 1);
    m_continuousPages = settings.value(kContinuousPagesSetting, m_continuousPages).toBool();
    m_showPagesPanel = settings.value(kPagesPanelSetting, m_showPagesPanel).toBool();
    m_showPagePanel = settings.value(kPagePanelSetting, m_showPagePanel).toBool();
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
    if (style.paper == core::Paper::Custom && !core::paperSize(style)) {
        style.customWidth = kOwnPaperWidth;
        style.customHeight = kOwnPaperHeight;
    }
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

void SettingsViewModel::setSmoothing(qreal smoothing) {
    const qreal wanted = std::clamp(smoothing, 0.0, 1.0);
    if (qFuzzyCompare(wanted + 1.0, m_smoothing + 1.0)) {
        return;
    }
    m_smoothing = wanted;
    QSettings settings;
    settings.setValue(kSmoothingSetting, m_smoothing);
    emit smoothingChanged();
}

void SettingsViewModel::setExportScope(int scope) {
    const int wanted = std::clamp(scope, 0, kExportScopes - 1);
    if (wanted == m_exportScope) {
        return;
    }
    m_exportScope = wanted;
    QSettings settings;
    settings.setValue(kExportScopeSetting, m_exportScope);
    emit exportScopeChanged();
}

void SettingsViewModel::setContinuousPages(bool continuous) {
    if (continuous == m_continuousPages) {
        return;
    }
    m_continuousPages = continuous;
    QSettings settings;
    settings.setValue(kContinuousPagesSetting, m_continuousPages);
    emit continuousPagesChanged();
}

void SettingsViewModel::setShowPagesPanel(bool shown) {
    if (shown == m_showPagesPanel) {
        return;
    }
    m_showPagesPanel = shown;
    QSettings settings;
    settings.setValue(kPagesPanelSetting, m_showPagesPanel);
    emit panelsChanged();
}

void SettingsViewModel::setShowPagePanel(bool shown) {
    if (shown == m_showPagePanel) {
        return;
    }
    m_showPagePanel = shown;
    QSettings settings;
    settings.setValue(kPagePanelSetting, m_showPagePanel);
    emit panelsChanged();
}

qreal SettingsViewModel::customWidth() const {
    return m_style.customWidth / core::millimeters(1.0F);
}

void SettingsViewModel::setCustomWidth(qreal millimeters) {
    core::PageStyle style = m_style;
    style.customWidth = core::millimeters(static_cast<float>(millimeters));
    changeStyle(core::normalized(style));
}

qreal SettingsViewModel::customHeight() const {
    return m_style.customHeight / core::millimeters(1.0F);
}

void SettingsViewModel::setCustomHeight(qreal millimeters) {
    core::PageStyle style = m_style;
    style.customHeight = core::millimeters(static_cast<float>(millimeters));
    changeStyle(core::normalized(style));
}

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
