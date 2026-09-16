#include "app/cpp/NotebookViewModel.hpp"

#include "core/Error.hpp"

#include <QDir>
#include <QStandardPaths>
#include <QString>
#include <QtLogging>

#include <filesystem>
#include <utility>
#include <vector>

namespace phvikapen::app {
namespace {

/// Name of the notebook the application opens at startup.
constexpr auto kDefaultNotebookName = "default.phvika";

[[nodiscard]] QString notebookDirectory() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/notebooks";
}

} // namespace

NotebookViewModel::NotebookViewModel(QObject* parent) : QObject(parent) {
    openNotebook();
}

void NotebookViewModel::openNotebook() {
    const QString directory = notebookDirectory();
    if (!QDir().mkpath(directory)) {
        reportError(tr("Could not create %1").arg(directory));
        return;
    }

    const std::filesystem::path path =
        std::filesystem::path{directory.toStdString()} / kDefaultNotebookName;
    core::Result<core::NotebookStore> store = core::NotebookStore::open(path);
    if (!store) {
        reportError(QString::fromStdString(store.error().message));
        return;
    }
    m_store = std::move(*store);
}

void NotebookViewModel::setCanvas(platform::ink::QtInkItem* canvas) {
    if (m_canvas == canvas) {
        return;
    }
    if (m_canvas != nullptr) {
        m_canvas->setSink(nullptr);
    }
    m_canvas = canvas;
    emit canvasChanged();

    if (m_canvas == nullptr) {
        return;
    }
    m_canvas->setSink(&m_sink);

    if (!m_store) {
        return;
    }
    core::Result<std::vector<core::Stroke>> strokes = m_store->strokesOfPage(m_pageId);
    if (!strokes) {
        reportError(QString::fromStdString(strokes.error().message));
        return;
    }
    m_storedStrokeCount = static_cast<int>(strokes->size());
    emit storedStrokeCountChanged();
    m_canvas->setStrokes(std::move(*strokes));
}

void NotebookViewModel::Sink::strokeStarted(const core::InkSample& /*sample*/) {}

void NotebookViewModel::Sink::sampleAdded(const core::InkSample& /*sample*/) {}

void NotebookViewModel::Sink::strokeFinished(const core::InkSample& /*sample*/) {}

void NotebookViewModel::Sink::strokeCompleted(const core::Stroke& stroke) {
    m_owner->storeStroke(stroke);
}

// An interrupted stroke never reaches the notebook, so there is nothing to undo here.
void NotebookViewModel::Sink::strokeCancelled() {}

void NotebookViewModel::storeStroke(const core::Stroke& stroke) {
    if (!m_store) {
        return;
    }
    // The stroke stays on the canvas even when storing it fails, so that nothing is lost silently.
    if (const core::Result<void> stored = m_store->appendStroke(m_pageId, stroke); !stored) {
        reportError(QString::fromStdString(stored.error().message));
        return;
    }
    ++m_storedStrokeCount;
    emit storedStrokeCountChanged();
}

void NotebookViewModel::reportError(const QString& message) {
    qWarning("%s", qUtf8Printable(message));
    m_errorMessage = message;
    emit errorMessageChanged();
}

} // namespace phvikapen::app
