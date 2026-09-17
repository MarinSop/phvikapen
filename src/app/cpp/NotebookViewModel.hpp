#pragma once

#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/undo/UndoStack.hpp"
#include "platform/ink/IInkBackend.hpp"
#include "platform/ink/qt/QtInkItem.hpp"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QtQmlIntegration>

#include <cstdint>
#include <optional>

namespace phvikapen::app {

// TODO(M3): Replace the single page with notebooks, sections and pages.
class NotebookViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(phvikapen::platform::ink::QtInkItem* canvas READ canvas WRITE setCanvas NOTIFY
                   canvasChanged FINAL)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged FINAL)
    Q_PROPERTY(int storedStrokeCount READ storedStrokeCount NOTIFY storedStrokeCountChanged FINAL)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged FINAL)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged FINAL)

public:
    explicit NotebookViewModel(QObject* parent = nullptr);

    [[nodiscard]] platform::ink::QtInkItem* canvas() const { return m_canvas; }

    void setCanvas(platform::ink::QtInkItem* canvas);

    [[nodiscard]] QString errorMessage() const { return m_errorMessage; }

    [[nodiscard]] int storedStrokeCount() const { return m_storedStrokeCount; }

    [[nodiscard]] bool canUndo() const { return m_history.canUndo(); }

    [[nodiscard]] bool canRedo() const { return m_history.canRedo(); }

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void clearPage();

signals:
    void canvasChanged();
    void errorMessageChanged();
    void storedStrokeCountChanged();
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
    void storeStroke(const core::Stroke& stroke);
    void reloadCanvas();
    void reportError(const QString& message);

    Sink m_sink{this};
    std::optional<core::NotebookStore> m_store;
    core::UndoStack m_history;
    core::Uuid m_pageId;
    QPointer<platform::ink::QtInkItem> m_canvas;
    QString m_errorMessage;
    std::int64_t m_nextOrdinal{0};
    int m_storedStrokeCount{0};
};

}
