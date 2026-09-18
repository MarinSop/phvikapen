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
    Q_PROPERTY(QString notebookFolder READ notebookFolder CONSTANT FINAL)
    Q_PROPERTY(phvikapen::app::page_options::Paper paper READ paper WRITE setPaper NOTIFY
                   pageStyleChanged FINAL)
    Q_PROPERTY(phvikapen::app::page_options::Background background READ background WRITE
                   setBackground NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(bool landscape READ landscape WRITE setLandscape NOTIFY pageStyleChanged FINAL)
    Q_PROPERTY(qreal smoothing READ smoothing WRITE setSmoothing NOTIFY smoothingChanged FINAL)
    Q_PROPERTY(bool exportEverything READ exportEverything WRITE setExportEverything NOTIFY
                   exportEverythingChanged FINAL)
    Q_PROPERTY(QVariantMap shortcuts READ shortcuts NOTIFY shortcutsChanged FINAL)
    Q_PROPERTY(phvikapen::app::ShortcutListModel* shortcutList READ shortcutList CONSTANT FINAL)

public:
    explicit SettingsViewModel(QObject* parent = nullptr);

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

    [[nodiscard]] bool exportEverything() const { return m_exportEverything; }

    void setExportEverything(bool everything);

    [[nodiscard]] QVariantMap shortcuts() const { return m_shortcuts; }

    [[nodiscard]] ShortcutListModel* shortcutList() { return &m_shortcutList; }

    Q_INVOKABLE bool changeShortcut(const QString& commandId, const QString& sequence);
    Q_INVOKABLE void resetShortcut(const QString& commandId);
    Q_INVOKABLE [[nodiscard]] QString conflictWith(const QString& commandId,
                                                   const QString& sequence) const;

    Q_INVOKABLE void showNotebookFolder();

signals:
    void lookForUpdatesChanged();
    void pageStyleChanged();
    void shortcutsChanged();
    void smoothingChanged();
    void exportEverythingChanged();

private:
    void changeStyle(const core::PageStyle& style);

    core::PageStyle m_style;
    QVariantMap m_shortcuts;
    qreal m_smoothing{kDefaultSmoothing};
    bool m_exportEverything{true};
    ShortcutListModel m_shortcutList;
    QString m_folder;
    bool m_lookForUpdates{true};
};

}
