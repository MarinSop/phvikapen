#include "platform/pdf/PdfRenderer.hpp"

#include "core/Error.hpp"
#include "core/id/ContentId.hpp"
#include "core/model/Asset.hpp"
#include "platform/pdf/IPdfDocument.hpp"
#include "platform/pdf/PdfiumDocument.hpp"

#include <memory>
#include <mutex>
#include <stop_token>
#include <utility>
#include <vector>

namespace phvikapen::platform::pdf {

PdfRenderer::PdfRenderer()
    : m_thread{[this](const std::stop_token& stopToken) { run(stopToken); }} {}

PdfRenderer::~PdfRenderer() {
    m_thread.request_stop();
    m_thread.join();
}

void PdfRenderer::open(core::Asset asset, DocumentHandler onOpened) {
    post([this, asset = std::move(asset), onOpened = std::move(onOpened)] mutable {
        if (!m_documents.contains(asset.id)) {
            core::Result<std::unique_ptr<PdfiumDocument>> document =
                PdfiumDocument::openBytes(std::move(asset.data));
            if (!document) {
                onOpened(std::unexpected{document.error()});
                return;
            }
            m_documents.emplace(asset.id, std::move(*document));
        }

        const IPdfDocument& document = *m_documents.at(asset.id);
        std::vector<PageSize> sizes;
        sizes.reserve(static_cast<std::size_t>(document.pageCount()));
        for (int index = 0; index < document.pageCount(); ++index) {
            core::Result<PageSize> size = document.pageSize(index);
            if (!size) {
                onOpened(std::unexpected{size.error()});
                return;
            }
            sizes.push_back(*size);
        }
        onOpened(std::move(sizes));
    });
}

void PdfRenderer::render(const core::ContentId& asset, int pageIndex, int widthInPixels,
                         int heightInPixels, ImageHandler onRendered) {
    post([this, asset, pageIndex, widthInPixels, heightInPixels,
          onRendered = std::move(onRendered)] {
        const auto found = m_documents.find(asset);
        if (found == m_documents.end()) {
            onRendered(core::makeError(core::ErrorCode::NotFound, "the document is not open"));
            return;
        }
        onRendered(found->second->renderPage(pageIndex, widthInPixels, heightInPixels));
    });
}

void PdfRenderer::forget(const core::ContentId& asset) {
    post([this, asset] { m_documents.erase(asset); });
}

void PdfRenderer::waitUntilIdle() {
    std::unique_lock lock{m_mutex};
    m_idle.wait(lock, [this] { return m_tasks.empty() && !m_busy; });
}

void PdfRenderer::post(Task task) {
    {
        const std::scoped_lock lock{m_mutex};
        m_tasks.push_back(std::move(task));
    }
    m_queued.notify_one();
}

void PdfRenderer::run(const std::stop_token& stopToken) {
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
    m_documents.clear();
}

}
