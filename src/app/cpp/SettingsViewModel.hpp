#pragma once

#include "app/cpp/OutlineModels.hpp"
#include "core/model/PageStyle.hpp"

#include <QObject>
#include <QString>
#include <QUrl>
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

    Q_INVOKABLE void showNotebookFolder();

signals:
    void lookForUpdatesChanged();
    void pageStyleChanged();

private:
    void changeStyle(const core::PageStyle& style);

    core::PageStyle m_style;
    QString m_folder;
    bool m_lookForUpdates{true};
};

}
