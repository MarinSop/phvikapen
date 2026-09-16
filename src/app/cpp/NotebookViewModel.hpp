#pragma once

#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/storage/NotebookStore.hpp"
#include "platform/ink/IInkBackend.hpp"
#include "platform/ink/qt/QtInkItem.hpp"

#include <QObject>
#include <QString>
#include <QtQmlIntegration>

#include <optional>

namespace phvikapen::app {

/// Keeps what is drawn on a canvas in a notebook file.
///
/// The canvas knows nothing about files, and the storage layer knows nothing about Qt. This view
/// model sits between them: it opens the notebook, shows what is already in it, and writes every
/// finished stroke back.
///
/// TODO(M2): Write on a storage thread instead of the thread that draws.
/// TODO(M3): Replace the single page with the notebook, section and page hierarchy.
class NotebookViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(phvikapen::platform::ink::QtInkItem* canvas READ canvas WRITE setCanvas NOTIFY
                   canvasChanged FINAL)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged FINAL)
    Q_PROPERTY(int storedStrokeCount READ storedStrokeCount NOTIFY storedStrokeCountChanged FINAL)

public:
    explicit NotebookViewModel(QObject* parent = nullptr);

    [[nodiscard]] platform::ink::QtInkItem* canvas() const { return m_canvas; }

    /// Attaches the canvas whose strokes are stored, loading the page into it.
    void setCanvas(platform::ink::QtInkItem* canvas);

    /// Empty while everything works, otherwise the reason the notebook could not be used.
    [[nodiscard]] QString errorMessage() const { return m_errorMessage; }

    [[nodiscard]] int storedStrokeCount() const { return m_storedStrokeCount; }

signals:
    void canvasChanged();
    void errorMessageChanged();
    void storedStrokeCountChanged();

private:
    /// Receives what the canvas reports and hands the finished strokes to the view model.
    ///
    /// The view model itself stays a plain QObject: QML can only create a type that is nothing
    /// but a default constructible QObject, which a class that also implements an interface is
    /// not.
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
    void reportError(const QString& message);

    Sink m_sink{this};
    std::optional<core::NotebookStore> m_store;
    core::Uuid m_pageId;
    platform::ink::QtInkItem* m_canvas{nullptr};
    QString m_errorMessage;
    int m_storedStrokeCount{0};
};

} // namespace phvikapen::app
