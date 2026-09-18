#include "platform/pdf/PdfiumDocument.hpp"

#include "core/Error.hpp"
#include "platform/pdf/IPdfDocument.hpp"

#include <fpdf_doc.h>
#include <fpdfview.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace phvikapen::platform::pdf {
namespace {

constexpr float kPageUnitsPerPoint = 96.0F / 72.0F;
constexpr int kBytesPerPixel = 4;
constexpr int kMaximumPixels = 8192;
constexpr unsigned long kWhite = 0xFFFFFFFF;

[[nodiscard]] std::string_view describe(unsigned long code) {
    switch (code) {
    case FPDF_ERR_FILE:
        return "the file could not be opened";
    case FPDF_ERR_FORMAT:
        return "the file is not a PDF";
    case FPDF_ERR_PASSWORD:
        return "the PDF needs a password";
    case FPDF_ERR_SECURITY:
        return "the PDF uses an unsupported security scheme";
    default:
        return "the PDF could not be read";
    }
}

}

class PdfiumLibrary {
public:
    [[nodiscard]] static std::shared_ptr<PdfiumLibrary> instance() {
        static std::mutex guard;
        static std::weak_ptr<PdfiumLibrary> shared;

        const std::scoped_lock lock{guard};
        std::shared_ptr<PdfiumLibrary> library = shared.lock();
        if (!library) {
            library = std::make_shared<PdfiumLibrary>();
            shared = library;
        }
        return library;
    }

    PdfiumLibrary() {
        FPDF_LIBRARY_CONFIG config{};
        config.version = 2;
        FPDF_InitLibraryWithConfig(&config);
    }

    ~PdfiumLibrary() { FPDF_DestroyLibrary(); }

    PdfiumLibrary(const PdfiumLibrary&) = delete;
    PdfiumLibrary& operator=(const PdfiumLibrary&) = delete;
    PdfiumLibrary(PdfiumLibrary&&) = delete;
    PdfiumLibrary& operator=(PdfiumLibrary&&) = delete;
};

PdfiumDocument::PdfiumDocument(std::shared_ptr<PdfiumLibrary> library, Handle document,
                               std::vector<std::byte> bytes) noexcept
    : m_library{std::move(library)}, m_bytes{std::move(bytes)}, m_document{document} {}

PdfiumDocument::~PdfiumDocument() {
    if (m_document != nullptr) {
        FPDF_CloseDocument(static_cast<FPDF_DOCUMENT>(m_document));
    }
}

core::Result<std::unique_ptr<PdfiumDocument>>
PdfiumDocument::openBytes(std::vector<std::byte> bytes) {
    if (bytes.empty()) {
        return core::makeError(core::ErrorCode::InvalidArgument, "the PDF is empty");
    }
    std::shared_ptr<PdfiumLibrary> library = PdfiumLibrary::instance();
    FPDF_DOCUMENT document =
        FPDF_LoadMemDocument64(bytes.data(), static_cast<size_t>(bytes.size()), nullptr);
    if (document == nullptr) {
        return core::makeError(core::ErrorCode::InvalidArgument,
                               std::string{describe(FPDF_GetLastError())});
    }
    return std::make_unique<PdfiumDocument>(std::move(library), document, std::move(bytes));
}

core::Result<std::unique_ptr<PdfiumDocument>>
PdfiumDocument::openFile(const std::filesystem::path& path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        return core::makeError(core::ErrorCode::IoFailure, "the PDF could not be opened");
    }
    const std::string contents{std::istreambuf_iterator<char>{file},
                               std::istreambuf_iterator<char>{}};
    if (file.bad()) {
        return core::makeError(core::ErrorCode::IoFailure, "the PDF could not be read");
    }

    std::vector<std::byte> bytes(contents.size());
    std::ranges::transform(contents, bytes.begin(),
                           [](char value) { return static_cast<std::byte>(value); });
    return openBytes(std::move(bytes));
}

