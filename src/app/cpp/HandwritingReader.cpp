#include "app/cpp/HandwritingReader.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/Stroke.hpp"
#include "core/model/Page.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/text/InkWord.hpp"
#include "platform/text/IHandwriting.hpp"

#include <QMetaObject>
#include <QString>

#include <atomic>
#include <chrono>
#include <exception>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stop_token>
#include <utility>
#include <vector>

namespace phvikapen::app {
namespace {

// Long enough to leave a hand that is still writing alone, short enough that a reader does not wait.
constexpr auto kRestBeforeReading = std::chrono::milliseconds{1500};

}

HandwritingReader::HandwritingReader(QObject* parent) : QObject(parent) {}

HandwritingReader::~HandwritingReader() {
    close();
}

bool HandwritingReader::availableHere() {
    return platform::text::openHandwriting() != nullptr;
}

void HandwritingReader::open(const std::filesystem::path& path) {
    close();
    if (!availableHere()) {
        return;
    }
    m_path = path;
    {
        const std::scoped_lock held{m_mutex};
        m_asked = true;
    }
    // The thread carries nothing of its own: what it needs is here and stands still while it runs.
    m_thread = std::jthread{[this](const std::stop_token& stopToken) {
        try {
            run(stopToken);
        } catch (...) {
            // Nothing can be said from here without risking another throw, so the reader stops.
            m_stumbled.store(true, std::memory_order_relaxed);
        }
    }};
}

void HandwritingReader::close() {
    if (!m_thread.joinable()) {
        return;
    }
    m_thread.request_stop();
    m_wanted.notify_all();
    m_thread.join();
}

void HandwritingReader::nudge() {
    {
        const std::scoped_lock held{m_mutex};
        m_asked = true;
    }
    m_wanted.notify_all();
}

void HandwritingReader::report(const core::Error& error) {
    QMetaObject::invokeMethod(
        this, [this, message = QString::fromStdString(error.message)] { emit failed(message); },
        Qt::QueuedConnection);
}

bool HandwritingReader::waitForWork(const std::stop_token& stopToken) {
    if (m_stumbled.load(std::memory_order_relaxed)) {
        return false;
    }
    std::unique_lock waiting{m_mutex};
    if (!m_wanted.wait(waiting, stopToken, [this] { return m_asked; })) {
        return false;
    }
    m_asked = false;
    // A hand that goes on writing is left to finish before anything is read.
    while (m_wanted.wait_for(waiting, stopToken, kRestBeforeReading, [this] { return m_asked; })) {
        m_asked = false;
    }
    return !stopToken.stop_requested();
}

core::Result<void> HandwritingReader::readPage(core::NotebookStore& store,
                                               platform::text::IHandwriting& reader,
                                               const core::Uuid& pageId) {
    const core::Result<std::int64_t> revision = store.inkRevisionOfPage(pageId);
    if (!revision) {
        return std::unexpected{revision.error()};
    }
    const core::Result<std::vector<core::PlacedStroke>> placed = store.strokesOfPage(pageId);
    if (!placed) {
        return std::unexpected{placed.error()};
    }
    std::vector<core::Stroke> strokes;
    strokes.reserve(placed->size());
    for (const core::PlacedStroke& one : *placed) {
        strokes.push_back(one.stroke);
    }
    const core::Result<std::vector<core::InkWord>> words = reader.read(strokes);
    if (!words) {
        return std::unexpected{words.error()};
    }
    return store.setWordsOfPage(pageId, *revision, *words);
}

void HandwritingReader::readPages(const std::stop_token& stopToken, core::NotebookStore& store,
                                  platform::text::IHandwriting& reader) {
    while (!stopToken.stop_requested()) {
        const core::Result<std::vector<core::Uuid>> waiting = store.pagesWaitingToBeRead();
        if (!waiting) {
            report(waiting.error());
            return;
        }
        QMetaObject::invokeMethod(
            this,
            [this, left = static_cast<int>(waiting->size())] { emit pagesWaitingChanged(left); },
            Qt::QueuedConnection);
        if (waiting->empty()) {
            return;
        }
        if (const core::Result<void> read = readPage(store, reader, waiting->front()); !read) {
            report(read.error());
            return;
        }
        QMetaObject::invokeMethod(this, [this] { emit pageRead(); }, Qt::QueuedConnection);
    }
}

void HandwritingReader::run(const std::stop_token& stopToken) {
    try {
        const std::unique_ptr<platform::text::IHandwriting> reader =
            platform::text::openHandwriting();
        if (!reader) {
            return;
        }
        while (waitForWork(stopToken)) {
            core::Result<core::NotebookStore> store = core::NotebookStore::open(m_path);
            if (!store) {
                report(store.error());
                return;
            }
            readPages(stopToken, *store, *reader);
        }
    } catch (const std::exception& failure) {
        report(core::Error{.code = core::ErrorCode::Unknown, .message = failure.what()});
    }
}

}
