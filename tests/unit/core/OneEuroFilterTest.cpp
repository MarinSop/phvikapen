#include "core/filter/OneEuroFilter.hpp"

#include "core/filter/InkFilter.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <vector>

namespace phvikapen::core {
namespace {

using std::chrono::microseconds;

constexpr microseconds kSamplePeriod{8'000}; // 125 Hz, a common pen report rate.

/// Deterministic zig-zag noise, so that the tests do not depend on a random engine.
[[nodiscard]] float noiseAt(int index) {
    return (index % 2 == 0) ? 0.5F : -0.5F;
}

[[nodiscard]] float meanDistanceFrom(const std::vector<float>& values, float reference) {
    float total = 0.0F;
    for (const float value : values) {
        total += std::abs(value - reference);
    }
    return total / static_cast<float>(values.size());
}

TEST(OneEuroFilterTest, PassesTheFirstSampleThrough) {
    OneEuroFilter filter;

    EXPECT_FLOAT_EQ(filter.filter(42.0F, microseconds{0}), 42.0F);
}

TEST(OneEuroFilterTest, LeavesAConstantSignalAlone) {
    OneEuroFilter filter;
    microseconds now{0};

    static_cast<void>(filter.filter(10.0F, now));
    for (int i = 0; i < 50; ++i) {
        now += kSamplePeriod;
        EXPECT_FLOAT_EQ(filter.filter(10.0F, now), 10.0F);
    }
}

TEST(OneEuroFilterTest, RemovesJitterFromAStandingPen) {
    OneEuroFilter filter;
    microseconds now{0};
    std::vector<float> raw;
    std::vector<float> filtered;

    static_cast<void>(filter.filter(100.0F, now));
    for (int i = 0; i < 100; ++i) {
        now += kSamplePeriod;
        const float sample = 100.0F + noiseAt(i);
        raw.push_back(sample);
        filtered.push_back(filter.filter(sample, now));
    }

    // The filtered signal has to sit far closer to the true position than the raw one.
    EXPECT_LT(meanDistanceFrom(filtered, 100.0F), meanDistanceFrom(raw, 100.0F) / 3.0F);
}

TEST(OneEuroFilterTest, SpeedAdaptationReducesLagOnFastMovement) {
    constexpr float kStep = 20.0F; // Page units per sample, a fast stroke.
    constexpr int kSampleCount = 40;

    const auto lagOf = [](float beta) {
        OneEuroFilter filter{OneEuroParameters{.minCutoff = 1.0F, .beta = beta}};
        microseconds now{0};
        float position = 0.0F;
        float output = filter.filter(position, now);
        for (int i = 0; i < kSampleCount; ++i) {
            now += kSamplePeriod;
            position += kStep;
            output = filter.filter(position, now);
        }
        return position - output;
    };

    const float lagWithoutAdaptation = lagOf(0.0F);
    const float lagWithAdaptation = lagOf(0.05F);

    EXPECT_GT(lagWithoutAdaptation, 0.0F);
    EXPECT_LT(lagWithAdaptation, lagWithoutAdaptation);
}

TEST(OneEuroFilterTest, IgnoresSamplesThatDoNotAdvanceTheClock) {
    OneEuroFilter filter;
    const microseconds now{1'000};

    static_cast<void>(filter.filter(0.0F, now));
    const float first = filter.filter(5.0F, now + kSamplePeriod);
    const float repeated = filter.filter(500.0F, now + kSamplePeriod);

    EXPECT_FLOAT_EQ(repeated, first);
}

TEST(OneEuroFilterTest, ResetStartsANewStroke) {
    OneEuroFilter filter;

    static_cast<void>(filter.filter(0.0F, microseconds{0}));
    static_cast<void>(filter.filter(0.0F, kSamplePeriod));
    filter.reset();

    EXPECT_FLOAT_EQ(filter.filter(77.0F, microseconds{0}), 77.0F);
}

TEST(InkFilterTest, SmoothsPositionAndPressureAndKeepsTheRest) {
    InkFilter filter;
    const InkSample first{
        .x = 10.0F,
        .y = 10.0F,
        .pressure = 0.5F,
        .tiltX = 12.0F,
        .tiltY = -3.0F,
        .timestamp = microseconds{0},
    };
    const InkSample jump{
        .x = 60.0F,
        .y = 60.0F,
        .pressure = 1.0F,
        .tiltX = 12.0F,
        .tiltY = -3.0F,
        .timestamp = kSamplePeriod,
    };

    EXPECT_EQ(filter.filter(first), first);
    const InkSample filtered = filter.filter(jump);

    // The jump is damped, but the filter still moves towards it.
    EXPECT_GT(filtered.x, first.x);
    EXPECT_LT(filtered.x, jump.x);
    EXPECT_GT(filtered.pressure, first.pressure);
    EXPECT_LT(filtered.pressure, jump.pressure);
    // Everything the filter does not touch survives unchanged.
    EXPECT_FLOAT_EQ(filtered.tiltX, jump.tiltX);
    EXPECT_FLOAT_EQ(filtered.tiltY, jump.tiltY);
    EXPECT_EQ(filtered.timestamp, jump.timestamp);
}

} // namespace
} // namespace phvikapen::core
