#include "core/ink/StrokeSelection.hpp"

#include "core/geometry/Distance.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/model/Page.hpp"

#include <gtest/gtest.h>

#include <array>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr std::array kSquare{
    Point{.x = 0.0F, .y = 0.0F},
    Point{.x = 100.0F, .y = 0.0F},
    Point{.x = 100.0F, .y = 100.0F},
    Point{.x = 0.0F, .y = 100.0F},
};

[[nodiscard]] Stroke line(Uuid7Generator& ids, float fromX, float toX, float y) {
    Stroke stroke{ids.next(), StrokeStyle{.width = 2.0F}};
    for (int step = 0; step <= 10; ++step) {
        const float along = fromX + ((toX - fromX) * static_cast<float>(step) / 10.0F);
        stroke.append(InkSample{.x = along, .y = y});
    }
    return stroke;
}

TEST(StrokeSelectionTest, KnowsWhatIsInsideAShape) {
    EXPECT_TRUE(inside(kSquare, Point{.x = 50.0F, .y = 50.0F}));
    EXPECT_FALSE(inside(kSquare, Point{.x = 150.0F, .y = 50.0F}));
    EXPECT_FALSE(inside(kSquare, Point{.x = 50.0F, .y = -1.0F}));
}

TEST(StrokeSelectionTest, ALineIsNotAShape) {
    const std::array line{Point{.x = 0.0F, .y = 0.0F}, Point{.x = 10.0F, .y = 10.0F}};

    EXPECT_FALSE(inside(line, Point{.x = 5.0F, .y = 5.0F}));
}

TEST(StrokeSelectionTest, PicksOnlyStrokesThatAreWhollyInside) {
    Uuid7Generator ids;
    const Stroke within = line(ids, 20.0F, 80.0F, 50.0F);
    const Stroke across = line(ids, 20.0F, 180.0F, 60.0F);
    const Stroke away = line(ids, 200.0F, 280.0F, 70.0F);
    Page page{ids.next()};
    ASSERT_TRUE(page.insert({.ordinal = 0, .stroke = within}).has_value());
    ASSERT_TRUE(page.insert({.ordinal = 1, .stroke = across}).has_value());
    ASSERT_TRUE(page.insert({.ordinal = 2, .stroke = away}).has_value());

    const std::vector<Uuid> picked = strokesInside(page, kSquare);

    ASSERT_EQ(picked.size(), 1U);
    EXPECT_EQ(picked.front(), within.id());
}

TEST(StrokeSelectionTest, TheBoundsOfWhatWasPickedHoldEveryStroke) {
    Uuid7Generator ids;
    const Stroke first = line(ids, 10.0F, 40.0F, 20.0F);
    const Stroke second = line(ids, 50.0F, 90.0F, 80.0F);
    Page page{ids.next()};
    ASSERT_TRUE(page.insert({.ordinal = 0, .stroke = first}).has_value());
    ASSERT_TRUE(page.insert({.ordinal = 1, .stroke = second}).has_value());
    const std::array picked{first.id(), second.id()};

    const std::optional<Rect> bounds = boundsOf(page, picked);

    ASSERT_TRUE(bounds.has_value());
    EXPECT_LE(bounds->left, 10.0F);
    EXPECT_GE(bounds->right, 90.0F);
    EXPECT_LE(bounds->top, 20.0F);
    EXPECT_GE(bounds->bottom, 80.0F);
}

TEST(StrokeSelectionTest, MovingAStrokeShiftsEverySampleAndKeepsItsName) {
    Uuid7Generator ids;
    const Stroke original = line(ids, 10.0F, 40.0F, 20.0F);

    const Stroke shifted = moved(original, 5.0F, -3.0F);

    EXPECT_EQ(shifted.id(), original.id());
    ASSERT_EQ(shifted.samples().size(), original.samples().size());
    EXPECT_FLOAT_EQ(shifted.samples().front().x, original.samples().front().x + 5.0F);
    EXPECT_FLOAT_EQ(shifted.samples().front().y, original.samples().front().y - 3.0F);
    EXPECT_FLOAT_EQ(shifted.samples().back().x, original.samples().back().x + 5.0F);
}

}
}
