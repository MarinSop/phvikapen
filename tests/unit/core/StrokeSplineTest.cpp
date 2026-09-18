#include "core/ink/StrokeSpline.hpp"

#include "core/ink/InkSample.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <vector>

namespace phvikapen::core {
namespace {

using std::chrono::microseconds;

[[nodiscard]] InkSample sampleAt(float x, float y, float pressure = 1.0F,
                                 microseconds timestamp = microseconds{0}) {
    return InkSample{.x = x, .y = y, .pressure = pressure, .timestamp = timestamp};
}

[[nodiscard]] bool contains(const std::vector<InkSample>& fitted, const InkSample& sample) {
    return std::ranges::find(fitted, sample) != fitted.end();
}

TEST(StrokeSplineTest, PassesThroughEverySample) {
    const std::vector<InkSample> samples{
        sampleAt(0.0F, 0.0F),
        sampleAt(10.0F, 8.0F),
        sampleAt(25.0F, 3.0F),
        sampleAt(31.0F, 20.0F),
    };

    const std::vector<InkSample> fitted = fitSpline(samples);

    EXPECT_EQ(fitted.front(), samples.front());
    EXPECT_EQ(fitted.back(), samples.back());
    for (const InkSample& sample : samples) {
        EXPECT_TRUE(contains(fitted, sample)) << sample.x << ", " << sample.y;
    }
}

TEST(StrokeSplineTest, KeepsPointsOnAStraightLineOnIt) {
    const std::vector<InkSample> samples{
        sampleAt(0.0F, 0.0F),
        sampleAt(10.0F, 10.0F),
        sampleAt(30.0F, 30.0F),
        sampleAt(35.0F, 35.0F),
    };

    const std::vector<InkSample> fitted = fitSpline(samples);

    for (const InkSample& sample : fitted) {
        EXPECT_NEAR(sample.x, sample.y, 1e-3F);
    }
}

TEST(StrokeSplineTest, LeavesNoGapWiderThanTheSpacing) {
    const std::vector<InkSample> samples{
        sampleAt(0.0F, 0.0F),
        sampleAt(40.0F, 10.0F),
        sampleAt(60.0F, 50.0F),
    };

    const std::vector<InkSample> fitted = fitSpline(samples, 2.0F);

    ASSERT_GT(fitted.size(), samples.size());
    for (std::size_t i = 1; i < fitted.size(); ++i) {
        const float gap = std::hypot(fitted[i].x - fitted[i - 1].x, fitted[i].y - fitted[i - 1].y);
        EXPECT_LE(gap, 3.0F);
    }
}

TEST(StrokeSplineTest, BlendsPressureAndTimeBetweenSamples) {
    const std::vector<InkSample> samples{
        sampleAt(0.0F, 0.0F, 0.2F, microseconds{0}),
        sampleAt(8.0F, 0.0F, 0.6F, microseconds{8000}),
    };

    const std::vector<InkSample> fitted = fitSpline(samples, 2.0F);

    ASSERT_EQ(fitted.size(), 5U);
    EXPECT_FLOAT_EQ(fitted[2].x, 4.0F);
    EXPECT_FLOAT_EQ(fitted[2].pressure, 0.4F);
    EXPECT_EQ(fitted[2].timestamp, microseconds{4000});
}

TEST(StrokeSplineTest, IgnoresSamplesThatDoNotMove) {
    const std::vector<InkSample> samples{
        sampleAt(5.0F, 5.0F),
        sampleAt(5.0F, 5.0F),
        sampleAt(9.0F, 5.0F),
        sampleAt(9.0F, 5.0F),
    };

    const std::vector<InkSample> fitted = fitSpline(samples);

    for (const InkSample& sample : fitted) {
        EXPECT_TRUE(std::isfinite(sample.x));
        EXPECT_TRUE(std::isfinite(sample.y));
        EXPECT_FLOAT_EQ(sample.y, 5.0F);
    }
    EXPECT_EQ(fitted.front(), samples.front());
}

TEST(StrokeSplineTest, KeepsASharpCornerSharp) {
    std::vector<InkSample> samples;
    samples.push_back(InkSample{.x = 0.0F, .y = 0.0F});
    samples.push_back(InkSample{.x = 20.0F, .y = 0.0F});
    samples.push_back(InkSample{.x = 20.0F, .y = 20.0F});

    const std::vector<InkSample> fitted = fitSpline(samples, 2.0F);

    ASSERT_FALSE(fitted.empty());
    for (const InkSample& sample : fitted) {
        EXPECT_LE(sample.x, 20.0F + 0.01F);
        EXPECT_GE(sample.y, -0.01F);
    }
    EXPECT_TRUE(std::ranges::any_of(fitted, [](const InkSample& sample) {
        return std::abs(sample.x - 20.0F) < 0.01F && std::abs(sample.y) < 0.01F;
    }));
}

TEST(StrokeSplineTest, KeepsASingleSampleAsItIs) {
    const std::vector<InkSample> dot{sampleAt(3.0F, 4.0F), sampleAt(3.0F, 4.0F)};

    const std::vector<InkSample> fitted = fitSpline(dot);

    ASSERT_EQ(fitted.size(), 1U);
    EXPECT_EQ(fitted.front(), dot.front());
    EXPECT_TRUE(fitSpline({}).empty());
}

}
}
