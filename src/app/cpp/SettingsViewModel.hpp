#pragma once

#include "app/cpp/OutlineModels.hpp"
#include "app/cpp/Shortcuts.hpp"
#include "core/model/PageStyle.hpp"

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantMap>
#include <QtQmlIntegration>

namespace phvikapen::app {

class SettingsViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool lookForUpdates READ lookForUpdates WRITE setLookForUpdates NOTIFY
                   lookForUpdatesChanged FINAL)
    Q_PROPERTY(int theme READ theme WRITE setTheme NOTIFY themeChanged FINAL)
    Q_PROPERTY(QString notebookFolder READ notebookFolder CONSTANT FINAL)
    Q_PROPERTY(phvikapen::app::page_options::Paper paper READ paper WRITE setPaper NOTIFY
                   pageStyleChanged FINAL)
    Q_PROPERTY(phvikapen::app::page_options::Background background READ background WRITE
                   setBackground NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(bool landscape READ landscape WRITE setLandscape NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(qreal smoothing READ smoothing WRITE setSmoothing NOTIFY smoothingChanged FINAL)
    Q_PROPERTY(
        int exportScope READ exportScope WRITE setExportScope NOTIFY exportScopeChanged FINAL)
    Q_PROPERTY(bool continuousPages READ continuousPages WRITE setContinuousPages NOTIFY
                   continuousPagesChanged FINAL)
    Q_PROPERTY(
        bool showPagePanel READ showPagePanel WRITE setShowPagePanel NOTIFY panelsChanged FINAL)
    Q_PROPERTY(bool themeChosen READ themeChosen WRITE setThemeChosen NOTIFY themeChanged FINAL)
    Q_PROPERTY(bool reopenNotebooks READ reopenNotebooks WRITE setReopenNotebooks NOTIFY
                   panelsChanged FINAL)
    Q_PROPERTY(bool showSections READ showSections WRITE setShowSections NOTIFY panelsChanged FINAL)
    Q_PROPERTY(bool showPages READ showPages WRITE setShowPages NOTIFY panelsChanged FINAL)
    Q_PROPERTY(int panelWidth READ panelWidth WRITE setPanelWidth NOTIFY panelsChanged FINAL)
    Q_PROPERTY(
        int sectionsHeight READ sectionsHeight WRITE setSectionsHeight NOTIFY panelsChanged FINAL)
    Q_PROPERTY(
        qreal customWidth READ customWidth WRITE setCustomWidth NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(
        qreal customHeight READ customHeight WRITE setCustomHeight NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(QVariantMap shortcuts READ shortcuts NOTIFY shortcutsChanged FINAL)
    Q_PROPERTY(phvikapen::app::ShortcutListModel* shortcutList READ shortcutList CONSTANT FINAL)

public:
    explicit SettingsViewModel(QObject* parent = nullptr);

    static constexpr int kThemeCount = 3;
    static constexpr int kLightTheme = 2;

    [[nodiscard]] int theme() const { return m_theme; }

    void setTheme(int theme);

    [[nodiscard]] bool lookForUpdates() const { return m_lookForUpdates; }

    void setLookForUpdates(bool wanted);

    [[nodiscard]] QString notebookFolder() const;

    [[nodiscard]] page_options::Paper paper() const;
    void setPaper(page_options::Paper paper);
    [[nodiscard]] page_options::Background background() const;
    void setBackground(page_options::Background background);
    [[nodiscard]] bool landscape() const;
    void setLandscape(bool landscape);

    static constexpr qreal kDefaultSmoothing = 0.5;

    [[nodiscard]] qreal smoothing() const { return m_smoothing; }

    void setSmoothing(qreal smoothing);

    [[nodiscard]] int exportScope() const { return m_exportScope; }

    void setExportScope(int scope);

    [[nodiscard]] bool continuousPages() const { return m_continuousPages; }

    void setContinuousPages(bool continuous);

    [[nodiscard]] bool showPagePanel() const { return m_showPagePanel; }

    static constexpr int kDefaultPanelWidth = 220;
    static constexpr int kDefaultSectionsHeight = 150;

    // Whether the notebooks that were open come back the next time, or the reader starts fresh.
    [[nodiscard]] bool reopenNotebooks() const { return m_reopenNotebooks; }

    void setReopenNotebooks(bool reopen);

    [[nodiscard]] bool themeChosen() const { return m_themeChosen; }

    void setThemeChosen(bool chosen);

    [[nodiscard]] bool showSections() const { return m_showSections; }

    void setShowSections(bool shown);

    [[nodiscard]] bool showPages() const { return m_showPages; }

    void setShowPages(bool shown);

    [[nodiscard]] int panelWidth() const { return m_panelWidth; }

    void setPanelWidth(int width);

    [[nodiscard]] int sectionsHeight() const { return m_sectionsHeight; }

    void setSectionsHeight(int height);

    void setShowPagePanel(bool shown);

    [[nodiscard]] qreal customWidth() const;
    void setCustomWidth(qreal millimeters);
    [[nodiscard]] qreal customHeight() const;
    void setCustomHeight(qreal millimeters);

    [[nodiscard]] QVariantMap shortcuts() const { return m_shortcuts; }

    [[nodiscard]] ShortcutListModel* shortcutList() { return &m_shortcutList; }

    Q_INVOKABLE bool changeShortcut(const QString& commandId, const QString& sequence);
    Q_INVOKABLE void resetShortcut(const QString& commandId);
    Q_INVOKABLE [[nodiscard]] QString conflictWith(const QString& commandId,
                                                   const QString& sequence) const;

    Q_INVOKABLE void showNotebookFolder();

signals:
    void lookForUpdatesChanged();
    void themeChanged();
    void pageStyleChanged();
    void shortcutsChanged();
    void smoothingChanged();
    void exportScopeChanged();
    void continuousPagesChanged();
    void panelsChanged();

private:
    void changeStyle(const core::PageStyle& style);
    void applyTheme() const;

    core::PageStyle m_style;
    QVariantMap m_shortcuts;
    qreal m_smoothing{kDefaultSmoothing};
    int m_exportScope{0};
    int m_theme{0};
    bool m_continuousPages{true};
    bool m_showPagePanel{false};
    bool m_themeChosen{false};
    bool m_reopenNotebooks{false};
    bool m_showSections{true};
    bool m_showPages{true};
    int m_panelWidth{kDefaultPanelWidth};
    int m_sectionsHeight{kDefaultSectionsHeight};
    ShortcutListModel m_shortcutList;
    QString m_folder;
    bool m_lookForUpdates{true};
};

}
