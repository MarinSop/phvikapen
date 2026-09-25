#include "core/ink/StrokeTurns.hpp"

#include "core/ink/InkSample.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] InkSample sampleAt(float x, float y) {
    return InkSample{.x = x, .y = y};
}

// A line drawn slowly along a diagonal, as a pen that reports whole pixels leaves it: a staircase
// of single steps rather than the straight line the hand meant.
[[nodiscard]] std::vector<InkSample> staircase(int steps) {
    std::vector<InkSample> samples;
    for (int i = 0; i < steps; ++i) {
        const auto along = static_cast<float>(i);
        samples.push_back(sampleAt(along + 1.0F, along));
        samples.push_back(sampleAt(along + 1.0F, along + 1.0F));
    }
    return samples;
}

[[nodiscard]] std::size_t cornerCount(const std::vector<bool>& corners) {
    std::size_t found = 0;
    for (const bool corner : corners) {
        found += corner ? 1U : 0U;
    }
    return found;
}

TEST(StrokeTurnsTest, HowFarAlongEachSampleLiesIsMeasuredFromTheFirst) {
    const std::vector<InkSample> samples{
        sampleAt(0.0F, 0.0F),
        sampleAt(3.0F, 4.0F),
        sampleAt(3.0F, 10.0F),
    };

    const std::vector<float> along = alongOf(samples);

    ASSERT_EQ(along.size(), 3U);
    EXPECT_FLOAT_EQ(along[0], 0.0F);
    EXPECT_FLOAT_EQ(along[1], 5.0F);
    EXPECT_FLOAT_EQ(along[2], 11.0F);
}

TEST(StrokeTurnsTest, AStraightLineTurnsNowhere) {
    std::vector<InkSample> samples;
    for (int i = 0; i <= 40; ++i) {
        samples.push_back(sampleAt(static_cast<float>(i), static_cast<float>(i)));
    }

    const std::vector<float> along = alongOf(samples);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        EXPECT_NEAR(turnAt(samples, along, i, kTurnReach), 0.0F, 1e-3F) << i;
    }
    EXPECT_EQ(cornerCount(cornersAlong(samples, kTurnReach)), 0U);
}

TEST(StrokeTurnsTest, ASquareCornerIsOneCornerAndNothingElse) {
    std::vector<InkSample> samples;
    for (int i = 0; i <= 20; ++i) {
        samples.push_back(sampleAt(static_cast<float>(i), 0.0F));
    }
    for (int i = 1; i <= 20; ++i) {
        samples.push_back(sampleAt(20.0F, static_cast<float>(i)));
    }

    const std::vector<bool> corners = cornersAlong(samples, kTurnReach);

    ASSERT_EQ(cornerCount(corners), 1U);
    EXPECT_TRUE(corners[20]);
}

TEST(StrokeTurnsTest, AStaircaseFromAPenReportingWholePixelsHoldsNoCornerAtAll) {
    const std::vector<InkSample> samples = staircase(12);
    const std::vector<float> along = alongOf(samples);

    // Two neighbouring samples turn a right angle at every step, yet the line does not.
    for (std::size_t i = 2; i + 2 < samples.size(); ++i) {
        EXPECT_LT(turnAt(samples, along, i, kTurnReach), kCornerTurn) << i;
    }
    EXPECT_EQ(cornerCount(cornersAlong(samples, kTurnReach)), 0U);
}

TEST(StrokeTurnsTest, AHandRoundingALoopAFewSamplesAtATimeTurnsNoCorners) {
    // Five samples to a turn, as a fast hand leaves them: each turns seventy two degrees.
    std::vector<InkSample> samples;
    for (int i = 0; i <= 10; ++i) {
        const float angle = 2.0F * std::numbers::pi_v<float> * static_cast<float>(i) / 5.0F;
        samples.push_back(sampleAt(20.0F * std::cos(angle), 20.0F * std::sin(angle)));
    }

    EXPECT_EQ(cornerCount(cornersAlong(samples, kTurnReach)), 0U);
}

TEST(StrokeTurnsTest, ALineTurnedBackOnItselfIsACornerWhateverItsNeighboursDo) {
    std::vector<InkSample> samples;
    for (int i = 0; i <= 10; ++i) {
        samples.push_back(sampleAt(static_cast<float>(i) * 2.0F, 0.0F));
    }
    for (int i = 1; i <= 10; ++i) {
        samples.push_back(sampleAt(20.0F - (static_cast<float>(i) * 2.0F), 0.4F));
    }

    const std::vector<bool> corners = cornersAlong(samples, kTurnReach);

    EXPECT_EQ(cornerCount(corners), 1U);
    EXPECT_TRUE(corners[10]);
}

TEST(StrokeTurnsTest, NeitherEndOfAStrokeTurns) {
    std::vector<InkSample> samples;
    for (int i = 0; i <= 10; ++i) {
        samples.push_back(sampleAt(static_cast<float>(i), 0.0F));
    }

    const std::vector<float> along = alongOf(samples);

    EXPECT_FLOAT_EQ(turnAt(samples, along, 0, kTurnReach), 0.0F);
    EXPECT_FLOAT_EQ(turnAt(samples, along, samples.size() - 1, kTurnReach), 0.0F);
    EXPECT_FLOAT_EQ(turnAt(samples, along, samples.size(), kTurnReach), 0.0F);
}

TEST(StrokeTurnsTest, TooShortALineToLookAlongTurnsNowhere) {
    const std::vector<InkSample> samples{sampleAt(0.0F, 0.0F), sampleAt(4.0F, 4.0F)};

    EXPECT_EQ(cornerCount(cornersAlong(samples, kTurnReach)), 0U);
    EXPECT_TRUE(cornersAlong({}, kTurnReach).empty());
    EXPECT_FLOAT_EQ(turnAt(samples, alongOf(samples), 1, 0.0F), 0.0F);
}

}
}