int PdfiumDocument::pageCount() const noexcept {
    return FPDF_GetPageCount(static_cast<FPDF_DOCUMENT>(m_document));
}

core::Result<PageSize> PdfiumDocument::pageSize(int pageIndex) const {
    FS_SIZEF size{};
    if (pageIndex < 0 || pageIndex >= pageCount()
        || FPDF_GetPageSizeByIndexF(static_cast<FPDF_DOCUMENT>(m_document), pageIndex, &size)
               == 0) {
        return core::makeError(core::ErrorCode::NotFound, "the PDF has no such page");
    }
    return PageSize{
        .width = size.width * kPageUnitsPerPoint,
        .height = size.height * kPageUnitsPerPoint,
    };
}

core::Result<PageImage> PdfiumDocument::renderPage(int pageIndex, int widthInPixels,
                                                   int heightInPixels) const {
    const core::Result<PageSize> size = pageSize(pageIndex);
    if (!size) {
        return std::unexpected{size.error()};
    }
    return renderRegion(pageIndex, widthInPixels, heightInPixels,
                        PageRegion{
                            .left = 0.0F,
                            .top = 0.0F,
                            .width = size->width,
                            .height = size->height,
                        });
}

core::Result<PageImage> PdfiumDocument::renderRegion(int pageIndex, int widthInPixels,
                                                     int heightInPixels,
                                                     const PageRegion& region) const {
    if (widthInPixels <= 0 || heightInPixels <= 0 || widthInPixels > kMaximumPixels
        || heightInPixels > kMaximumPixels) {
        return core::makeError(core::ErrorCode::InvalidArgument,
                               "the wanted size is not one a page can be drawn at");
    }
    if (region.width <= 0.0F || region.height <= 0.0F) {
        return core::makeError(core::ErrorCode::InvalidArgument,
                               "the wanted part of the page is empty");
    }
    if (pageIndex < 0 || pageIndex >= pageCount()) {
        return core::makeError(core::ErrorCode::NotFound, "the PDF has no such page");
    }
    const core::Result<PageSize> size = pageSize(pageIndex);
    if (!size) {
        return std::unexpected{size.error()};
    }

    FPDF_PAGE page = FPDF_LoadPage(static_cast<FPDF_DOCUMENT>(m_document), pageIndex);
    if (page == nullptr) {
        return core::makeError(core::ErrorCode::IoFailure, "the page could not be read");
    }

    PageImage image{
        .width = widthInPixels,
        .height = heightInPixels,
        .pixels = std::vector<std::uint8_t>(static_cast<std::size_t>(widthInPixels)
                                                * static_cast<std::size_t>(heightInPixels)
                                                * static_cast<std::size_t>(kBytesPerPixel),
                                            0),
    };
    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(widthInPixels, heightInPixels, FPDFBitmap_BGRA,
                                             image.pixels.data(), widthInPixels * kBytesPerPixel);
    if (bitmap == nullptr) {
        FPDF_ClosePage(page);
        return core::makeError(core::ErrorCode::IoFailure, "the page could not be drawn");
    }

    const float across = static_cast<float>(widthInPixels) / region.width;
    const float down = static_cast<float>(heightInPixels) / region.height;
    FPDFBitmap_FillRect(bitmap, 0, 0, widthInPixels, heightInPixels, kWhite);
    FPDF_RenderPageBitmap(bitmap, page, static_cast<int>(std::lround(-region.left * across)),
                          static_cast<int>(std::lround(-region.top * down)),
                          static_cast<int>(std::lround(size->width * across)),
                          static_cast<int>(std::lround(size->height * down)), 0, FPDF_ANNOT);
    FPDFBitmap_Destroy(bitmap);
    FPDF_ClosePage(page);

    for (std::size_t i = 0; i + 3 < image.pixels.size(); i += kBytesPerPixel) {
        std::swap(image.pixels[i], image.pixels[i + 2]);
    }
    return image;
}

}
