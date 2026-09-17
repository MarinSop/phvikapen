#include "platform/pdf/PdfRenderer.hpp"

#include "core/Error.hpp"
#include "core/id/ContentId.hpp"
#include "core/model/Asset.hpp"
#include "platform/pdf/IPdfDocument.hpp"

#include <gtest/gtest.h>

#include <QColor>
#include <QPainter>
#include <QPdfWriter>
#include <QRect>
#include <QString>
#include <QTemporaryDir>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace phvikapen::platform::pdf {
namespace {

constexpr int kResolution = 150;

[[nodiscard]] core::Asset writeAsset(const QTemporaryDir& directory) {
    const QString path = directory.filePath("lecture.pdf");
    {
        QPdfWriter writer{path};
        writer.setResolution(kResolution);
        writer.setPageSize(QPageSize{QPageSize::A5});
        QPainter painter{&writer};
        painter.fillRect(QRect{0, 0, writer.width() / 2, writer.height() / 2}, QColor{Qt::black});
        writer.newPage();
        painter.fillRect(QRect{0, 0, writer.width(), writer.height()}, QColor{Qt::white});
        painter.end();
    }

    std::ifstream file{std::filesystem::path{path.toStdU16String()}, std::ios::binary};
    const std::string contents{std::istreambuf_iterator<char>{file},
                               std::istreambuf_iterator<char>{}};
    std::vector<std::byte> data(contents.size());
    std::ranges::transform(contents, data.begin(),
                           [](char value) { return static_cast<std::byte>(value); });
    return core::Asset{
        .id = core::ContentId{core::ContentId::Bytes{1, 2, 3}},
        .kind = core::AssetKind::Pdf,
        .name = "lecture.pdf",
        .data = std::move(data),
    };
}

[[nodiscard]] bool hasDarkPixel(const PageImage& image, int fromX, int fromY, int toX, int toY) {
    for (int y = fromY; y < toY; ++y) {
        for (int x = fromX; x < toX; ++x) {
            const std::size_t offset =
                ((static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width))
                 + static_cast<std::size_t>(x))
                * 4U;
            if (image.pixels[offset] < 64) {
                return true;
            }
        }
    }
    return false;
}

TEST(PdfRendererTest, OpensADocumentAndReportsItsPages) {
    const QTemporaryDir directory;
    const core::Asset asset = writeAsset(directory);
    PdfRenderer renderer;
    std::optional<core::Result<std::vector<PageSize>>> pages;

    renderer.open(
        asset, [&pages](core::Result<std::vector<PageSize>> sizes) { pages = std::move(sizes); });
    renderer.waitUntilIdle();

    ASSERT_TRUE(pages.has_value());
    ASSERT_TRUE(pages->has_value()) << pages->error().message;
    ASSERT_EQ((*pages)->size(), 2U);
    EXPECT_NEAR((*pages)->front().width, 559.4F, 1.0F);
    EXPECT_NEAR((*pages)->front().height, 793.7F, 1.0F);
}

TEST(PdfRendererTest, DrawsAPageOfAnOpenDocument) {
    const QTemporaryDir directory;
    const core::Asset asset = writeAsset(directory);
    PdfRenderer renderer;
    std::optional<core::Result<PageImage>> image;

    renderer.open(asset, [](core::Result<std::vector<PageSize>>) {});
    renderer.render(asset.id, 0, 100, 142,
                    [&image](core::Result<PageImage> rendered) { image = std::move(rendered); });
    renderer.waitUntilIdle();

    ASSERT_TRUE(image.has_value());
    ASSERT_TRUE(image->has_value()) << image->error().message;
    EXPECT_EQ((*image)->width, 100);
    EXPECT_EQ((*image)->pixels.size(), 100U * 142U * 4U);
    EXPECT_TRUE(hasDarkPixel(**image, 20, 20, 45, 60));
    EXPECT_FALSE(hasDarkPixel(**image, 70, 100, 100, 142));
}

TEST(PdfRendererTest, ReportsADocumentItWasNotGiven) {
    PdfRenderer renderer;
    std::optional<core::Result<PageImage>> image;

    renderer.render(core::ContentId{core::ContentId::Bytes{8}}, 0, 10, 10,
                    [&image](core::Result<PageImage> rendered) { image = std::move(rendered); });
    renderer.waitUntilIdle();

    ASSERT_TRUE(image.has_value());
    ASSERT_FALSE(image->has_value());
    EXPECT_EQ(image->error().code, core::ErrorCode::NotFound);
}

TEST(PdfRendererTest, ForgetsADocumentWhenAsked) {
    const QTemporaryDir directory;
    const core::Asset asset = writeAsset(directory);
    PdfRenderer renderer;
    std::optional<core::Result<PageImage>> image;

    renderer.open(asset, [](core::Result<std::vector<PageSize>>) {});
    renderer.forget(asset.id);
    renderer.render(asset.id, 0, 10, 10,
                    [&image](core::Result<PageImage> rendered) { image = std::move(rendered); });
    renderer.waitUntilIdle();

    ASSERT_TRUE(image.has_value());
    EXPECT_EQ(image->error().code, core::ErrorCode::NotFound);
}

}
}
