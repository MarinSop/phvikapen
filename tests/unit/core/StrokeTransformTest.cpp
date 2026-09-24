#include "core/ink/StrokeTransform.hpp"

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr float kClose = 0.001F;

[[nodiscard]] Stroke box(Uuid7Generator& ids, float left, float top, float right, float bottom) {
    Stroke stroke{ids.next(), StrokeStyle{.width = 4.0F}};
    stroke.append(InkSample{.x = left, .y = top});
    stroke.append(InkSample{.x = right, .y = top});
    stroke.append(InkSample{.x = right, .y = bottom});
    stroke.append(InkSample{.x = left, .y = bottom});
    return stroke;
}

TEST(StrokeTransformTest, NothingAskedForChangesNothing) {
    EXPECT_TRUE(leavesAsItWas(Transform{}));
    EXPECT_FALSE(leavesAsItWas(Transform{.dx = 1.0F}));
    EXPECT_FALSE(leavesAsItWas(Transform{.wide = 2.0F}));
    EXPECT_FALSE(leavesAsItWas(Transform{.turn = 90.0F}));
}

TEST(StrokeTransformTest, ThePivotStaysWhereItIs) {
    const Transform turned{
        .pivot = Point{.x = 10.0F, .y = 20.0F},
        .wide = 3.0F,
        .tall = 0.5F,
        .turn = 37.0F,
    };

    const Point still = placed(turned, turned.pivot);

    EXPECT_NEAR(still.x, turned.pivot.x, kClose);
    EXPECT_NEAR(still.y, turned.pivot.y, kClose);
}

TEST(StrokeTransformTest, AQuarterTurnPutsTheRightSideUnderneath) {
    const Transform quarter{.pivot = Point{.x = 0.0F, .y = 0.0F}, .turn = 90.0F};

    const Point moved = placed(quarter, Point{.x = 10.0F, .y = 0.0F});

    EXPECT_NEAR(moved.x, 0.0F, kClose);
    EXPECT_NEAR(moved.y, 10.0F, kClose);
}

TEST(StrokeTransformTest, FourQuarterTurnsComeBackToWhereItStarted) {
    const Transform quarter{.pivot = Point{.x = 5.0F, .y = 7.0F}, .turn = 90.0F};
    Point walked{.x = 30.0F, .y = 11.0F};

    for (int turn = 0; turn < 4; ++turn) {
        walked = placed(quarter, walked);
    }

    EXPECT_NEAR(walked.x, 30.0F, kClose);
    EXPECT_NEAR(walked.y, 11.0F, kClose);
}

TEST(StrokeTransformTest, MovingCarriesEveryPointTheSameWay) {
    Uuid7Generator ids;
    const Stroke drawn = box(ids, 0.0F, 0.0F, 10.0F, 10.0F);
    const Transform across{.dx = 4.0F, .dy = -3.0F};

    const Stroke moved = transformed(drawn, across);

    ASSERT_EQ(moved.samples().size(), drawn.samples().size());
    for (std::size_t at = 0; at < moved.samples().size(); ++at) {
        EXPECT_NEAR(moved.samples()[at].x, drawn.samples()[at].x + 4.0F, kClose);
        EXPECT_NEAR(moved.samples()[at].y, drawn.samples()[at].y - 3.0F, kClose);
    }
}

TEST(StrokeTransformTest, AStrokeKeepsWhoItIsAndHowHardItWasPressed) {
    Uuid7Generator ids;
    Stroke drawn{ids.next(), StrokeStyle{.width = 4.0F}};
    drawn.append(InkSample{.x = 0.0F, .y = 0.0F, .pressure = 0.25F});
    drawn.append(InkSample{.x = 8.0F, .y = 0.0F, .pressure = 0.75F});

    const Stroke moved = transformed(drawn, Transform{.dx = 5.0F});

    EXPECT_EQ(moved.id(), drawn.id());
    EXPECT_FLOAT_EQ(moved.samples().front().pressure, 0.25F);
    EXPECT_FLOAT_EQ(moved.samples().back().pressure, 0.75F);
}

TEST(StrokeTransformTest, MakingADrawingLargerMakesItsLinesThicker) {
    Uuid7Generator ids;
    const Stroke drawn = box(ids, 0.0F, 0.0F, 10.0F, 10.0F);

    const Stroke larger = transformed(drawn, Transform{.wide = 2.0F, .tall = 2.0F});

    EXPECT_FLOAT_EQ(larger.style().width, drawn.style().width * 2.0F);
}

TEST(StrokeTransformTest, StretchingOneWayOnlyThickensTheLineByWhatBothComeTo) {
    Uuid7Generator ids;
    const Stroke drawn = box(ids, 0.0F, 0.0F, 10.0F, 10.0F);

    const Stroke stretched = transformed(drawn, Transform{.wide = 4.0F, .tall = 1.0F});

    EXPECT_NEAR(stretched.style().width, drawn.style().width * 2.0F, kClose);
}

TEST(StrokeTransformTest, SizingToABoxLandsOnThatBox) {
    const Rect from{.left = 10.0F, .top = 10.0F, .right = 30.0F, .bottom = 20.0F};
    const Rect to{.left = 50.0F, .top = 5.0F, .right = 110.0F, .bottom = 35.0F};

    const Rect landed = placed(sizingTo(from, to), from);

    EXPECT_NEAR(landed.left, to.left, kClose);
    EXPECT_NEAR(landed.top, to.top, kClose);
    EXPECT_NEAR(landed.right, to.right, kClose);
    EXPECT_NEAR(landed.bottom, to.bottom, kClose);
}

TEST(StrokeTransformTest, ABoxTurnedAQuarterKeepsItsSizeTheOtherWayAround) {
    const Rect upright{.left = 0.0F, .top = 0.0F, .right = 40.0F, .bottom = 10.0F};

    const Rect turned = placed(Transform{.turn = 90.0F}, upright);

    EXPECT_NEAR(turned.width(), upright.height(), kClose);
    EXPECT_NEAR(turned.height(), upright.width(), kClose);
}

TEST(StrokeTransformTest, NothingIsEverSizedAwayToNothing) {
    const Transform flat = normalized(Transform{.wide = 0.0F, .tall = 0.0F});

    EXPECT_GT(std::abs(flat.wide), 0.0F);
    EXPECT_GT(std::abs(flat.tall), 0.0F);
}

TEST(StrokeTransformTest, TurningRoundAndRoundIsKeptWithinOneTurn) {
    const Transform wound = normalized(Transform{.turn = 725.0F});

    EXPECT_NEAR(wound.turn, 5.0F, kClose);
}

}
}
