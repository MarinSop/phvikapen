#pragma once

#include "core/Error.hpp"
#include "core/id/ContentId.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Asset.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/storage/NotebookStore.hpp"

#include <condition_variable>
#include <deque>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>
#include <vector>

namespace phvikapen::core {

class StorageThread {
public:
    using ErrorHandler = std::function<void(const Error&)>;
    using PageHandler = std::function<void(Result<std::vector<PlacedStroke>>)>;
    using OutlineHandler = std::function<void(Result<NotebookOutline>)>;
    using AssetHandler = std::function<void(Result<Asset>)>;
    using Change = std::function<Result<void>(NotebookStore&)>;

    StorageThread(std::filesystem::path path, ErrorHandler onError);
    ~StorageThread();

    StorageThread(const StorageThread&) = delete;
    StorageThread& operator=(const StorageThread&) = delete;
    StorageThread(StorageThread&&) = delete;
    StorageThread& operator=(StorageThread&&) = delete;

    void loadOutline(OutlineHandler onLoaded);
    void loadAsset(const ContentId& assetId, AssetHandler onLoaded);
    void loadPage(const Uuid& pageId, PageHandler onLoaded);
    void submit(Change change);
    void insertStroke(const Uuid& pageId, PlacedStroke placed);
    void removeStroke(const Uuid& pageId, const Uuid& strokeId);
    void removeStrokesOfPage(const Uuid& pageId);

    void waitUntilIdle();

private:
    using Task = std::function<void()>;

    void post(Task task);
    void run(const std::stop_token& stopToken);
    void open(const std::filesystem::path& path);
    void write(const Change& change);

    ErrorHandler m_onError;
    std::optional<NotebookStore> m_store;
    std::mutex m_mutex;
    std::condition_variable_any m_queued;
    std::condition_variable m_idle;
    std::deque<Task> m_tasks;
    bool m_busy{false};
    std::jthread m_thread;
};

}
