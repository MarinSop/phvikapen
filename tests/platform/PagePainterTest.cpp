#include "platform/render/PagePainter.hpp"

#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/model/Layer.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/model/Picture.hpp"
#include "core/model/Table.hpp"
#include "core/model/TextBox.hpp"
#include "platform/render/PaperLook.hpp"

#include <gtest/gtest.h>

#include <QColor>
#include <QImage>
#include <QPainter>

#include <array>
#include <span>
#include <string>
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

[[nodiscard]] bool hasColorIn(const QImage& sheet, const QColor& wanted, const core::Rect& where) {
    const int fromX = std::max(0, static_cast<int>(where.left));
    const int fromY = std::max(0, static_cast<int>(where.top));
    const int toX = std::min(sheet.width(), static_cast<int>(where.right));
    const int toY = std::min(sheet.height(), static_cast<int>(where.bottom));
    for (int y = fromY; y < toY; ++y) {
        for (int x = fromX; x < toX; ++x) {
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

[[nodiscard]] bool hasColorNear(const QImage& sheet, const QColor& wanted) {
    return hasColorIn(sheet, wanted,
                      core::Rect{
                          .right = static_cast<float>(sheet.width()),
                          .bottom = static_cast<float>(sheet.height()),
                      });
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
        .pictures = {},
        .tables = {},
        .layers = {},
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
        .pictures = {},
        .tables = {},
        .layers = {},
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
        .pictures = {},
        .tables = {},
        .layers = {},
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
        .pictures = {},
        .tables = {},
        .layers = {},
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
        .pictures = {},
        .tables = {},
        .layers = {},
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
        .pictures = {},
        .tables = {},
        .layers = {},
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
        .pictures = {},
        .tables = {},
        .layers = {},
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
        .pictures = {},
        .tables = {},
        .layers = {},
    };

    const QImage sheet = paint(page, core::Rect{.right = 200.0F, .bottom = 200.0F});

    EXPECT_FALSE(hasColorNear(sheet, QColor{220, 20, 40}));
}

TEST(PagePainterTest, APictureIsDrawnOverThePaperAndUnderTheInk) {
    QImage red{8, 8, QImage::Format_ARGB32_Premultiplied};
    red.fill(QColor{255, 0, 0});
    const std::vector<core::PlacedStroke> strokes{
        line(core::Color{.green = 255}, 8.0F, 10.0F, 190.0F, 60.0F),
    };
    const std::array pictures{
        DrawnPicture{
            .placed =
                core::Picture{
                    .id = {},
                    .source = {},
                    .at = core::Point{.x = 20.0F, .y = 100.0F},
                    .width = 80.0F,
                    .height = 60.0F,
                    .turn = 0.0F,
                },
            .picture = &red,
        },
    };
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = strokes,
        .texts = {},
        .pictures = pictures,
        .tables = {},
        .layers = {},
    };

    const QImage sheet = paint(page, core::Rect{.right = 200.0F, .bottom = 200.0F});

    EXPECT_EQ(sheet.pixelColor(60, 130), QColor(255, 0, 0));
    EXPECT_EQ(sheet.pixelColor(100, 60), QColor(0, 255, 0));
}

[[nodiscard]] core::Layer layerNamed(std::uint8_t mark, const std::string& name, bool shown) {
    core::Uuid::Bytes bytes{};
    bytes.front() = mark;
    return core::Layer{.id = core::Uuid{bytes}, .name = name, .shown = shown, .locked = false};
}

TEST(PagePainterTest, ALayerAboveAnotherIsDrawnOverIt) {
    QImage red{8, 8, QImage::Format_ARGB32_Premultiplied};
    red.fill(QColor{255, 0, 0});
    const std::array layers{
        layerNamed(1, "Under", true),
        layerNamed(2, "Over", true),
    };
    const std::vector<core::PlacedStroke> strokes{
        line(core::Color{.green = 255}, 8.0F, 8.0F, 190.0F, 130.0F),
    };
    const std::array pictures{
        DrawnPicture{
            .placed =
                core::Picture{
                    .id = {},
                    .source = {},
                    .at = core::Point{.x = 20.0F, .y = 100.0F},
                    .width = 80.0F,
                    .height = 60.0F,
                    .turn = 0.0F,
                },
            .picture = &red,
            .layer = layers[1].id,
        },
    };
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = strokes,
        .texts = {},
        .pictures = pictures,
        .tables = {},
        .layers = layers,
    };

    const QImage sheet = paint(page, core::Rect{.right = 200.0F, .bottom = 200.0F});

    // The ink runs right through where the picture stands, and the picture is on the layer above,
    // so the picture covers it there and nowhere else.
    EXPECT_EQ(sheet.pixelColor(60, 130), QColor(255, 0, 0));
    EXPECT_EQ(sheet.pixelColor(150, 130), QColor(0, 255, 0));
}

