#include "core/storage/StorageThread.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Page.hpp"
#include "core/storage/NotebookStore.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <mutex>
#include <stop_token>
#include <utility>
#include <vector>

namespace phvikapen::core {

StorageThread::StorageThread(std::filesystem::path path, ErrorHandler onError)
    : m_onError{std::move(onError)},
      m_thread{[this](const std::stop_token& stopToken) { run(stopToken); }} {
    post([this, path = std::move(path)] { open(path); });
}

StorageThread::~StorageThread() {
    m_thread.request_stop();
    m_thread.join();
}

void StorageThread::loadPage(const Uuid& pageId, PageHandler onLoaded) {
    post([this, pageId, onLoaded = std::move(onLoaded)] {
        if (!m_store) {
            onLoaded(makeError(ErrorCode::IoFailure, "the notebook is not open"));
            return;
        }
        onLoaded(m_store->strokesOfPage(pageId));
    });
}

void StorageThread::insertStroke(const Uuid& pageId, PlacedStroke placed) {
    post([this, pageId, placed = std::move(placed)] {
        write([&](NotebookStore& store) { return store.insertStroke(pageId, placed); });
    });
}

void StorageThread::removeStroke(const Uuid& pageId, const Uuid& strokeId) {
    post([this, pageId, strokeId] {
        write([&](NotebookStore& store) { return store.removeStroke(pageId, strokeId); });
    });
}

void StorageThread::removeStrokesOfPage(const Uuid& pageId) {
    post([this, pageId] {
        write([&](NotebookStore& store) -> Result<void> {
            const Result<std::size_t> removed = store.removeStrokesOfPage(pageId);
            if (!removed) {
                return std::unexpected{removed.error()};
            }
            return {};
        });
    });
}

void StorageThread::waitUntilIdle() {
    std::unique_lock lock{m_mutex};
    m_idle.wait(lock, [this] { return m_tasks.empty() && !m_busy; });
}

void StorageThread::post(Task task) {
    {
        const std::scoped_lock lock{m_mutex};
        m_tasks.push_back(std::move(task));
    }
    m_queued.notify_one();
}

void StorageThread::open(const std::filesystem::path& path) {
    Result<NotebookStore> opened = NotebookStore::open(path);
    if (!opened) {
        m_onError(opened.error());
        return;
    }
    m_store = std::move(*opened);
}

void StorageThread::write(const std::function<Result<void>(NotebookStore&)>& change) {
    if (!m_store) {
        return;
    }
    if (const Result<void> written = change(*m_store); !written) {
        m_onError(written.error());
    }
}

void StorageThread::run(const std::stop_token& stopToken) {
    while (true) {
        Task task;
        {
            std::unique_lock lock{m_mutex};
            m_busy = false;
            if (m_tasks.empty()) {
                m_idle.notify_all();
            }
            m_queued.wait(lock, stopToken, [this] { return !m_tasks.empty(); });
            if (m_tasks.empty()) {
                break;
            }
            task = std::move(m_tasks.front());
            m_tasks.pop_front();
            m_busy = true;
        }
        task();
    }
    m_store.reset();
}

}
