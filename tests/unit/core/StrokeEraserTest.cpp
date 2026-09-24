#include "core/ink/StrokeEraser.hpp"

#include "core/geometry/Distance.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeHitTest.hpp"

#include <gtest/gtest.h>

#include <array>
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] Stroke line(Uuid7Generator& ids, float fromX, float toX, float y) {
    Stroke stroke{ids.next(), StrokeStyle{.width = 2.0F}};
    for (int step = 0; step <= 100; ++step) {
        const float along = fromX + ((toX - fromX) * static_cast<float>(step) / 100.0F);
        stroke.append(InkSample{.x = along, .y = y});
    }
    return stroke;
}

TEST(StrokeEraserTest, AStrokeTheEraserMissedComesBackWhole) {
    Uuid7Generator ids;
    const Stroke stroke = line(ids, 0.0F, 100.0F, 50.0F);
    const EraserSweep away{
        .from = Point{.x = 0.0F, .y = 500.0F},
        .to = Point{.x = 100.0F, .y = 500.0F},
        .radius = 8.0F,
    };
    const std::array sweeps{away};

    const std::vector<Stroke> pieces = erased(stroke, sweeps, ids);

    ASSERT_EQ(pieces.size(), 1U);
    EXPECT_TRUE(wholeStrokeSurvives(stroke, pieces));
    EXPECT_EQ(pieces.front().samples().size(), stroke.samples().size());
}

TEST(StrokeEraserTest, RubbingTheMiddleLeavesTwoPieces) {
    Uuid7Generator ids;
    const Stroke stroke = line(ids, 0.0F, 100.0F, 50.0F);
    const EraserSweep tap{
        .from = Point{.x = 50.0F, .y = 50.0F},
        .to = Point{.x = 50.0F, .y = 50.0F},
        .radius = 8.0F,
    };
    const std::array sweeps{tap};

    const std::vector<Stroke> pieces = erased(stroke, sweeps, ids);

    ASSERT_EQ(pieces.size(), 2U);
    EXPECT_LT(pieces.front().samples().back().x, 45.0F);
    EXPECT_GT(pieces.back().samples().front().x, 55.0F);
    EXPECT_NE(pieces.front().id(), stroke.id());
    EXPECT_EQ(pieces.front().style(), stroke.style());
    EXPECT_FALSE(wholeStrokeSurvives(stroke, pieces));
}

TEST(StrokeEraserTest, RubbingAnEndShortensTheStroke) {
    Uuid7Generator ids;
    const Stroke stroke = line(ids, 0.0F, 100.0F, 50.0F);
    const EraserSweep start{
        .from = Point{.x = 0.0F, .y = 50.0F},
        .to = Point{.x = 20.0F, .y = 50.0F},
        .radius = 6.0F,
    };
    const std::array sweeps{start};

    const std::vector<Stroke> pieces = erased(stroke, sweeps, ids);

    ASSERT_EQ(pieces.size(), 1U);
    EXPECT_GT(pieces.front().samples().front().x, 20.0F);
    EXPECT_FLOAT_EQ(pieces.front().samples().back().x, 100.0F);
}

TEST(StrokeEraserTest, RubbingAllOfItLeavesNothing) {
    Uuid7Generator ids;
    const Stroke stroke = line(ids, 0.0F, 100.0F, 50.0F);
    const EraserSweep across{
        .from = Point{.x = -10.0F, .y = 50.0F},
        .to = Point{.x = 110.0F, .y = 50.0F},
        .radius = 20.0F,
    };
    const std::array sweeps{across};

    EXPECT_TRUE(erased(stroke, sweeps, ids).empty());
}

TEST(StrokeEraserTest, ASinglePointLeftOverIsNotKept) {
    Uuid7Generator ids;
    Stroke stroke{ids.next(), StrokeStyle{}};
    stroke.append(InkSample{.x = 0.0F, .y = 0.0F});
    stroke.append(InkSample{.x = 10.0F, .y = 0.0F});
    const EraserSweep tip{
        .from = Point{.x = 10.0F, .y = 0.0F},
        .to = Point{.x = 10.0F, .y = 0.0F},
        .radius = 2.0F,
    };
    const std::array sweeps{tip};

    EXPECT_TRUE(erased(stroke, sweeps, ids).empty());
}

TEST(StrokeEraserTest, TakingAWholeLineReachesFarLessFarThanRubbingOneOut) {
    EXPECT_FLOAT_EQ(reachOf(20.0F, EraseMode::Touched), 20.0F);
    EXPECT_FLOAT_EQ(reachOf(20.0F, EraseMode::WholeStroke), 5.0F);
}

TEST(StrokeEraserTest, TheReachOfAWholeLineNeverFallsToNothing) {
    EXPECT_FLOAT_EQ(reachOf(1.0F, EraseMode::WholeStroke), kNarrowestReach);
}

}
}
