#include "core/math/Solving.hpp"

#include "core/math/Equation.hpp"
#include "core/math/Reading.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] Statement read(const std::string& written) {
    const Result<Statement> statement = statementOf(written);
    EXPECT_TRUE(statement.has_value()) << (statement ? "" : statement.error().message);
    return statement ? *statement : Statement{};
}

[[nodiscard]] Result<Solution> solve(const std::string& written) {
    const Statement statement = read(written);
    const Result<char> letter = letterToSolveFor(statement);
    if (!letter) {
        return std::unexpected{letter.error()};
    }
    return solvedFor(statement, *letter, {});
}

TEST(StatementTest, ReadsBothSidesOfAnEqualsSign) {
    const Result<Statement> statement = statementOf("2x + 5 = 15");

    ASSERT_TRUE(statement.has_value());
    EXPECT_TRUE(statement->balanced);
    EXPECT_EQ(lettersOf(*statement), "x");
}

TEST(StatementTest, ALineWithoutAnEqualsSignIsAllLeftSide) {
    const Result<Statement> statement = statementOf("2 + 3");

    ASSERT_TRUE(statement.has_value());
    EXPECT_FALSE(statement->balanced);
    EXPECT_TRUE(isEmpty(statement->right));
}

TEST(StatementTest, RefusesMoreThanOneEqualsSign) {
    EXPECT_FALSE(statementOf("1 = 2 = 3").has_value());
}

TEST(StatementTest, ANumberAgainstALetterMeansMultiplication) {
    const Statement statement = read("3x");
    const Result<std::vector<double>> powers = powersOf(statement.left, 'x', {});

    ASSERT_TRUE(powers.has_value());
    ASSERT_EQ(powers->size(), 2U);
    EXPECT_DOUBLE_EQ((*powers)[0], 0.0);
    EXPECT_DOUBLE_EQ((*powers)[1], 3.0);
}

TEST(StatementTest, TwoLettersBesideEachOtherAreMultipliedTogether) {
    const Statement statement = read("xy");
    const std::array<Standing, 1> standing{Standing{.letter = 'y', .value = 4.0}};
    const Result<std::vector<double>> powers = powersOf(statement.left, 'x', standing);

    ASSERT_TRUE(powers.has_value());
    ASSERT_EQ(powers->size(), 2U);
    EXPECT_DOUBLE_EQ((*powers)[1], 4.0);
}

TEST(SolvingTest, SolvesAStraightLine) {
    const Result<Solution> solution = solve("2x + 5 = 15");

    ASSERT_TRUE(solution.has_value()) << solution.error().message;
    EXPECT_EQ(solution->letter, 'x');
    EXPECT_EQ(solution->power, 1);
    ASSERT_EQ(solution->answers.size(), 1U);
    EXPECT_DOUBLE_EQ(solution->answers.front(), 5.0);
}

TEST(SolvingTest, ShowsTheWorkingItWentThrough) {
    const Result<Solution> solution = solve("2x + 5 = 15");

    ASSERT_TRUE(solution.has_value());
    ASSERT_GE(solution->working.size(), 2U);
    EXPECT_EQ(solution->working.front().reason, Working::Reason::Gathered);
    EXPECT_EQ(solution->working.front().said, "2x - 10 = 0");
    EXPECT_EQ(solution->working.back().reason, Working::Reason::Answered);
    EXPECT_EQ(solution->working.back().said, "x = 5");
}

TEST(SolvingTest, SolvesASquareWithTwoAnswers) {
    const Result<Solution> solution = solve("x^2 - 5x + 6 = 0");

    ASSERT_TRUE(solution.has_value()) << solution.error().message;
    EXPECT_EQ(solution->power, 2);
    ASSERT_EQ(solution->answers.size(), 2U);
    EXPECT_DOUBLE_EQ(solution->answers[0], 2.0);
    EXPECT_DOUBLE_EQ(solution->answers[1], 3.0);
}

TEST(SolvingTest, ASquareThatTouchesOnceHasOneAnswer) {
    const Result<Solution> solution = solve("x^2 - 2x + 1 = 0");

    ASSERT_TRUE(solution.has_value());
    ASSERT_EQ(solution->answers.size(), 1U);
    EXPECT_NEAR(solution->answers.front(), 1.0, 1e-9);
}

