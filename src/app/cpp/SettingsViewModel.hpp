#pragma once

#include "app/cpp/OutlineModels.hpp"
#include "app/cpp/Shortcuts.hpp"
#include "core/model/PageStyle.hpp"

#include <QColor>
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
    Q_PROPERTY(
        bool testVersions READ testVersions WRITE setTestVersions NOTIFY testVersionsChanged FINAL)
    Q_PROPERTY(int theme READ theme WRITE setTheme NOTIFY themeChanged FINAL)
    Q_PROPERTY(QString notebookFolder READ notebookFolder CONSTANT FINAL)
    Q_PROPERTY(phvikapen::app::page_options::Paper paper READ paper WRITE setPaper NOTIFY
                   pageStyleChanged FINAL)
    Q_PROPERTY(phvikapen::app::page_options::Background background READ background WRITE
                   setBackground NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(bool landscape READ landscape WRITE setLandscape NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(QColor paperColor READ paperColor WRITE setPaperColor NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(
        QColor marginColor READ marginColor WRITE setMarginColor NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(
        qreal lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(qreal marginAt READ marginAt WRITE setMarginAt NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(bool margin READ margin WRITE setMargin NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(qreal smoothing READ smoothing WRITE setSmoothing NOTIFY smoothingChanged FINAL)
    Q_PROPERTY(bool usePressure READ usePressure WRITE setUsePressure NOTIFY drawingChanged FINAL)
    Q_PROPERTY(qreal zoomStep READ zoomStep WRITE setZoomStep NOTIFY drawingChanged FINAL)
    Q_PROPERTY(qreal uiScale READ uiScale WRITE setUiScale NOTIFY drawingChanged FINAL)
    Q_PROPERTY(bool holdForMenu READ holdForMenu WRITE setHoldForMenu NOTIFY drawingChanged FINAL)
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

    [[nodiscard]] bool testVersions() const { return m_testVersions; }

    void setLookForUpdates(bool wanted);

    void setTestVersions(bool wanted);

    [[nodiscard]] QString notebookFolder() const;

    [[nodiscard]] page_options::Paper paper() const;
    void setPaper(page_options::Paper paper);
    [[nodiscard]] page_options::Background background() const;
    void setBackground(page_options::Background background);
    [[nodiscard]] QColor paperColor() const;
    void setPaperColor(const QColor& color);
    [[nodiscard]] QColor lineColor() const;
    void setLineColor(const QColor& color);
    [[nodiscard]] QColor marginColor() const;
    void setMarginColor(const QColor& color);
    [[nodiscard]] qreal lineWidth() const;
    void setLineWidth(qreal width);
    [[nodiscard]] qreal lineSpacing() const;
    void setLineSpacing(qreal millimeters);
    [[nodiscard]] qreal marginAt() const;
    void setMarginAt(qreal millimeters);
    [[nodiscard]] bool margin() const;
    void setMargin(bool shown);

    [[nodiscard]] bool landscape() const;
    void setLandscape(bool landscape);

    static constexpr qreal kDefaultSmoothing = 0.5;
    static constexpr qreal kDefaultZoomStep = 1.25;
    static constexpr qreal kGentlestZoomStep = 1.05;
    static constexpr qreal kBoldestZoomStep = 2.0;
    static constexpr qreal kDefaultUiScale = 1.0;
    static constexpr qreal kSmallestUiScale = 0.85;
    static constexpr qreal kLargestUiScale = 1.6;

    [[nodiscard]] qreal smoothing() const { return m_smoothing; }

    void setSmoothing(qreal smoothing);

    // Whether how hard the pen is pressed changes how wide the line is.
    [[nodiscard]] bool usePressure() const { return m_usePressure; }

    void setUsePressure(bool used);

    // How much closer one step of the zoom brings the page.
    [[nodiscard]] qreal zoomStep() const { return m_zoomStep; }

    void setZoomStep(qreal step);

    // How large the buttons, bars and menus are drawn, for a hand holding a pen rather than a
    // mouse.
    [[nodiscard]] qreal uiScale() const { return m_uiScale; }

    void setUiScale(qreal scale);

    // Whether holding the pen still opens the menu of what can be done, as pressing and holding
    // does elsewhere on the platform.
    [[nodiscard]] bool holdForMenu() const { return m_holdForMenu; }

    void setHoldForMenu(bool wanted);

    // Everything on this page back to what it came with.
    Q_INVOKABLE void resetDrawing();

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

    // The one place the keys of a command are decided: what the reader chose, or what it came
    // with. Menus, the palette and the keyboard all ask here, so none of them can drift apart.
    Q_INVOKABLE [[nodiscard]] QString keysFor(const QString& commandId) const;

    // What a command answers to when nobody has changed it. Never changes while the application
    // runs, so it is safe to ask for it from anywhere.
    Q_INVOKABLE [[nodiscard]] static QString defaultKeys(const QString& commandId);

    Q_INVOKABLE bool changeShortcut(const QString& commandId, const QString& sequence);
    Q_INVOKABLE void resetShortcut(const QString& commandId);
    Q_INVOKABLE [[nodiscard]] QString conflictWith(const QString& commandId,
                                                   const QString& sequence) const;

    Q_INVOKABLE void showNotebookFolder();

signals:
    void lookForUpdatesChanged();
    void testVersionsChanged();
    void themeChanged();
    void pageStyleChanged();
    void shortcutsChanged();
    void smoothingChanged();
    void drawingChanged();
    void exportScopeChanged();
    void continuousPagesChanged();
    void panelsChanged();

private:
    void changeStyle(const core::PageStyle& style);
    void applyTheme() const;

    core::PageStyle m_style;
    QVariantMap m_shortcuts;
    qreal m_smoothing{kDefaultSmoothing};
    qreal m_zoomStep{kDefaultZoomStep};
    qreal m_uiScale{kDefaultUiScale};
    bool m_usePressure{true};
    bool m_holdForMenu{true};
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
    bool m_testVersions{false};
};

}
