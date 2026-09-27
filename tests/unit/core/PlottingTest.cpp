#include "core/math/Plotting.hpp"

#include "core/math/Reading.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace phvikapen::core {
namespace {

constexpr int kSamples = 201;

[[nodiscard]] Statement read(const std::string& written) {
    const Result<Statement> statement = statementOf(written);
    EXPECT_TRUE(statement.has_value()) << (statement ? "" : statement.error().message);
    return statement ? *statement : Statement{};
}

[[nodiscard]] Result<Curve> draw(const std::string& written, const Frame& frame = Frame{}) {
    const Statement statement = read(written);
    const Result<std::pair<char, char>> axes = axesOf(statement);
    if (!axes) {
        return std::unexpected{axes.error()};
    }
    return plotted(statement, axes->first, axes->second, frame, kSamples);
}

[[nodiscard]] std::size_t spotsIn(const Curve& curve) {
    std::size_t counted = 0;
    for (const auto& run : curve.runs) {
        counted += run.size();
    }
    return counted;
}

TEST(PlottingTest, DrawsAStraightLineInOneRun) {
    const Result<Curve> curve = draw("y = 2x + 1");

    ASSERT_TRUE(curve.has_value()) << curve.error().message;
    ASSERT_EQ(curve->runs.size(), 1U);
    EXPECT_EQ(curve->runs.front().size(), static_cast<std::size_t>(kSamples));
    const Spot& first = curve->runs.front().front();
    EXPECT_NEAR(first.up, (2.0 * first.across) + 1.0, 1e-9);
}

TEST(PlottingTest, DrawsASquare) {
    const Result<Curve> curve = draw("y = x^2");

    ASSERT_TRUE(curve.has_value()) << curve.error().message;
    ASSERT_EQ(curve->runs.size(), 1U);
    const auto& run = curve->runs.front();
    const auto lowest = std::ranges::min_element(run, {}, &Spot::up);
    EXPECT_NEAR(lowest->across, 0.0, 0.2);
    EXPECT_NEAR(lowest->up, 0.0, 0.2);
}

TEST(PlottingTest, DrawsACircleAsTwoRuns) {
    const Result<Curve> curve = draw("x^2 + y^2 = 25");

    ASSERT_TRUE(curve.has_value()) << curve.error().message;
    EXPECT_EQ(curve->runs.size(), 2U);
    for (const auto& run : curve->runs) {
        for (const Spot& spot : run) {
            EXPECT_NEAR((spot.across * spot.across) + (spot.up * spot.up), 25.0, 1e-6);
        }
    }
}

TEST(PlottingTest, ACurveThatBreaksComesBackAsMoreThanOneRun) {
    const Result<Curve> curve = draw("y = 1 / x");

    ASSERT_TRUE(curve.has_value()) << curve.error().message;
    EXPECT_GE(curve->runs.size(), 2U);
}

TEST(PlottingTest, DrawsALineAcrossTheGraph) {
    const Result<Curve> curve = draw("y = 3");

    ASSERT_TRUE(curve.has_value()) << curve.error().message;
    ASSERT_EQ(curve->runs.size(), 1U);
    for (const Spot& spot : curve->runs.front()) {
        EXPECT_DOUBLE_EQ(spot.up, 3.0);
    }
}

TEST(PlottingTest, SaysSoWhenNothingOfTheCurveIsInSight) {
    const Result<Curve> curve = draw("y = 10000000");

    EXPECT_FALSE(curve.has_value());
}

TEST(PlottingTest, RefusesAFrameWithNoRoomInIt) {
    const Statement statement = read("y = x");
    const Frame flat{.left = 1.0, .right = 1.0, .bottom = -1.0, .top = 1.0};

    EXPECT_FALSE(plotted(statement, 'x', 'y', flat, kSamples).has_value());
}

TEST(PlottingTest, RefusesTheSameLetterBothWays) {
    const Statement statement = read("y = x");

    EXPECT_FALSE(plotted(statement, 'x', 'x', Frame{}, kSamples).has_value());
}

TEST(PlottingTest, RefusesMoreThanTwoLetters) {
    const Statement statement = read("z = x + y");

    EXPECT_FALSE(axesOf(statement).has_value());
}

TEST(PlottingTest, KeepsTheNumberOfSamplesInRange) {
    const Result<Curve> curve = draw("y = x");
    ASSERT_TRUE(curve.has_value());

    const Statement statement = read("y = x");
    const Result<Curve> few = plotted(statement, 'x', 'y', Frame{}, 1);

    ASSERT_TRUE(few.has_value());
    EXPECT_GE(spotsIn(*few), static_cast<std::size_t>(kFewestSamples));
}

TEST(PlottingTest, FindsTheTwoWaysAcrossAGraph) {
    const Result<std::pair<char, char>> axes = axesOf(read("x^2 + y^2 = 4"));

    ASSERT_TRUE(axes.has_value());
    EXPECT_EQ(axes->first, 'x');
    EXPECT_EQ(axes->second, 'y');
}

TEST(RuleStepTest, StepsAreOnesTwosOrFivesTimesAPowerOfTen) {
    EXPECT_DOUBLE_EQ(ruleStep(20.0), 2.0);
    EXPECT_DOUBLE_EQ(ruleStep(80.0), 10.0);
    EXPECT_DOUBLE_EQ(ruleStep(1.0), 0.1);
    EXPECT_DOUBLE_EQ(ruleStep(400.0), 50.0);
}

TEST(RuleStepTest, ASpanOfNothingIsGivenAStepOfOne) {
    EXPECT_DOUBLE_EQ(ruleStep(0.0), 1.0);
    EXPECT_DOUBLE_EQ(ruleStep(-5.0), 1.0);
}

TEST(RulesBetweenTest, RulesFallOnWholeMultiplesOfTheStep) {
    const std::vector<double> rules = rulesBetween(-5.0, 5.0, 2.0);

    ASSERT_EQ(rules.size(), 5U);
    EXPECT_DOUBLE_EQ(rules.front(), -4.0);
    EXPECT_DOUBLE_EQ(rules[2], 0.0);
    EXPECT_DOUBLE_EQ(rules.back(), 4.0);
}

TEST(RulesBetweenTest, BothEndsAreKeptWhereTheyFallOnAMultiple) {
    const std::vector<double> rules = rulesBetween(0.0, 10.0, 5.0);

    ASSERT_EQ(rules.size(), 3U);
    EXPECT_DOUBLE_EQ(rules.front(), 0.0);
    EXPECT_DOUBLE_EQ(rules.back(), 10.0);
}

TEST(RulesBetweenTest, AStepThatWouldMakeTooManyRulesMakesNone) {
    EXPECT_TRUE(rulesBetween(0.0, 1e9, 1.0).empty());
    EXPECT_TRUE(rulesBetween(0.0, 10.0, 0.0).empty());
    EXPECT_TRUE(rulesBetween(5.0, 5.0, 1.0).empty());
}

TEST(RulesBetweenTest, ARuleFarFromNothingIsStillAWholeMultiple) {
    const std::vector<double> rules = rulesBetween(1000.0, 1000.5, 0.1);

    ASSERT_FALSE(rules.empty());
    for (const double at : rules) {
        EXPECT_NEAR(std::round(at * 10.0) / 10.0, at, 1e-9);
    }
}

}
}