TEST(PagePainterTest, ALayerBelowAnotherIsDrawnUnderIt) {
    QImage red{8, 8, QImage::Format_ARGB32_Premultiplied};
    red.fill(QColor{255, 0, 0});
    const std::array layers{
        layerNamed(1, "Under", true),
        layerNamed(2, "Over", true),
    };
    const std::vector<core::PlacedStroke> strokes{
        core::PlacedStroke{
            .ordinal = 0,
            .stroke = line(core::Color{.green = 255}, 8.0F, 8.0F, 190.0F, 130.0F).stroke,
            .layer = layers[1].id,
        },
    };
    const std::array pictures{
        DrawnPicture{
            .placed =
                core::Picture{
                    .id = {},
                    .source = {},
                    .at = core::Point{.x = 20.0F, .y = 100.0F},
                    .width = 80.0F,
                    .height = 60.0F,
                    .turn = 0.0F,
                },
            .picture = &red,
            .layer = layers[0].id,
        },
    };
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = strokes,
        .texts = {},
        .pictures = pictures,
        .tables = {},
        .layers = layers,
    };

    const QImage sheet = paint(page, core::Rect{.right = 200.0F, .bottom = 200.0F});

    // The same two things, the other way round: now the ink is drawn over the picture.
    EXPECT_EQ(sheet.pixelColor(60, 130), QColor(0, 255, 0));
}

TEST(PagePainterTest, NothingOnALayerThatIsNotShownIsDrawn) {
    QImage red{8, 8, QImage::Format_ARGB32_Premultiplied};
    red.fill(QColor{255, 0, 0});
    const std::array layers{layerNamed(1, "Hidden", false)};
    const std::array pictures{
        DrawnPicture{
            .placed =
                core::Picture{
                    .id = {},
                    .source = {},
                    .at = core::Point{.x = 20.0F, .y = 100.0F},
                    .width = 80.0F,
                    .height = 60.0F,
                    .turn = 0.0F,
                },
            .picture = &red,
            .layer = layers.front().id,
        },
    };
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = {},
        .texts = {},
        .pictures = pictures,
        .tables = {},
        .layers = layers,
    };

    const QImage sheet = paint(page, core::Rect{.right = 200.0F, .bottom = 200.0F});

    EXPECT_FALSE(hasColorNear(sheet, QColor{255, 0, 0}));
}

TEST(PagePainterTest, APageWithoutPicturesPrintsNone) {
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = {},
        .texts = {},
        .pictures = {},
        .tables = {},
        .layers = {},
    };

    const QImage sheet = paint(page, core::Rect{.right = 200.0F, .bottom = 200.0F});

    EXPECT_FALSE(hasColorNear(sheet, QColor{255, 0, 0}));
}

