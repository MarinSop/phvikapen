#include "platform/pdf/PdfiumDocument.hpp"

#include "core/Error.hpp"
#include "platform/pdf/IPdfDocument.hpp"

#include <gtest/gtest.h>

#include <QColor>
#include <QPainter>
#include <QPdfWriter>
#include <QRect>
#include <QString>
#include <QTemporaryDir>

#include <cstddef>
#include <memory>
#include <vector>

namespace phvikapen::platform::pdf {
namespace {

constexpr int kResolution = 300;
constexpr int kPageCount = 2;

[[nodiscard]] QString writePdf(const QTemporaryDir& directory) {
    const QString path = directory.filePath("notes.pdf");
    QPdfWriter writer{path};
    writer.setResolution(kResolution);
    writer.setPageSize(QPageSize{QPageSize::A4});

    QPainter painter{&writer};
    const int halfWidth = writer.width() / 2;
    const int halfHeight = writer.height() / 2;
    painter.fillRect(QRect{0, 0, halfWidth, halfHeight}, QColor{Qt::black});
    writer.newPage();
    painter.fillRect(QRect{halfWidth, halfHeight, halfWidth, halfHeight}, QColor{Qt::black});
    painter.end();
    return path;
}

[[nodiscard]] bool hasDarkPixel(const PageImage& image, int fromX, int fromY, int toX, int toY) {
    for (int y = fromY; y < toY; ++y) {
        for (int x = fromX; x < toX; ++x) {
            const std::size_t offset =
                ((static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width))
                 + static_cast<std::size_t>(x))
                * 4U;
            if (image.pixels[offset] < 64 && image.pixels[offset + 1] < 64
                && image.pixels[offset + 2] < 64) {
                return true;
            }
        }
    }
    return false;
}

TEST(PdfiumDocumentTest, ReadsThePagesOfAFile) {
    const QTemporaryDir directory;
    const QString path = writePdf(directory);

    const core::Result<std::unique_ptr<PdfiumDocument>> document =
        PdfiumDocument::openFile(path.toStdU16String());

    ASSERT_TRUE(document.has_value()) << document.error().message;
    EXPECT_EQ((*document)->pageCount(), kPageCount);
    const core::Result<PageSize> size = (*document)->pageSize(0);
    ASSERT_TRUE(size.has_value()) << size.error().message;
    EXPECT_NEAR(size->width, 793.7F, 1.0F);
    EXPECT_NEAR(size->height, 1122.5F, 1.0F);
}

TEST(PdfiumDocumentTest, DrawsAPageAtTheWantedSize) {
    const QTemporaryDir directory;
    const QString path = writePdf(directory);
    const core::Result<std::unique_ptr<PdfiumDocument>> document =
        PdfiumDocument::openFile(path.toStdU16String());
    ASSERT_TRUE(document.has_value()) << document.error().message;

    const core::Result<PageImage> image = (*document)->renderPage(0, 200, 283);

    ASSERT_TRUE(image.has_value()) << image.error().message;
    EXPECT_EQ(image->width, 200);
    EXPECT_EQ(image->height, 283);
    EXPECT_EQ(image->pixels.size(), 200U * 283U * 4U);
    EXPECT_TRUE(hasDarkPixel(*image, 10, 10, 80, 100));
    EXPECT_FALSE(hasDarkPixel(*image, 150, 200, 200, 283));
}

TEST(PdfiumDocumentTest, EachPageHasItsOwnContent) {
    const QTemporaryDir directory;
    const QString path = writePdf(directory);
    const core::Result<std::unique_ptr<PdfiumDocument>> document =
        PdfiumDocument::openFile(path.toStdU16String());
    ASSERT_TRUE(document.has_value()) << document.error().message;

    const core::Result<PageImage> second = (*document)->renderPage(1, 200, 283);

    ASSERT_TRUE(second.has_value()) << second.error().message;
    EXPECT_FALSE(hasDarkPixel(*second, 10, 10, 80, 100));
    EXPECT_TRUE(hasDarkPixel(*second, 120, 160, 190, 270));
}

TEST(PdfiumDocumentTest, RefusesWhatIsNotAPdf) {
    std::vector<std::byte> nonsense(64, std::byte{7});

    const core::Result<std::unique_ptr<PdfiumDocument>> document =
        PdfiumDocument::openBytes(std::move(nonsense));

    ASSERT_FALSE(document.has_value());
    EXPECT_EQ(document.error().code, core::ErrorCode::InvalidArgument);
}

TEST(PdfiumDocumentTest, ReportsPagesAndSizesItCannotDraw) {
    const QTemporaryDir directory;
    const QString path = writePdf(directory);
    const core::Result<std::unique_ptr<PdfiumDocument>> document =
        PdfiumDocument::openFile(path.toStdU16String());
    ASSERT_TRUE(document.has_value()) << document.error().message;

    EXPECT_EQ((*document)->pageSize(kPageCount).error().code, core::ErrorCode::NotFound);
    EXPECT_EQ((*document)->renderPage(kPageCount, 100, 100).error().code,
              core::ErrorCode::NotFound);
    EXPECT_EQ((*document)->renderPage(0, 0, 100).error().code, core::ErrorCode::InvalidArgument);
    EXPECT_EQ((*document)->renderPage(0, 100000, 100).error().code,
              core::ErrorCode::InvalidArgument);
}

TEST(PdfiumDocumentTest, DrawsOnlyThePartOfThePageThatWasAskedFor) {
    const QTemporaryDir directory;
    const QString path = writePdf(directory);
    const core::Result<std::unique_ptr<PdfiumDocument>> document =
        PdfiumDocument::openFile(path.toStdU16String());
    ASSERT_TRUE(document.has_value()) << document.error().message;
    const core::Result<PageSize> size = (*document)->pageSize(0);
    ASSERT_TRUE(size.has_value());

    const core::Result<PageImage> corner =
        (*document)->renderRegion(0, 100, 100,
                                  PageRegion{
                                      .left = 0.0F,
                                      .top = 0.0F,
                                      .width = size->width / 4.0F,
                                      .height = size->height / 4.0F,
                                  });
    const core::Result<PageImage> away =
        (*document)->renderRegion(0, 100, 100,
                                  PageRegion{
                                      .left = size->width * 0.7F,
                                      .top = size->height * 0.7F,
                                      .width = size->width / 4.0F,
                                      .height = size->height / 4.0F,
                                  });

    ASSERT_TRUE(corner.has_value()) << corner.error().message;
    ASSERT_TRUE(away.has_value()) << away.error().message;
    const std::size_t middle = (((std::size_t{50} * 100U) + 50U) * 4U);
    EXPECT_LT(corner->pixels[middle], 64);
    EXPECT_GT(away->pixels[middle], 192);
}

TEST(PdfiumDocumentTest, RefusesAnEmptyPartOfAPage) {
    const QTemporaryDir directory;
    const QString path = writePdf(directory);
    const core::Result<std::unique_ptr<PdfiumDocument>> document =
        PdfiumDocument::openFile(path.toStdU16String());
    ASSERT_TRUE(document.has_value()) << document.error().message;

    const core::Result<PageImage> nothing = (*document)->renderRegion(0, 10, 10, PageRegion{});

    ASSERT_FALSE(nothing.has_value());
    EXPECT_EQ(nothing.error().code, core::ErrorCode::InvalidArgument);
}

}
}
