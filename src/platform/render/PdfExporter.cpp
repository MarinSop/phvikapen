#include "platform/render/PdfExporter.hpp"

#include "core/Error.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/ContentId.hpp"
#include "core/model/Asset.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/storage/NotebookStore.hpp"
#include "platform/pdf/IPdfDocument.hpp"
#include "platform/pdf/PdfiumDocument.hpp"
#include "platform/render/PagePainter.hpp"

#include <QByteArray>
#include <QImage>
#include <QMarginsF>
#include <QPageSize>
#include <QPagedPaintDevice>
#include <QPainter>
#include <QPdfWriter>
#include <QSizeF>
#include <QString>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace phvikapen::platform::render {
namespace {

constexpr int kResolution = 96;
constexpr double kPointsPerPageUnit = 72.0 / 96.0;
constexpr float kMediaScale = 2.0F;
constexpr int kMaximumMediaPixels = 4000;

[[nodiscard]] QString toQString(const std::filesystem::path& path) {
    return QString::fromStdU16String(path.u16string());
}

[[nodiscard]] QImage toImage(const pdf::PageImage& image) {
    const QImage view{image.pixels.data(), image.width, image.height,
                      static_cast<qsizetype>(image.width) * 4, QImage::Format_RGBA8888};
    return view.copy();
}

[[nodiscard]] QByteArray toByteArray(const std::vector<std::byte>& bytes) {
    QByteArray copy;
    copy.resize(static_cast<qsizetype>(bytes.size()));
    std::ranges::transform(bytes, copy.begin(),
                           [](std::byte value) { return static_cast<char>(value); });
    return copy;
}

class MediaCache {
public:
    [[nodiscard]] QImage imageFor(const core::NotebookStore& store, const core::PageMedia& media,
                                  const core::Rect& area) {
        const core::Result<core::Asset> asset = store.asset(media.asset);
        if (!asset) {
            return {};
        }
        if (asset->kind == core::AssetKind::Image) {
            QImage picture;
            return picture.loadFromData(toByteArray(asset->data)) ? picture : QImage{};
        }
        return pdfPage(*asset, media.index, area);
    }

private:
    [[nodiscard]] QImage pdfPage(const core::Asset& asset, int index, const core::Rect& area) {
        auto found = m_documents.find(asset.id);
        if (found == m_documents.end()) {
            core::Result<std::unique_ptr<pdf::PdfiumDocument>> document =
                pdf::PdfiumDocument::openBytes(asset.data);
            if (!document) {
                return {};
            }
            found = m_documents.emplace(asset.id, std::move(*document)).first;
        }

        const int width =
            std::min(static_cast<int>(area.width() * kMediaScale), kMaximumMediaPixels);
        const int height =
            std::min(static_cast<int>(area.height() * kMediaScale), kMaximumMediaPixels);
        const core::Result<pdf::PageImage> page = found->second->renderPage(index, width, height);
        return page ? toImage(*page) : QImage{};
    }

    std::map<core::ContentId, std::unique_ptr<pdf::PdfiumDocument>> m_documents;
};

[[nodiscard]] std::vector<core::PageInfo> pagesOf(const core::NotebookOutline& outline,
                                                  ExportScope scope) {
    std::vector<core::PageInfo> pages;
    for (const core::SectionInfo& section : outline.sections) {
        for (const core::PageInfo& page : section.pages) {
            if (scope != ExportScope::Document || page.media) {
                pages.push_back(page);
            }
        }
    }
    return pages;
}

[[nodiscard]] core::Rect areaFor(const PageContents& contents, ExportScope scope) {
    core::Rect area = pageArea(contents);
    if (scope != ExportScope::Everything) {
        return area;
    }
    for (const core::PlacedStroke& placed : contents.strokes) {
        if (const std::optional<core::Rect> bounds = placed.stroke.boundingBox()) {
            area = area.united(*bounds);
        }
    }
    return area;
}

void setPageSize(QPdfWriter& writer, const core::Rect& area) {
    const QSizeF points{area.width() * kPointsPerPageUnit, area.height() * kPointsPerPageUnit};
    writer.setPageSize(QPageSize{points, QPageSize::Point});
    writer.setPageMargins(QMarginsF{});
}

}

core::Result<int> exportNotebookToPdf(const std::filesystem::path& notebook,
                                      const std::filesystem::path& target, ExportOptions options,
                                      const ProgressHandler& onProgress) {
    core::Result<core::NotebookStore> store = core::NotebookStore::open(notebook);
    if (!store) {
        return std::unexpected{store.error()};
    }

    const core::Result<core::NotebookOutline> outline = store->readOutline();
    if (!outline) {
        return std::unexpected{outline.error()};
    }

    const std::vector<core::PageInfo> pages = pagesOf(*outline, options.scope);
    if (pages.empty()) {
        return core::makeError(core::ErrorCode::InvalidArgument,
                               "the notebook has no pages to write");
    }

    QPdfWriter writer{toQString(target)};
    writer.setResolution(kResolution);
    writer.setTitle(QString::fromStdString(outline->title));

    MediaCache media;
    std::optional<QPainter> painter;
    int written = 0;

    for (const core::PageInfo& info : pages) {
        const core::Result<std::vector<core::PlacedStroke>> strokes = store->strokesOfPage(info.id);
        if (!strokes) {
            return std::unexpected{strokes.error()};
        }

        const PageContents contents{
            .style = info.style,
            .strokes = *strokes,
            .media = nullptr,
        };
        const core::Rect area = areaFor(contents, options.scope);
        const QImage picture = info.media ? media.imageFor(*store, *info.media, area) : QImage{};
        const PageContents page{
            .style = info.style,
            .strokes = *strokes,
            .media = picture.isNull() ? nullptr : &picture,
        };

        setPageSize(writer, area);
        if (!painter) {
            painter.emplace();
            if (!painter->begin(&writer)) {
                return core::makeError(core::ErrorCode::IoFailure,
                                       "the document could not be written");
            }
        } else if (!writer.newPage()) {
            return core::makeError(core::ErrorCode::IoFailure, "the next page could not be added");
        }

        paintPage(*painter, page, area);
        ++written;
        if (onProgress) {
            onProgress(
                ExportProgress{.page = written, .pageCount = static_cast<int>(pages.size())});
        }
    }

    if (painter) {
        painter->end();
    }
    return written;
}

}