TEST(PagePainterTest, ATurnedPictureLeavesTheCornersOfItsUprightBoxBare) {
    QImage red{8, 8, QImage::Format_ARGB32_Premultiplied};
    red.fill(QColor{255, 0, 0});
    const std::array upright{
        DrawnPicture{
            .placed =
                core::Picture{
                    .id = {},
                    .source = {},
                    .at = core::Point{.x = 40.0F, .y = 40.0F},
                    .width = 100.0F,
                    .height = 100.0F,
                    .turn = 0.0F,
                },
            .picture = &red,
        },
    };
    std::array turned = upright;
    turned.front().placed.turn = 45.0F;
    const core::Rect area{.right = 200.0F, .bottom = 200.0F};
    const core::PageStyle style{.paper = core::Paper::A5, .background = core::Background::Blank};

    const QImage flat = paint(
        PageContents{
            .style = style,
            .strokes = {},
            .texts = {},
            .pictures = upright,
            .tables = {},
            .layers = {},
        },
        area);
    const QImage spun = paint(
        PageContents{
            .style = style,
            .strokes = {},
            .texts = {},
            .pictures = turned,
            .tables = {},
            .layers = {},
        },
        area);

    EXPECT_EQ(flat.pixelColor(45, 45), QColor(255, 0, 0));
    EXPECT_NE(spun.pixelColor(45, 45), QColor(255, 0, 0));
    EXPECT_EQ(spun.pixelColor(90, 90), QColor(255, 0, 0));
}

[[nodiscard]] core::PlacedTable ruled(core::Color rule, const std::string& words) {
    core::Table table = core::gridOf(2, 2);
    table.at = core::Point{.x = 20.0F, .y = 20.0F};
    table.columns = {60.0F, 60.0F};
    table.rows = {40.0F, 40.0F};
    table.rule = rule;
    table.ruleWidth = 3.0F;
    table.style.size = 12.0F;
    return core::PlacedTable{
        .ordinal = 0,
        .table = core::withCellWritten(std::move(table), core::CellAt{.row = 0, .column = 1}, words)
                     .value(),
    };
}

TEST(PagePainterTest, ATableIsRuledInItsOwnColourAndItsBoxesAreLeftEmpty) {
    const std::array tables{ruled(core::Color{.red = 255}, "")};
    const core::Rect area{.right = 200.0F, .bottom = 200.0F};
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = kNoStrokes,
        .texts = {},
        .pictures = {},
        .tables = tables,
        .layers = {},
    };

    const QImage sheet = paint(page, area);

    const core::Rect box =
        core::areaOfCell(tables.front().table, core::CellAt{.row = 0, .column = 0});
    EXPECT_TRUE(hasColorIn(sheet, QColor(255, 0, 0), core::areaOf(tables.front().table)));
    EXPECT_FALSE(hasColorIn(sheet, QColor(255, 0, 0), box.inflated(-4.0F)));
}

TEST(PagePainterTest, WordsTypedIntoABoxAreDrawnInThatBoxAndNoOther) {
    const std::array tables{ruled(core::Color{.red = 255}, "IIII")};
    const core::Rect area{.right = 200.0F, .bottom = 200.0F};
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = kNoStrokes,
        .texts = {},
        .pictures = {},
        .tables = tables,
        .layers = {},
    };

    const QImage sheet = paint(page, area);

    const core::Table& table = tables.front().table;
    const core::Rect written = core::areaOfCell(table, core::CellAt{.row = 0, .column = 1});
    const core::Rect blank = core::areaOfCell(table, core::CellAt{.row = 1, .column = 0});
    EXPECT_TRUE(hasColorIn(sheet, QColor(0, 0, 0), written.inflated(-2.0F)));
    EXPECT_FALSE(hasColorIn(sheet, QColor(0, 0, 0), blank.inflated(-2.0F)));
}

