#include "platform/render/PdfExporter.hpp"

#include "core/Error.hpp"
#include "core/id/ContentId.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/model/Asset.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/storage/NotebookStore.hpp"
#include "platform/pdf/IPdfDocument.hpp"
#include "platform/pdf/PdfiumDocument.hpp"

#include <gtest/gtest.h>

#include <QString>
#include <QTemporaryDir>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace phvikapen::platform::render {
namespace {

[[nodiscard]] std::filesystem::path pathIn(const QTemporaryDir& directory, const QString& name) {
    return std::filesystem::path{directory.filePath(name).toStdU16String()};
}

[[nodiscard]] core::Stroke diagonal(core::Uuid7Generator& ids, float from, float to) {
    core::Stroke stroke{ids.next(), core::StrokeStyle{.width = 8.0F}};
    for (int step = 0; step <= 20; ++step) {
        const float along = from + ((to - from) * static_cast<float>(step) / 20.0F);
        stroke.append(core::InkSample{.x = along, .y = along});
    }
    return stroke;
}

constexpr core::PageStyle kUsual{};
constexpr core::PageStyle kA5{.paper = core::Paper::A5};
constexpr core::PageStyle kA5Blank{.paper = core::Paper::A5, .background = core::Background::Blank};
constexpr core::PageStyle kA4Landscape{
    .paper = core::Paper::A4,
    .orientation = core::Orientation::Landscape,
};

struct Notebook {
    core::Uuid section;
    std::vector<core::PageInfo> pages;
};

Notebook writeNotebook(const std::filesystem::path& path,
                       const std::vector<core::PageStyle>& styles) {
    core::Uuid7Generator ids;
    core::Result<core::NotebookStore> store = core::NotebookStore::open(path);
    EXPECT_TRUE(store.has_value()) << store.error().message;

    const core::NotebookOutline initial = store->readOutline().value();
    Notebook written{.section = initial.sections.front().id, .pages = {}};
    std::vector<core::Uuid> order{initial.sections.front().pages.front().id};

    for (std::size_t i = 0; i < styles.size(); ++i) {
        core::PageInfo page{
            .id = ids.next(),
            .title = "Page " + std::to_string(i + 2),
            .style = styles[i],
            .media = std::nullopt,
        };
        order.push_back(page.id);
        EXPECT_TRUE(store->insertPage(written.section, page, order).has_value());
        EXPECT_TRUE(store
                        ->insertStroke(page.id,
                                       core::PlacedStroke{
                                           .ordinal = 1,
                                           .stroke = diagonal(ids, 40.0F, 200.0F),
                                       })
                        .has_value());
        written.pages.push_back(std::move(page));
    }
    return written;
}

TEST(PdfExporterTest, WritesOnePdfPagePerNotebookPage) {
    const QTemporaryDir directory;
    const std::filesystem::path notebook = pathIn(directory, "Lecture.phvika");
    const std::filesystem::path target = pathIn(directory, "Lecture.pdf");
    writeNotebook(notebook, {kA5});

    const core::Result<int> written = exportNotebookToPdf(notebook, target);

    ASSERT_TRUE(written.has_value()) << written.error().message;
    EXPECT_EQ(*written, 2);
    const core::Result<std::unique_ptr<pdf::PdfiumDocument>> document =
        pdf::PdfiumDocument::openFile(target);
    ASSERT_TRUE(document.has_value()) << document.error().message;
    EXPECT_EQ((*document)->pageCount(), 2);
}

TEST(PdfExporterTest, EveryPageKeepsTheSizeItWasWrittenOn) {
    const QTemporaryDir directory;
    const std::filesystem::path notebook = pathIn(directory, "Sizes.phvika");
    const std::filesystem::path target = pathIn(directory, "Sizes.pdf");
    writeNotebook(notebook, {kA5, kA4Landscape});

    ASSERT_TRUE(exportNotebookToPdf(notebook, target).has_value());

    const core::Result<std::unique_ptr<pdf::PdfiumDocument>> document =
        pdf::PdfiumDocument::openFile(target);
    ASSERT_TRUE(document.has_value()) << document.error().message;
    ASSERT_EQ((*document)->pageCount(), 3);
    const core::Result<pdf::PageSize> a5 = (*document)->pageSize(1);
    const core::Result<pdf::PageSize> a4 = (*document)->pageSize(2);
    ASSERT_TRUE(a5.has_value());
    ASSERT_TRUE(a4.has_value());
    EXPECT_NEAR(a5->width, 559.4F, 2.0F);
    EXPECT_NEAR(a5->height, 793.7F, 2.0F);
    EXPECT_NEAR(a4->width, 1122.5F, 2.0F);
    EXPECT_NEAR(a4->height, 793.7F, 2.0F);
}

TEST(PdfExporterTest, TheInkIsOnThePage) {
    const QTemporaryDir directory;
    const std::filesystem::path notebook = pathIn(directory, "Ink.phvika");
    const std::filesystem::path target = pathIn(directory, "Ink.pdf");
    writeNotebook(notebook, {kA5Blank});

    ASSERT_TRUE(exportNotebookToPdf(notebook, target).has_value());

    const core::Result<std::unique_ptr<pdf::PdfiumDocument>> document =
        pdf::PdfiumDocument::openFile(target);
    ASSERT_TRUE(document.has_value()) << document.error().message;
    const core::Result<pdf::PageImage> page = (*document)->renderPage(1, 280, 397);
    ASSERT_TRUE(page.has_value()) << page.error().message;

    const auto darkAt = [&page](int x, int y) {
        const std::size_t offset =
            ((static_cast<std::size_t>(y) * static_cast<std::size_t>(page->width))
             + static_cast<std::size_t>(x))
            * 4U;
        return page->pixels[offset] < 128;
    };
    EXPECT_TRUE(darkAt(50, 50));
    EXPECT_FALSE(darkAt(200, 50));
}

TEST(PdfExporterTest, ReportsAPlaceItCannotWriteTo) {
    const QTemporaryDir directory;
    const std::filesystem::path notebook = pathIn(directory, "Closed.phvika");
    writeNotebook(notebook, {kUsual});

    const core::Result<int> written =
        exportNotebookToPdf(notebook, pathIn(directory, "missing/Closed.pdf"));

    ASSERT_FALSE(written.has_value());
    EXPECT_EQ(written.error().code, core::ErrorCode::IoFailure);
}

TEST(PdfExporterTest, TellsHowFarItGot) {
    const QTemporaryDir directory;
    const std::filesystem::path notebook = pathIn(directory, "Progress.phvika");
    writeNotebook(notebook, {kUsual, kUsual});
    std::vector<int> seen;

    ASSERT_TRUE(exportNotebookToPdf(notebook, pathIn(directory, "Progress.pdf"), ExportOptions{},
                                    [&seen](ExportProgress progress) {
                                        EXPECT_EQ(progress.pageCount, 3);
                                        seen.push_back(progress.page);
                                    })
                    .has_value());

    EXPECT_EQ(seen, (std::vector<int>{1, 2, 3}));
}

TEST(PdfExporterTest, InkBesideTheSheetIsTakenInOrLeftOutAsAsked) {
    const QTemporaryDir directory;
    const std::filesystem::path notebook = pathIn(directory, "Beside.phvika");
    const Notebook written = writeNotebook(notebook, {kA5});
    {
        core::Uuid7Generator ids;
        core::Result<core::NotebookStore> store = core::NotebookStore::open(notebook);
        ASSERT_TRUE(store.has_value()) << store.error().message;
        ASSERT_TRUE(store
                        ->insertStroke(written.pages.front().id,
                                       core::PlacedStroke{
                                           .ordinal = 2,
                                           .stroke = diagonal(ids, 700.0F, 900.0F),
                                       })
                        .has_value());
    }

    const std::filesystem::path wide = pathIn(directory, "Wide.pdf");
    const std::filesystem::path sheet = pathIn(directory, "Sheet.pdf");
    ASSERT_TRUE(exportNotebookToPdf(notebook, wide, ExportOptions{.scope = ExportScope::Everything})
                    .has_value());
    ASSERT_TRUE(exportNotebookToPdf(notebook, sheet, ExportOptions{.scope = ExportScope::Sheet})
                    .has_value());

    const core::Result<std::unique_ptr<pdf::PdfiumDocument>> wider =
        pdf::PdfiumDocument::openFile(wide);
    const core::Result<std::unique_ptr<pdf::PdfiumDocument>> plain =
        pdf::PdfiumDocument::openFile(sheet);
    ASSERT_TRUE(wider.has_value()) << wider.error().message;
    ASSERT_TRUE(plain.has_value()) << plain.error().message;
    const core::Result<pdf::PageSize> grown = (*wider)->pageSize(1);
    const core::Result<pdf::PageSize> kept = (*plain)->pageSize(1);
    ASSERT_TRUE(grown.has_value());
    ASSERT_TRUE(kept.has_value());
    EXPECT_GT(grown->width, kept->width);
    EXPECT_NEAR(kept->width, 559.4F, 2.0F);
}

TEST(PdfExporterTest, OnlyTheImportedPagesAreWrittenWhenAskedFor) {
    const QTemporaryDir directory;
    const std::filesystem::path notebook = pathIn(directory, "Mixed.phvika");
    const Notebook written = writeNotebook(notebook, {kA5, kA5Blank});
    {
        core::Result<core::NotebookStore> store = core::NotebookStore::open(notebook);
        ASSERT_TRUE(store.has_value()) << store.error().message;
        core::ContentId::Bytes bytes{};
        bytes.front() = 7;
        const core::Asset asset{
            .id = core::ContentId{bytes},
            .kind = core::AssetKind::Image,
            .name = "picture.png",
            .data = {std::byte{0x89}, std::byte{0x50}},
        };
        ASSERT_TRUE(store->insertAsset(asset).has_value());
        const core::Result<void> attached = store->setPageMedia(
            written.pages.back().id, core::PageMedia{.asset = asset.id, .index = 0});
        ASSERT_TRUE(attached.has_value()) << attached.error().message;
    }

    const std::filesystem::path only = pathIn(directory, "Imported.pdf");
    const core::Result<int> pages =
        exportNotebookToPdf(notebook, only, ExportOptions{.scope = ExportScope::Document});
    ASSERT_TRUE(pages.has_value()) << pages.error().message;
    EXPECT_EQ(*pages, 1);
}

}
}
