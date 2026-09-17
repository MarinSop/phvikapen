#pragma once

#include "core/Error.hpp"
#include "core/id/ContentId.hpp"
#include "core/model/Asset.hpp"
#include "platform/pdf/IPdfDocument.hpp"

#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>

namespace phvikapen::platform::pdf {

class PdfRenderer {
public:
    using ImageHandler = std::function<void(core::Result<PageImage>)>;
    using DocumentHandler = std::function<void(core::Result<std::vector<PageSize>>)>;

    PdfRenderer();
    ~PdfRenderer();

    PdfRenderer(const PdfRenderer&) = delete;
    PdfRenderer& operator=(const PdfRenderer&) = delete;
    PdfRenderer(PdfRenderer&&) = delete;
    PdfRenderer& operator=(PdfRenderer&&) = delete;

    void open(core::Asset asset, DocumentHandler onOpened);
    void render(const core::ContentId& asset, int pageIndex, int widthInPixels, int heightInPixels,
                ImageHandler onRendered);
    void forget(const core::ContentId& asset);

    void waitUntilIdle();

private:
    using Task = std::function<void()>;

    void post(Task task);
    void run(const std::stop_token& stopToken);

    std::map<core::ContentId, std::unique_ptr<IPdfDocument>> m_documents;
    std::mutex m_mutex;
    std::condition_variable_any m_queued;
    std::condition_variable m_idle;
    std::deque<Task> m_tasks;
    bool m_busy{false};
    std::jthread m_thread;
};

}