TEST(PagePainterTest, ABoxGivenAColourIsFilledWithItAndTheRestAreLeftClear) {
    std::array tables{ruled(core::Color{.red = 255}, "")};
    const core::Color amber{.red = 255, .green = 200, .blue = 0, .alpha = core::Color::kOpaque};
    tables.front().table =
        core::withRangeFilled(tables.front().table, core::CellAt{.row = 0, .column = 0},
                              core::CellAt{.row = 0, .column = 1}, amber)
            .value();
    const core::Rect area{.right = 200.0F, .bottom = 200.0F};
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = kNoStrokes,
        .texts = {},
        .pictures = {},
        .tables = tables,
        .layers = {},
    };

    const QImage sheet = paint(page, area);

    const core::Table& table = tables.front().table;
    const core::Rect filled = core::areaOfCell(table, core::CellAt{.row = 0, .column = 0});
    const core::Rect clear = core::areaOfCell(table, core::CellAt{.row = 1, .column = 0});
    EXPECT_TRUE(hasColorIn(sheet, QColor(255, 200, 0), filled.inflated(-4.0F)));
    EXPECT_FALSE(hasColorIn(sheet, QColor(255, 200, 0), clear.inflated(-4.0F)));
    // The ruling is drawn over what fills a box rather than under it.
    EXPECT_TRUE(hasColorIn(sheet, QColor(255, 0, 0), core::areaOf(table)));
}

TEST(PagePainterTest, WordsAskedToSitAtTheFootOfABoxAreDrawnThere) {
    std::array tables{ruled(core::Color{.red = 255}, "IIII")};
    tables.front().table =
        core::withRangeRisen(tables.front().table, core::CellAt{.row = 0, .column = 1},
                             core::CellAt{.row = 0, .column = 1}, core::CellRise::Bottom)
            .value();
    const core::Rect area{.right = 200.0F, .bottom = 200.0F};
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = kNoStrokes,
        .texts = {},
        .pictures = {},
        .tables = tables,
        .layers = {},
    };

    const QImage sheet = paint(page, area);

    const core::Rect box =
        core::areaOfCell(tables.front().table, core::CellAt{.row = 0, .column = 1});
    const core::Rect upper{
        .left = box.left + 4.0F,
        .top = box.top + 4.0F,
        .right = box.right - 4.0F,
        .bottom = box.top + (box.height() / 2.0F),
    };
    const core::Rect lower{
        .left = box.left + 4.0F,
        .top = box.top + (box.height() / 2.0F),
        .right = box.right - 4.0F,
        .bottom = box.bottom - 4.0F,
    };
    EXPECT_TRUE(hasColorIn(sheet, QColor(0, 0, 0), lower));
    EXPECT_FALSE(hasColorIn(sheet, QColor(0, 0, 0), upper));
}

TEST(PagePainterTest, AJoinedBoxIsNeverRuledThroughTheMiddle) {
    std::array tables{ruled(core::Color{.red = 255}, "")};
    tables.front().table =
        core::withMergedRange(tables.front().table, core::CellAt{.row = 0, .column = 0},
                              core::CellAt{.row = 0, .column = 1})
            .value();
    const core::Rect area{.right = 200.0F, .bottom = 200.0F};
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = kNoStrokes,
        .texts = {},
        .pictures = {},
        .tables = tables,
        .layers = {},
    };

    const QImage sheet = paint(page, area);

    const core::Table& table = tables.front().table;
    const core::Rect joined = core::areaOfCell(table, core::CellAt{.row = 0, .column = 0});
    // The rule that stood between the two boxes is gone, but the one below the joined box stays.
    const core::Rect wasBetween{
        .left = joined.left + (joined.width() / 2) - 2.0F,
        .top = joined.top + 4.0F,
        .right = joined.left + (joined.width() / 2) + 2.0F,
        .bottom = joined.bottom - 4.0F,
    };
    EXPECT_FALSE(hasColorIn(sheet, QColor(255, 0, 0), wasBetween));
    EXPECT_TRUE(hasColorIn(sheet, QColor(255, 0, 0), joined));
}

