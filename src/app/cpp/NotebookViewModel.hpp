#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/model/Page.hpp"
#include "core/storage/StorageThread.hpp"
#include "core/undo/UndoStack.hpp"
#include "platform/ink/IInkBackend.hpp"
#include "platform/ink/qt/QtInkItem.hpp"

#include <QObject>
#include <QPointer>
#include <QQmlParserStatus>
#include <QString>
#include <QtQmlIntegration>

#include <cstdint>
#include <optional>
#include <vector>

namespace phvikapen::app {

// TODO(M3): Replace the single page with notebooks, sections and pages.
class NotebookViewModel : public QObject, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT
    Q_PROPERTY(QString notebookPath READ notebookPath WRITE setNotebookPath NOTIFY
                   notebookPathChanged FINAL)
    Q_PROPERTY(phvikapen::platform::ink::QtInkItem* canvas READ canvas WRITE setCanvas NOTIFY
                   canvasChanged FINAL)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged FINAL)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged FINAL)
    Q_PROPERTY(int strokeCount READ strokeCount NOTIFY pageChanged FINAL)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged FINAL)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged FINAL)

public:
    explicit NotebookViewModel(QObject* parent = nullptr);
    ~NotebookViewModel() override;

    NotebookViewModel(const NotebookViewModel&) = delete;
    NotebookViewModel& operator=(const NotebookViewModel&) = delete;
    NotebookViewModel(NotebookViewModel&&) = delete;
    NotebookViewModel& operator=(NotebookViewModel&&) = delete;

    void classBegin() override {}

    void componentComplete() override;

    [[nodiscard]] QString notebookPath() const { return m_notebookPath; }

    void setNotebookPath(const QString& path);

    [[nodiscard]] platform::ink::QtInkItem* canvas() const { return m_canvas; }

    void setCanvas(platform::ink::QtInkItem* canvas);

    [[nodiscard]] bool loaded() const { return m_loaded; }

    [[nodiscard]] QString errorMessage() const { return m_errorMessage; }

    [[nodiscard]] int strokeCount() const { return static_cast<int>(m_page.strokes().size()); }

    [[nodiscard]] bool canUndo() const { return m_history.canUndo(); }

    [[nodiscard]] bool canRedo() const { return m_history.canRedo(); }

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void clearPage();

signals:
    void notebookPathChanged();
    void canvasChanged();
    void loadedChanged();
    void errorMessageChanged();
    void pageChanged();
    void historyChanged();

private:
    class Sink final : public platform::ink::IInkSink {
    public:
        explicit Sink(NotebookViewModel* owner) noexcept : m_owner{owner} {}

        void strokeStarted(const core::InkSample& sample) override;
        void sampleAdded(const core::InkSample& sample) override;
        void strokeFinished(const core::InkSample& sample) override;
        void strokeCompleted(const core::Stroke& stroke) override;
        void strokeCancelled() override;

    private:
        NotebookViewModel* m_owner;
    };

    void openNotebook();
    void showLoadedPage(std::uint64_t opening,
                        core::Result<std::vector<core::PlacedStroke>> strokes);
    void storeStroke(const core::Stroke& stroke);
    void finishChange(const core::Result<void>& change);
    void refreshCanvas();
    void reportError(const QString& message);

    Sink m_sink{this};
    core::Page m_page;
    std::optional<core::StorageThread> m_storage;
    core::UndoStack m_history;
    QPointer<platform::ink::QtInkItem> m_canvas;
    QString m_notebookPath;
    QString m_errorMessage;
    std::uint64_t m_opening{0};
    bool m_completed{false};
    bool m_loaded{false};
};

}
