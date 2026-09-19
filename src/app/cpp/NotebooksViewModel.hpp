#pragma once

#include "app/cpp/NotebookViewModel.hpp"
#include "platform/ink/qt/QtInkItem.hpp"

#include <QObject>
#include <QPointer>
#include <QQmlParserStatus>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtQmlIntegration>

#include <memory>
#include <vector>

namespace phvikapen::app {

class NotebooksViewModel : public QObject, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT
    Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged FINAL)
    Q_PROPERTY(QStringList library READ library NOTIFY libraryChanged FINAL)
    Q_PROPERTY(QStringList openNotebooks READ openNotebooks NOTIFY openNotebooksChanged FINAL)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged FINAL)
    Q_PROPERTY(phvikapen::app::NotebookViewModel* current READ current NOTIFY currentChanged FINAL)
    Q_PROPERTY(phvikapen::platform::ink::QtInkItem* canvas READ canvas WRITE setCanvas NOTIFY
                   canvasChanged FINAL)
    Q_PROPERTY(bool continuousPages READ continuousPages WRITE setContinuousPages NOTIFY
                   continuousPagesChanged FINAL)

public:
    explicit NotebooksViewModel(QObject* parent = nullptr);

    void classBegin() override {}

    void componentComplete() override;

    [[nodiscard]] QString directory() const { return m_directory; }

    void setDirectory(const QString& directory);

    [[nodiscard]] QStringList library() const { return m_library; }

    [[nodiscard]] QStringList openNotebooks() const;

    [[nodiscard]] int currentIndex() const { return m_currentIndex; }

    void setCurrentIndex(int index);
    [[nodiscard]] NotebookViewModel* current() const;

    [[nodiscard]] platform::ink::QtInkItem* canvas() const { return m_canvas; }

    void setCanvas(platform::ink::QtInkItem* canvas);

    [[nodiscard]] bool continuousPages() const { return m_continuousPages; }

    void setContinuousPages(bool continuous);

    Q_INVOKABLE void refreshLibrary();
    Q_INVOKABLE [[nodiscard]] QString suggestedName() const;
    Q_INVOKABLE [[nodiscard]] bool isNameFree(const QString& name) const;
    Q_INVOKABLE void createNotebook(const QString& name);
    Q_INVOKABLE void createNotebookWithSetup(const QString& name, int paper, int background,
                                             bool landscape, const QUrl& document = {});
    Q_INVOKABLE void openNotebook(const QString& name);
    Q_INVOKABLE void closeNotebook(int index);
    Q_INVOKABLE void moveNotebook(int from, int to);
    Q_INVOKABLE void renameNotebook(int index, const QString& name);
    void takeName(NotebookViewModel& notebook, const QString& wanted);
    Q_INVOKABLE void deleteNotebook(const QString& name);

signals:
    void directoryChanged();
    void libraryChanged();
    void openNotebooksChanged();
    void currentChanged();
    void canvasChanged();
    void continuousPagesChanged();
    void errorMessage(const QString& message);

private:
    [[nodiscard]] QString pathFor(const QString& name) const;
    [[nodiscard]] int indexOf(const QString& name) const;
    void show(int index);
    void restoreSession();
    void rememberSession() const;
    static void rememberPage(const NotebookViewModel& notebook);
    [[nodiscard]] static QString rememberedPage(const QString& name);

    std::vector<std::unique_ptr<NotebookViewModel>> m_open;
    QStringList m_library;
    QString m_directory;
    QPointer<platform::ink::QtInkItem> m_canvas;
    int m_currentIndex{-1};
    bool m_continuousPages{false};
    bool m_completed{false};
    bool m_restoring{false};
};

}