TEST(PagePainterTest, ASumIsDrawnWithTheBarOfItsFraction) {
    const std::array texts{
        core::PlacedText{
            .ordinal = 0,
            .box =
                core::TextBox{
                    .id = {},
                    .at = core::Point{.x = 20.0F, .y = 20.0F},
                    .width = 120.0F,
                    .height = 60.0F,
                    .text = "1/2",
                    .style = core::TextStyle{.font = {}, .size = 24.0F},
                    .formula = true,
                },
        },
    };
    const core::Rect area{.right = 200.0F, .bottom = 200.0F};
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = kNoStrokes,
        .texts = texts,
        .pictures = {},
        .tables = {},
        .layers = {},
    };

    const QImage sheet = paint(page, area);

    const core::Drawing drawn = drawnFormula(texts.front().box.text, texts.front().box.style);
    ASSERT_EQ(drawn.bars.size(), 1U);
    // The bar of the fraction is filled in where it was laid out.
    const core::Rect bar = drawn.bars.front().area;
    EXPECT_TRUE(hasColorIn(sheet, QColor(0, 0, 0),
                           core::Rect{
                               .left = 20.0F + bar.left + 1.0F,
                               .top = 20.0F + bar.top,
                               .right = 20.0F + bar.right - 1.0F,
                               .bottom = 20.0F + bar.bottom + 1.0F,
                           }));
}

TEST(PagePainterTest, WhatIsNoSumIsWrittenOutAsItWasTyped) {
    const std::array texts{
        core::PlacedText{
            .ordinal = 0,
            .box =
                core::TextBox{
                    .id = {},
                    .at = core::Point{.x = 20.0F, .y = 20.0F},
                    .width = 160.0F,
                    .height = 40.0F,
                    .text = "1/2 +",
                    .style = core::TextStyle{.font = {}, .size = 24.0F},
                    .formula = true,
                },
        },
    };
    const core::Rect area{.right = 200.0F, .bottom = 200.0F};
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = kNoStrokes,
        .texts = texts,
        .pictures = {},
        .tables = {},
        .layers = {},
    };

    EXPECT_TRUE(drawnFormula(texts.front().box.text, texts.front().box.style).glyphs.empty());
    // Half a sum lays nothing out, so it is written out rather than left blank.
    EXPECT_TRUE(
        hasColorIn(paint(page, area), QColor(0, 0, 0),
                   core::Rect{.left = 20.0F, .top = 20.0F, .right = 180.0F, .bottom = 60.0F}));
}

TEST(PagePainterTest, HandwritingInsideABoxIsSeenThroughTheTable) {
    const std::array tables{ruled(core::Color{.red = 255}, "")};
    const std::vector<core::PlacedStroke> strokes{
        line(core::Color{.green = 255}, 8.0F, 30.0F, 70.0F, 40.0F),
    };
    const core::Rect area{.right = 200.0F, .bottom = 200.0F};
    const PageContents page{
        .style = core::PageStyle{.paper = core::Paper::A5, .background = core::Background::Blank},
        .strokes = strokes,
        .texts = {},
        .pictures = {},
        .tables = tables,
        .layers = {},
    };

    const QImage sheet = paint(page, area);

    const core::Rect box =
        core::areaOfCell(tables.front().table, core::CellAt{.row = 0, .column = 0});
    EXPECT_TRUE(hasColorIn(sheet, QColor(0, 255, 0), box.inflated(-4.0F)));
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
        .pictures = {},
        .tables = {},
        .layers = {},
        .media = &media,
    };

    const QImage sheet = paint(page, core::Rect{.right = 200.0F, .bottom = 200.0F});

    EXPECT_EQ(sheet.pixelColor(150, 150), QColor(0, 0, 255));
    EXPECT_EQ(sheet.pixelColor(100, 60), QColor(0, 255, 0));
}

}
}
