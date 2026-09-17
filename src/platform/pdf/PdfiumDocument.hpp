#pragma once

#include "core/Error.hpp"
#include "platform/pdf/IPdfDocument.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>

namespace phvikapen::platform::pdf {

class PdfiumLibrary;

class PdfiumDocument final : public IPdfDocument {
public:
    using Handle = void*;

    [[nodiscard]] static core::Result<std::unique_ptr<PdfiumDocument>>
    openFile(const std::filesystem::path& path);

    [[nodiscard]] static core::Result<std::unique_ptr<PdfiumDocument>>
    openBytes(std::vector<std::byte> bytes);

    PdfiumDocument(std::shared_ptr<PdfiumLibrary> library, Handle document,
                   std::vector<std::byte> bytes) noexcept;

    ~PdfiumDocument() override;

    PdfiumDocument(const PdfiumDocument&) = delete;
    PdfiumDocument& operator=(const PdfiumDocument&) = delete;
    PdfiumDocument(PdfiumDocument&&) = delete;
    PdfiumDocument& operator=(PdfiumDocument&&) = delete;

    [[nodiscard]] int pageCount() const noexcept override;

    [[nodiscard]] core::Result<PageSize> pageSize(int pageIndex) const override;

    [[nodiscard]] core::Result<PageImage> renderPage(int pageIndex, int widthInPixels,
                                                     int heightInPixels) const override;

private:
    std::shared_ptr<PdfiumLibrary> m_library;
    std::vector<std::byte> m_bytes;
    Handle m_document{nullptr};
};

}
