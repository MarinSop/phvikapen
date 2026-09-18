#include "core/ink/StrokeShapes.hpp"

#include "core/geometry/Rect.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <optional>

namespace phvikapen::core {
namespace {

[[nodiscard]] Stroke wobble(Uuid7Generator& ids) {
    Stroke stroke{ids.next(), StrokeStyle{.width = 3.0F}};
    for (int i = 0; i <= 20; ++i) {
        const auto step = static_cast<float>(i);
        stroke.append(InkSample{
            .x = 10.0F + (step * 5.0F),
            .y = 40.0F + (std::sin(step) * 4.0F),
            .pressure = 0.5F,
        });
    }
    return stroke;
}

TEST(StrokeShapesTest, FreehandIsLeftAlone) {
    Uuid7Generator ids;
    const Stroke drawn = wobble(ids);

    const Stroke kept = shaped(drawn, Shape::Freehand);

    EXPECT_EQ(kept.samples().size(), drawn.samples().size());
    EXPECT_EQ(kept.id(), drawn.id());
}

TEST(StrokeShapesTest, ALineRunsStraightFromStartToEnd) {
    Uuid7Generator ids;
    const Stroke drawn = wobble(ids);
    const InkSample first = drawn.samples().front();
    const InkSample last = drawn.samples().back();

    const Stroke line = shaped(drawn, Shape::Line);

    ASSERT_FALSE(line.samples().empty());
    EXPECT_EQ(line.id(), drawn.id());
    EXPECT_EQ(line.style(), drawn.style());
    EXPECT_NEAR(line.samples().front().x, first.x, 0.01F);
    EXPECT_NEAR(line.samples().front().y, first.y, 0.01F);
    EXPECT_NEAR(line.samples().back().x, last.x, 0.01F);
    EXPECT_NEAR(line.samples().back().y, last.y, 0.01F);
    for (const InkSample& sample : line.samples()) {
        const float part = (sample.x - first.x) / (last.x - first.x);
        EXPECT_NEAR(sample.y, first.y + ((last.y - first.y) * part), 0.01F);
    }
}

TEST(StrokeShapesTest, ABoxHasTheCornersOfWhatWasDrawn) {
    Uuid7Generator ids;
    const Stroke drawn = wobble(ids);
    const std::optional<Rect> bounds = drawn.boundingBox();
    ASSERT_TRUE(bounds.has_value());

    const Stroke box = shaped(drawn, Shape::Rectangle);

    const std::optional<Rect> made = box.boundingBox();
    ASSERT_TRUE(made.has_value());
    EXPECT_NEAR(made->left, bounds->left, 0.01F);
    EXPECT_NEAR(made->right, bounds->right, 0.01F);
    EXPECT_NEAR(made->top, bounds->top, 0.01F);
    EXPECT_NEAR(made->bottom, bounds->bottom, 0.01F);
    EXPECT_NEAR(box.samples().front().x, box.samples().back().x, 0.01F);
    EXPECT_NEAR(box.samples().front().y, box.samples().back().y, 0.01F);
}

TEST(StrokeShapesTest, AnOvalFillsWhatWasDrawnAndClosesItself) {
    Uuid7Generator ids;
    const Stroke drawn = wobble(ids);
    const std::optional<Rect> bounds = drawn.boundingBox();
    ASSERT_TRUE(bounds.has_value());

    const Stroke oval = shaped(drawn, Shape::Ellipse);

    const std::optional<Rect> made = oval.boundingBox();
    ASSERT_TRUE(made.has_value());
    EXPECT_NEAR(made->width(), bounds->width(), 0.5F);
    EXPECT_NEAR(made->height(), bounds->height(), 0.5F);
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
