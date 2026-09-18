#include "core/ink/StrokeShapes.hpp"

#include "core/geometry/Rect.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <optional>

namespace phvikapen::core {
namespace {

[[nodiscard]] Stroke drag(Uuid7Generator& ids, float fromX, float fromY, float toX, float toY) {
    Stroke stroke{ids.next(), StrokeStyle{.width = 3.0F}};
    for (int i = 0; i <= 20; ++i) {
        const float part = static_cast<float>(i) / 20.0F;
        stroke.append(InkSample{
            .x = fromX + ((toX - fromX) * part),
            .y = fromY + ((toY - fromY) * part) + (std::sin(part * 10.0F) * 4.0F),
            .pressure = 0.5F,
        });
    }
    return stroke;
}

TEST(StrokeShapesTest, FreehandIsLeftAlone) {
    Uuid7Generator ids;
    const Stroke drawn = drag(ids, 10.0F, 10.0F, 110.0F, 60.0F);

    const Stroke kept = shaped(drawn, Shape::Freehand);

    EXPECT_EQ(kept.samples().size(), drawn.samples().size());
    EXPECT_EQ(kept.id(), drawn.id());
}

TEST(StrokeShapesTest, ALineRunsFromWhereTheStrokeStartedToWhereItEnded) {
    Uuid7Generator ids;
    const Stroke drawn = drag(ids, 10.0F, 20.0F, 110.0F, 70.0F);
    const InkSample first = drawn.samples().front();
    const InkSample last = drawn.samples().back();

    const Stroke line = shaped(drawn, Shape::Line);

    ASSERT_FALSE(line.samples().empty());
    EXPECT_NEAR(line.samples().front().x, first.x, 0.01F);
    EXPECT_NEAR(line.samples().front().y, first.y, 0.01F);
    EXPECT_NEAR(line.samples().back().x, last.x, 0.01F);
    EXPECT_NEAR(line.samples().back().y, last.y, 0.01F);
}

TEST(StrokeShapesTest, AnEvenLineSnapsToAnEighthOfATurn) {
    Uuid7Generator ids;
    const Stroke drawn = drag(ids, 0.0F, 0.0F, 100.0F, 10.0F);

    const Stroke line = shaped(drawn, Shape::Line, ShapeKeys{.even = true});

    const InkSample last = line.samples().back();
    EXPECT_NEAR(last.y, line.samples().front().y, 1.0F);
    EXPECT_GT(last.x, 90.0F);
}

TEST(StrokeShapesTest, ALineFromTheCentreReachesBothWays) {
    Uuid7Generator ids;
    const Stroke drawn = drag(ids, 50.0F, 50.0F, 90.0F, 50.0F);

    const Stroke line = shaped(drawn, Shape::Line, ShapeKeys{.fromCentre = true});

    EXPECT_NEAR(line.samples().front().x, 10.0F, 1.0F);
    EXPECT_NEAR(line.samples().back().x, 90.0F, 1.0F);
}

TEST(StrokeShapesTest, ABoxSpansFromTheStartOfTheStrokeToItsEnd) {
    Uuid7Generator ids;
    const Stroke drawn = drag(ids, 20.0F, 30.0F, 120.0F, 90.0F);

    const std::optional<Rect> box = shaped(drawn, Shape::Rectangle).boundingBox();

    ASSERT_TRUE(box.has_value());
    EXPECT_NEAR(box->width(), 100.0F, 5.0F);
    EXPECT_NEAR(box->height(), 60.0F, 8.0F);
}

TEST(StrokeShapesTest, AnEvenBoxIsASquare) {
    Uuid7Generator ids;
    const Stroke drawn = drag(ids, 0.0F, 0.0F, 100.0F, 40.0F);

    const std::optional<Rect> box =
        shaped(drawn, Shape::Rectangle, ShapeKeys{.even = true}).boundingBox();

    ASSERT_TRUE(box.has_value());
    EXPECT_NEAR(box->width(), box->height(), 1.0F);
}

TEST(StrokeShapesTest, ABoxFromTheCentreGrowsBothWays) {
    Uuid7Generator ids;
    const Stroke drawn = drag(ids, 100.0F, 100.0F, 140.0F, 130.0F);

    const std::optional<Rect> box =
        shaped(drawn, Shape::Rectangle, ShapeKeys{.fromCentre = true}).boundingBox();

    ASSERT_TRUE(box.has_value());
    EXPECT_NEAR((box->left + box->right) / 2.0F, 100.0F, 1.0F);
    EXPECT_NEAR(box->width(), 80.0F, 5.0F);
}

TEST(StrokeShapesTest, AnEvenCircleIsAsWideAsItIsTall) {
    Uuid7Generator ids;
    const Stroke drawn = drag(ids, 0.0F, 0.0F, 120.0F, 40.0F);

    const std::optional<Rect> box =
        shaped(drawn, Shape::Ellipse, ShapeKeys{.even = true}).boundingBox();

    ASSERT_TRUE(box.has_value());
    EXPECT_NEAR(box->width(), box->height(), 1.0F);
}

TEST(StrokeShapesTest, AnOvalClosesItself) {
    Uuid7Generator ids;
    const Stroke drawn = drag(ids, 10.0F, 10.0F, 110.0F, 60.0F);

    const Stroke oval = shaped(drawn, Shape::Ellipse);

    ASSERT_FALSE(oval.samples().empty());
    EXPECT_NEAR(oval.samples().front().x, oval.samples().back().x, 0.01F);
    EXPECT_NEAR(oval.samples().front().y, oval.samples().back().y, 0.01F);
}

TEST(StrokeShapesTest, ADotStaysADot) {
    Uuid7Generator ids;
    Stroke dot{ids.next(), StrokeStyle{}};
    dot.append(InkSample{.x = 5.0F, .y = 5.0F});

    EXPECT_EQ(shaped(dot, Shape::Rectangle).samples().size(), 1U);
}

}
}