TEST(SolvingTest, RefusesASquareThatNeverReachesNothing) {
    const Result<Solution> solution = solve("x^2 + 1 = 0");

    ASSERT_FALSE(solution.has_value());
    EXPECT_FALSE(solution.error().message.empty());
}

TEST(SolvingTest, RefusesTwoSidesThatCanNeverBeEqual) {
    EXPECT_FALSE(solve("2 = 3").has_value());
}

TEST(SolvingTest, SaysWhenAStatementHoldsForEveryNumber) {
    const Result<Solution> solution = solve("2x + x = 3x");

    ASSERT_TRUE(solution.has_value());
    EXPECT_TRUE(solution->always);
    EXPECT_TRUE(solution->answers.empty());
}

TEST(SolvingTest, RefusesAPowerHigherThanASquare) {
    const Result<Solution> solution = solve("x^3 = 8");

    ASSERT_FALSE(solution.has_value());
    EXPECT_EQ(solution.error().code, ErrorCode::Unsupported);
}

TEST(SolvingTest, RefusesALetterUnderARoot) {
    const Statement statement = read("sqrt(x)");

    EXPECT_FALSE(powersOf(statement.left, 'x', {}).has_value());
}

TEST(SolvingTest, RefusesDividingByTheLetterItSolvesFor) {
    const Statement statement = read("1/x");

    EXPECT_FALSE(powersOf(statement.left, 'x', {}).has_value());
}

TEST(SolvingTest, BracketsAgainstBracketsAreMultipliedOut) {
    const Result<Solution> solution = solve("(x - 2)(x - 3) = 0");

    ASSERT_TRUE(solution.has_value()) << solution.error().message;
    ASSERT_EQ(solution->answers.size(), 2U);
    EXPECT_NEAR(solution->answers[0], 2.0, 1e-9);
    EXPECT_NEAR(solution->answers[1], 3.0, 1e-9);
}

TEST(SolvingTest, SolvesForTheLetterItIsAsked) {
    const Statement statement = read("y = 2x + 1");
    const std::array<Standing, 1> standing{Standing{.letter = 'x', .value = 3.0}};
    const Result<Solution> solution = solvedFor(statement, 'y', standing);

    ASSERT_TRUE(solution.has_value()) << solution.error().message;
    ASSERT_EQ(solution->answers.size(), 1U);
    EXPECT_DOUBLE_EQ(solution->answers.front(), 7.0);
}

TEST(SolvingTest, AsksWhichLetterWhenThereIsMoreThanOneAndNoneIsX) {
    const Statement statement = read("a + b = 2");

    EXPECT_FALSE(letterToSolveFor(statement).has_value());
}

TEST(SolvingTest, PicksXWhenThereIsMoreThanOneLetter) {
    const Statement statement = read("y = 2x");
    const Result<char> letter = letterToSolveFor(statement);

    ASSERT_TRUE(letter.has_value());
    EXPECT_EQ(*letter, 'x');
}

TEST(SolvingTest, WorksOutASumWithNothingUnknownInIt) {
    const Statement statement = read("2 + 3 * 4");
    const Result<double> answer = answerWith(statement.left, {});

    ASSERT_TRUE(answer.has_value());
    EXPECT_DOUBLE_EQ(*answer, 14.0);
}

TEST(SolvingTest, RefusesASumWithNothingStandingForItsLetter) {
    const Statement statement = read("2x");

    EXPECT_FALSE(answerWith(statement.left, {}).has_value());
}

TEST(WrittenPowersTest, WritesARunOfPowersTheWayItIsRead) {
    const std::array<double, 3> powers{-10.0, 0.0, 1.0};

    EXPECT_EQ(writtenPowers(powers, 'x'), "x^2 - 10");
}

TEST(WrittenPowersTest, LeavesOutAOneStandingAgainstTheLetter) {
    const std::array<double, 2> powers{3.0, 1.0};

    EXPECT_EQ(writtenPowers(powers, 'x'), "x + 3");
}

TEST(WrittenPowersTest, ARunThatCameToNothingReadsAsNothing) {
    EXPECT_EQ(writtenPowers({}, 'x'), "0");
}

}
}
