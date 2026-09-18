#pragma once

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

public:
    explicit SettingsViewModel(QObject* parent = nullptr);

    [[nodiscard]] bool lookForUpdates() const { return m_lookForUpdates; }

    void setLookForUpdates(bool wanted);

    [[nodiscard]] QString notebookFolder() const;

    Q_INVOKABLE void showNotebookFolder();

signals:
    void lookForUpdatesChanged();

private:
    QString m_folder;
    bool m_lookForUpdates{true};
};

}
