#include "app/cpp/SettingsViewModel.hpp"

#include "app/cpp/PageDefaults.hpp"
#include "core/model/PageStyle.hpp"

#include <QDesktopServices>
#include <QDir>
#include <QGuiApplication>
#include <QKeySequence>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStyleHints>
#include <QUrl>
#include <QVariant>

#include <algorithm>

namespace phvikapen::app {
namespace {

constexpr auto kLookForUpdatesSetting = "updates/lookAtStart";
constexpr auto kThemeSetting = "look/theme";
constexpr auto kShortcutPrefix = "shortcuts/";
constexpr auto kSmoothingSetting = "ink/smoothing";
constexpr auto kExportScopeSetting = "export/scope";
constexpr auto kContinuousPagesSetting = "view/continuous";
constexpr auto kPagePanelSetting = "view/pagePanel";
constexpr auto kThemeChosenSetting = "look/chosen";
constexpr auto kReopenSetting = "session/reopen";
constexpr auto kSectionsSetting = "view/sections";
constexpr auto kPagesSetting = "view/pages";
constexpr auto kPanelWidthSetting = "view/panelWidth";
constexpr auto kSectionsHeightSetting = "view/sectionsHeight";
constexpr int kNarrowestPanel = 140;
constexpr int kWidestPanel = 520;
constexpr int kShortestList = 60;
constexpr int kTallestList = 800;
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
    m_theme = std::clamp(settings.value(kThemeSetting, m_theme).toInt(), 0, kThemeCount - 1);
    applyTheme();
    m_style = defaults::pageStyle();
    m_smoothing = std::clamp(settings.value(kSmoothingSetting, m_smoothing).toDouble(), 0.0, 1.0);
    m_exportScope = std::clamp(settings.value(kExportScopeSetting, m_exportScope).toInt(), 0,
                               kExportScopes - 1);
    m_continuousPages = settings.value(kContinuousPagesSetting, m_continuousPages).toBool();
    m_themeChosen = settings.value(kThemeChosenSetting, m_themeChosen).toBool();
    m_reopenNotebooks = settings.value(kReopenSetting, m_reopenNotebooks).toBool();
    m_showSections = settings.value(kSectionsSetting, m_showSections).toBool();
    m_showPages = settings.value(kPagesSetting, m_showPages).toBool();
    m_panelWidth = std::clamp(settings.value(kPanelWidthSetting, m_panelWidth).toInt(),
                              kNarrowestPanel, kWidestPanel);
    m_sectionsHeight = std::clamp(settings.value(kSectionsHeightSetting, m_sectionsHeight).toInt(),
                                  kShortestList, kTallestList);
    m_showPagePanel = settings.value(kPagePanelSetting, m_showPagePanel).toBool();
    for (const Command& command : commands()) {
        const QString kept = settings.value(kShortcutPrefix + command.id).toString();
        if (!kept.isEmpty()) {
            m_shortcuts.insert(command.id, kept);
        }
    }
    m_shortcutList.setSequences(m_shortcuts);
}

// The style draws its own controls light or dark, so it is told which of the two this theme is.
void SettingsViewModel::applyTheme() const {
    QStyleHints* const hints = QGuiApplication::styleHints();
    if (hints != nullptr) {
        hints->setColorScheme(m_theme == kLightTheme ? Qt::ColorScheme::Light
                                                     : Qt::ColorScheme::Dark);
    }
}

void SettingsViewModel::setTheme(int theme) {
    const int wanted = std::clamp(theme, 0, kThemeCount - 1);
    if (wanted == m_theme) {
        return;
    }
    m_theme = wanted;
    QSettings settings;
    settings.setValue(kThemeSetting, m_theme);
    applyTheme();
    emit themeChanged();
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

void SettingsViewModel::setShowPagePanel(bool shown) {
    if (shown == m_showPagePanel) {
        return;
    }
    m_showPagePanel = shown;
    QSettings settings;
    settings.setValue(kPagePanelSetting, m_showPagePanel);
    emit panelsChanged();
}

void SettingsViewModel::setReopenNotebooks(bool reopen) {
    if (reopen == m_reopenNotebooks) {
        return;
    }
    m_reopenNotebooks = reopen;
    QSettings settings;
    settings.setValue(kReopenSetting, m_reopenNotebooks);
    emit panelsChanged();
}

void SettingsViewModel::setThemeChosen(bool chosen) {
    if (chosen == m_themeChosen) {
        return;
    }
    m_themeChosen = chosen;
    QSettings settings;
    settings.setValue(kThemeChosenSetting, m_themeChosen);
    emit themeChanged();
}

void SettingsViewModel::setShowSections(bool shown) {
    if (shown == m_showSections) {
        return;
    }
    m_showSections = shown;
    QSettings settings;
    settings.setValue(kSectionsSetting, m_showSections);
    emit panelsChanged();
}

void SettingsViewModel::setShowPages(bool shown) {
    if (shown == m_showPages) {
        return;
    }
    m_showPages = shown;
    QSettings settings;
    settings.setValue(kPagesSetting, m_showPages);
    emit panelsChanged();
}

void SettingsViewModel::setPanelWidth(int width) {
    const int wanted = std::clamp(width, kNarrowestPanel, kWidestPanel);
    if (wanted == m_panelWidth) {
        return;
    }
    m_panelWidth = wanted;
    QSettings settings;
    settings.setValue(kPanelWidthSetting, m_panelWidth);
    emit panelsChanged();
}

void SettingsViewModel::setSectionsHeight(int height) {
    const int wanted = std::clamp(height, kShortestList, kTallestList);
    if (wanted == m_sectionsHeight) {
        return;
    }
    m_sectionsHeight = wanted;
    QSettings settings;
    settings.setValue(kSectionsHeightSetting, m_sectionsHeight);
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
