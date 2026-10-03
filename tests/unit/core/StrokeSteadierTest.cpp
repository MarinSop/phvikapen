#include "core/ink/StrokeSteadier.hpp"

#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

namespace phvikapen::core {
namespace {

using std::chrono::microseconds;

[[nodiscard]] InkSample sampleAt(float x, float y, microseconds timestamp = microseconds{0}) {
    return InkSample{.x = x, .y = y, .timestamp = timestamp};
}

[[nodiscard]] std::vector<InkSample> circle(float radius, std::size_t count) {
    std::vector<InkSample> samples;
    for (std::size_t i = 0; i <= count; ++i) {
        const float angle =
            2.0F * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(count);
        samples.push_back(sampleAt(radius * std::cos(angle), radius * std::sin(angle)));
    }
    return samples;
}

TEST(StrokeSteadierTest, AReachOfNothingLeavesTheSamplesAsTheyAre) {
    const std::vector<InkSample> samples{
        sampleAt(0.0F, 0.0F),
        sampleAt(3.0F, 1.0F),
        sampleAt(5.0F, -2.0F),
        sampleAt(9.0F, 4.0F),
    };

    EXPECT_EQ(steadied(samples, 0.0F), samples);
}

TEST(StrokeSteadierTest, KeepsEverySampleEvenWhenTheyShareATime) {
    // Windows hands over several positions of the pen at once, all stamped with the same time.
    std::vector<InkSample> samples;
    for (int i = 0; i < 40; ++i) {
        const auto x = static_cast<float>(i);
        samples.push_back(sampleAt(x, std::sin(x / 4.0F) * 6.0F, microseconds{(i / 4) * 16000}));
    }

    const std::vector<InkSample> steady = steadied(samples, 2.0F);

    ASSERT_EQ(steady.size(), samples.size());
    for (std::size_t i = 0; i < samples.size(); ++i) {
        EXPECT_EQ(steady[i].timestamp, samples[i].timestamp);
        EXPECT_NEAR(steady[i].x, samples[i].x, 0.5F);
    }
}

TEST(StrokeSteadierTest, BothEndsStayWhereThePenWas) {
    std::vector<InkSample> samples;
    for (int i = 0; i <= 50; ++i) {
        const auto x = static_cast<float>(i);
        samples.push_back(sampleAt(x, i % 2 == 0 ? 0.4F : -0.4F));
    }

    const std::vector<InkSample> steady = steadied(samples, 3.0F);

    EXPECT_EQ(steady.front(), samples.front());
    EXPECT_EQ(steady.back(), samples.back());
}

TEST(StrokeSteadierTest, EvensOutAWaveringLine) {
    std::vector<InkSample> samples;
    for (int i = 0; i <= 200; ++i) {
        const float x = static_cast<float>(i) * 0.5F;
        samples.push_back(sampleAt(x, i % 2 == 0 ? 0.3F : -0.3F));
    }

    const std::vector<InkSample> steady = steadied(samples, 2.0F);

    for (std::size_t i = 20; i + 20 < steady.size(); ++i) {
        EXPECT_LT(std::abs(steady[i].y), 0.03F) << "at " << steady[i].x;
    }
}

TEST(StrokeSteadierTest, DoesNotPullALoopBehindThePen) {
    const std::vector<InkSample> samples = circle(40.0F, 360);

    const std::vector<InkSample> steady = steadied(samples, 4.0F);

    for (std::size_t i = 0; i < steady.size(); ++i) {
        const float angle = std::atan2(samples[i].y, samples[i].x);
        const float at = std::atan2(steady[i].y, steady[i].x);
        EXPECT_NEAR(std::hypot(steady[i].x, steady[i].y), 40.0F, 0.3F);
        EXPECT_NEAR(std::remainder(at - angle, 2.0F * std::numbers::pi_v<float>), 0.0F, 1e-3F);
    }
}

TEST(StrokeSteadierTest, AHandThatSlowsDownDoesNotBendTheLine) {
    std::vector<InkSample> samples;
    float x = 0.0F;
    for (int i = 0; i < 120; ++i) {
        samples.push_back(sampleAt(x, 0.0F));
        x += i < 60 ? 2.0F : 0.1F;
    }

    const std::vector<InkSample> steady = steadied(samples, 3.0F);

    for (std::size_t i = 1; i < steady.size(); ++i) {
        EXPECT_FLOAT_EQ(steady[i].y, 0.0F);
        EXPECT_GT(steady[i].x, steady[i - 1].x);
    }
}

TEST(StrokeSteadierTest, SettlingAsThePenMovesGivesTheSameLineAsSettlingAtTheEnd) {
    std::vector<InkSample> samples;
    for (int i = 0; i < 300; ++i) {
        const auto t = static_cast<float>(i) * 0.05F;
        samples.push_back(sampleAt((t * 12.0F) + (3.0F * std::sin(t * 5.0F)),
                                   (3.0F * std::cos(t * 5.0F)) + (0.2F * std::sin(t * 90.0F))));
    }

    StrokeSteadier live{2.5F};
    for (const InkSample& sample : samples) {
        live.append(sample);
        static_cast<void>(live.settled());
    }
    const std::vector<InkSample> atOnce = steadied(samples, 2.5F);
    const auto settled = live.settled();

    ASSERT_EQ(settled.size(), atOnce.size());
    for (std::size_t i = 0; i < atOnce.size(); ++i) {
        EXPECT_NEAR(settled[i].x, atOnce[i].x, 1e-4F);
        EXPECT_NEAR(settled[i].y, atOnce[i].y, 1e-4F);
    }
}

TEST(StrokeSteadierTest, APenStandingStillAddsNoSamplesButKeepsItsPressure) {
    StrokeSteadier steadier{2.0F};
    steadier.append(InkSample{.x = 10.0F, .y = 10.0F, .pressure = 0.2F});
    steadier.append(InkSample{.x = 10.01F, .y = 10.0F, .pressure = 0.5F});
    steadier.append(InkSample{.x = 10.0F, .y = 10.02F, .pressure = 0.8F});

    const auto settled = steadier.settled();

    ASSERT_EQ(settled.size(), 1U);
    EXPECT_FLOAT_EQ(settled.front().x, 10.0F);
    EXPECT_FLOAT_EQ(settled.front().pressure, 0.8F);
}

TEST(StrokeSteadierTest, AStrokeKeepsItsNameAndItsLook) {
    const StrokeStyle style{.color = Color{.red = 200, .alpha = 255}, .width = 3.0F};
    Stroke stroke{Uuid{}, style};
    for (int i = 0; i < 10; ++i) {
        stroke.append(sampleAt(static_cast<float>(i) * 2.0F, 0.0F));
    }

    const Stroke steady = steadied(stroke, 1.0F);

    EXPECT_EQ(steady.id(), stroke.id());
    EXPECT_EQ(steady.style(), style);
    EXPECT_EQ(steady.samples().size(), stroke.samples().size());
}

}
}
