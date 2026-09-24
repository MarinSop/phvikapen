#include "platform/render/PagePainter.hpp"

#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/model/TextBox.hpp"
#include "platform/render/PaperLook.hpp"

#include <gtest/gtest.h>

#include <QColor>
#include <QImage>
#include <QPainter>

#include <array>
#include <span>
#include <utility>
#include <vector>

namespace phvikapen::platform::render {
namespace {

[[nodiscard]] QImage paint(const PageContents& page, const core::Rect& area) {
    QImage sheet{static_cast<int>(area.width()), static_cast<int>(area.height()),
                 QImage::Format_ARGB32_Premultiplied};
    sheet.fill(Qt::transparent);
    QPainter painter{&sheet};
    paintPage(painter, page, area);
    painter.end();
    return sheet;
}

[[nodiscard]] bool hasColorNear(const QImage& sheet, const QColor& wanted) {
    for (int y = 0; y < sheet.height(); ++y) {
        for (int x = 0; x < sheet.width(); ++x) {
            const QColor found = sheet.pixelColor(x, y);
            if (std::abs(found.red() - wanted.red()) < 16
                && std::abs(found.green() - wanted.green()) < 16
                && std::abs(found.blue() - wanted.blue()) < 16) {
                return true;
            }
        }
    }
    return false;
}

constexpr core::Color kBlack{};
constexpr std::span<const core::PlacedStroke> kNoStrokes{};

[[nodiscard]] core::PlacedStroke line(core::Color color, float width, float fromX, float toX,
                                      float y) {
    core::Stroke stroke{core::Uuid{}, core::StrokeStyle{.color = color, .width = width}};
    for (int step = 0; static_cast<float>(step) * 4.0F <= toX - fromX; ++step) {
        stroke.append(core::InkSample{.x = fromX + (static_cast<float>(step) * 4.0F), .y = y});
    }
    return core::PlacedStroke{.ordinal = 1, .stroke = std::move(stroke)};
}

TEST(PagePainterTest, AFixedPageIsAsLargeAsItsPaper) {
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5},
        .strokes = kNoStrokes,
        .texts = {},
    };

    const core::Rect area = pageArea(page);

    EXPECT_FLOAT_EQ(area.left, 0.0F);
    EXPECT_FLOAT_EQ(area.top, 0.0F);
    EXPECT_NEAR(area.width(), 559.4F, 1.0F);
    EXPECT_NEAR(area.height(), 793.7F, 1.0F);
}

TEST(PagePainterTest, AnInfinitePageIsAsLargeAsWhatWasWrittenOnIt) {
    const std::vector<core::PlacedStroke> strokes{
        line(kBlack, 2.0F, 100.0F, 200.0F, 300.0F),
    };
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::Infinite},
        .strokes = strokes,
        .texts = {},
    };

    const core::Rect area = pageArea(page);

    EXPECT_LT(area.left, 100.0F);
    EXPECT_GT(area.right, 200.0F);
    EXPECT_LT(area.top, 300.0F);
    EXPECT_GT(area.bottom, 300.0F);
}

TEST(PagePainterTest, AnEmptyInfinitePageFallsBackToASheetOfPaper) {
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::Infinite},
        .strokes = kNoStrokes,
        .texts = {},
    };

    const core::Rect area = pageArea(page);

    EXPECT_GT(area.width(), 0.0F);
    EXPECT_GT(area.height(), 0.0F);
}

TEST(PagePainterTest, TheSheetIsWhiteAndTheInkIsWhereItWasWritten) {
    const std::vector<core::PlacedStroke> strokes{
        line(core::Color{.red = 200, .green = 30, .blue = 30}, 6.0F, 20.0F, 180.0F, 100.0F),
    };
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = strokes,
        .texts = {},
    };

    const QImage sheet = paint(page, core::Rect{.right = 200.0F, .bottom = 200.0F});

    EXPECT_EQ(sheet.pixelColor(190, 190), QColor(Qt::white));
    EXPECT_EQ(sheet.pixelColor(100, 100), QColor(200, 30, 30));
    EXPECT_EQ(sheet.pixelColor(100, 160), QColor(Qt::white));
}

TEST(PagePainterTest, RuledPaperGetsItsLinesAndItsMargin) {
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Lined},
        .strokes = kNoStrokes,
        .texts = {},
    };

    const QImage sheet = paint(page, pageArea(page));

    EXPECT_TRUE(hasColorNear(sheet, QColor::fromRgbF(kRuleColor[0], kRuleColor[1], kRuleColor[2])));
    EXPECT_TRUE(
        hasColorNear(sheet, QColor::fromRgbF(kMarginColor[0], kMarginColor[1], kMarginColor[2])));
}

TEST(PagePainterTest, BlankPaperStaysBlank) {
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = kNoStrokes,
        .texts = {},
    };

    const QImage sheet = paint(page, pageArea(page));

    EXPECT_FALSE(
        hasColorNear(sheet, QColor::fromRgbF(kRuleColor[0], kRuleColor[1], kRuleColor[2])));
}

[[nodiscard]] core::PlacedText typed(const char* what, float x, float y, core::Color color) {
    return core::PlacedText{
        .ordinal = 0,
        .box =
            core::TextBox{
                .id = core::Uuid{},
                .at = core::Point{.x = x, .y = y},
                .width = 160.0F,
                .height = 40.0F,
                .text = what,
                .style =
                    core::TextStyle{
                        .font = {},
                        .size = 24.0F,
                        .color = color,
                    },
            },
    };
}

TEST(PagePainterTest, TypedTextIsPrintedWhereItSits) {
    const std::vector<core::PlacedText> texts{
        typed("Hello", 20.0F, 20.0F, core::Color{.red = 220, .green = 20, .blue = 40}),
    };
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = kNoStrokes,
        .texts = texts,
    };

    const QImage sheet = paint(page, core::Rect{.right = 200.0F, .bottom = 200.0F});

    EXPECT_TRUE(hasColorNear(sheet, QColor{220, 20, 40}));
    EXPECT_FALSE(hasColorNear(sheet.copy(0, 120, 200, 80), QColor{220, 20, 40}));
}

TEST(PagePainterTest, APageWithoutTypedTextPrintsNone) {
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = kNoStrokes,
        .texts = {},
    };

    const QImage sheet = paint(page, core::Rect{.right = 200.0F, .bottom = 200.0F});

    EXPECT_FALSE(hasColorNear(sheet, QColor{220, 20, 40}));
}

TEST(PagePainterTest, AnImportedPageIsDrawnUnderTheInk) {
    QImage media{10, 10, QImage::Format_ARGB32_Premultiplied};
    media.fill(QColor{0, 0, 255});
    const std::vector<core::PlacedStroke> strokes{
        line(core::Color{.green = 255}, 8.0F, 10.0F, 190.0F, 60.0F),
    };
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = strokes,
        .texts = {},
        .media = &media,
    };

    const QImage sheet = paint(page, core::Rect{.right = 200.0F, .bottom = 200.0F});

    EXPECT_EQ(sheet.pixelColor(150, 150), QColor(0, 0, 255));
    EXPECT_EQ(sheet.pixelColor(100, 60), QColor(0, 255, 0));
}

}
}
