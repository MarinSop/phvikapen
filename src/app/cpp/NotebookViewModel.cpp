#include "app/cpp/NotebookViewModel.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/StrokeHitTest.hpp"
#include "core/model/Page.hpp"
#include "core/undo/StrokeCommands.hpp"

#include <QDir>
#include <QMetaObject>
#include <QStandardPaths>
#include <QString>
#include <QtLogging>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

namespace phvikapen::app {
namespace {

constexpr auto kDefaultNotebookName = "default.phvika";
constexpr float kEraserRadius = 8.0F;

[[nodiscard]] QString notebookDirectory() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/notebooks";
}

}

NotebookViewModel::NotebookViewModel(QObject* parent) : QObject(parent), m_page{core::Uuid{}} {}

NotebookViewModel::~NotebookViewModel() {
    if (!m_canvas.isNull()) {
        m_canvas->setSink(nullptr);
    }
}

void NotebookViewModel::componentComplete() {
    m_completed = true;
    openNotebook();
}

void NotebookViewModel::setNotebookPath(const QString& path) {
    if (m_notebookPath == path) {
        return;
    }
    m_notebookPath = path;
    emit notebookPathChanged();
    if (m_completed) {
        openNotebook();
    }
}

void NotebookViewModel::openNotebook() {
    m_history.clear();
    m_erasing.clear();
    m_storage.reset();
    m_page = core::Page{m_page.id()};
    const std::uint64_t opening = ++m_opening;
    if (m_loaded) {
        m_loaded = false;
        emit loadedChanged();
    }
    if (!m_errorMessage.isEmpty()) {
        m_errorMessage.clear();
        emit errorMessageChanged();
    }
    emit pageChanged();
    emit historyChanged();
    refreshCanvas();

    QString file = m_notebookPath;
    if (file.isEmpty()) {
        const QString directory = notebookDirectory();
        if (!QDir().mkpath(directory)) {
            reportError(tr("Could not create %1").arg(directory));
            return;
        }
        file = directory + "/" + kDefaultNotebookName;
    }

    const std::filesystem::path path{file.toStdU16String()};
    m_storage.emplace(path, [this](const core::Error& error) {
        QMetaObject::invokeMethod(
            this, [this, message = QString::fromStdString(error.message)] { reportError(message); },
            Qt::QueuedConnection);
    });
    m_storage->loadPage(m_page.id(),
                        [this, opening](core::Result<std::vector<core::PlacedStroke>> strokes) {
                            QMetaObject::invokeMethod(
                                this,
                                [this, opening, strokes = std::move(strokes)] mutable {
                                    showLoadedPage(opening, std::move(strokes));
                                },
                                Qt::QueuedConnection);
                        });
}

void NotebookViewModel::showLoadedPage(std::uint64_t opening,
                                       core::Result<std::vector<core::PlacedStroke>> strokes) {
    if (opening != m_opening) {
        return;
    }
    if (!strokes) {
        reportError(QString::fromStdString(strokes.error().message));
        return;
    }
    m_page = core::Page{m_page.id(), std::move(*strokes)};
    m_loaded = true;
    emit loadedChanged();
    emit pageChanged();
    refreshCanvas();
}

void NotebookViewModel::setCanvas(platform::ink::QtInkItem* canvas) {
    if (m_canvas == canvas) {
        return;
    }
    if (!m_canvas.isNull()) {
        m_canvas->setSink(nullptr);
    }
    m_canvas = canvas;
    emit canvasChanged();

    if (m_canvas.isNull()) {
        return;
    }
    m_canvas->setSink(&m_sink);
    refreshCanvas();
}

void NotebookViewModel::Sink::strokeStarted(const core::InkSample& /*sample*/) {}

void NotebookViewModel::Sink::sampleAdded(const core::InkSample& /*sample*/) {}

void NotebookViewModel::Sink::strokeFinished(const core::InkSample& /*sample*/) {}

void NotebookViewModel::Sink::strokeCompleted(const core::Stroke& stroke) {
    m_owner->storeStroke(stroke);
}

void NotebookViewModel::Sink::strokeCancelled() {}

void NotebookViewModel::Sink::eraserMoved(const core::InkSample& from, const core::InkSample& to) {
    m_owner->erase(from, to);
}

void NotebookViewModel::Sink::eraseFinished() {
    m_owner->finishErasing();
}

void NotebookViewModel::storeStroke(const core::Stroke& stroke) {
    if (!m_loaded || !m_storage) {
        refreshCanvas();
        return;
    }
    const core::Result<void> stored = m_history.run(std::make_unique<core::AddStrokeCommand>(
        &m_page, &*m_storage,
        core::PlacedStroke{.ordinal = m_page.nextOrdinal(), .stroke = stroke}));
    if (!stored) {
        reportError(QString::fromStdString(stored.error().message));
        refreshCanvas();
        return;
    }
    emit pageChanged();
    emit historyChanged();
}

void NotebookViewModel::erase(const core::InkSample& from, const core::InkSample& to) {
    if (!m_loaded) {
        return;
    }
    const core::EraserSweep sweep{
        .from = {.x = from.x, .y = from.y},
        .to = {.x = to.x, .y = to.y},
        .radius = kEraserRadius,
    };
    bool found = false;
    for (const core::Uuid& strokeId : m_page.strokesTouchedBy(sweep)) {
        if (std::ranges::find(m_erasing, strokeId) == m_erasing.end()) {
            m_erasing.push_back(strokeId);
            found = true;
        }
    }
    if (found) {
        refreshCanvas();
    }
}

void NotebookViewModel::finishErasing() {
    if (m_erasing.empty() || !m_storage) {
        m_erasing.clear();
        return;
    }
    finishChange(m_history.run(std::make_unique<core::EraseStrokesCommand>(
        &m_page, &*m_storage, std::exchange(m_erasing, {}))));
}

void NotebookViewModel::undo() {
    m_erasing.clear();
    if (m_history.canUndo()) {
        finishChange(m_history.undo());
    }
}

void NotebookViewModel::redo() {
    m_erasing.clear();
    if (m_history.canRedo()) {
        finishChange(m_history.redo());
    }
}

void NotebookViewModel::clearPage() {
    m_erasing.clear();
    if (m_loaded && m_storage) {
        finishChange(m_history.run(std::make_unique<core::ClearPageCommand>(&m_page, &*m_storage)));
    }
}

void NotebookViewModel::finishChange(const core::Result<void>& change) {
    if (!change) {
        reportError(QString::fromStdString(change.error().message));
    }
    refreshCanvas();
    emit pageChanged();
    emit historyChanged();
}

void NotebookViewModel::refreshCanvas() {
    if (!m_canvas.isNull()) {
        m_canvas->showPage(m_page, m_erasing);
    }
}

void NotebookViewModel::reportError(const QString& message) {
    qWarning("%s", qUtf8Printable(message));
    m_errorMessage = message;
    emit errorMessageChanged();
}

}
