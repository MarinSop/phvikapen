#include "core/math/Equation.hpp"

#include "core/Error.hpp"
#include "core/math/Answer.hpp"
#include "core/math/Reading.hpp"

#include <gtest/gtest.h>

#include <string>

namespace phvikapen::core {
namespace {

[[nodiscard]] Result<double> worked(std::string_view written) {
    const Result<Equation> equation = equationOf(tidied(written));
    if (!equation) {
        return std::unexpected{equation.error()};
    }
    return answerOf(*equation);
}

[[nodiscard]] std::string answered(std::string_view written) {
    const Result<double> answer = worked(written);
    return answer ? writtenAnswer(*answer) : std::string{};
}

TEST(EquationTest, AnEquationIsBuiltOutOfSmallerOnes) {
    const Equation sum = joined(Operation::Add, numberOf(2.0), numberOf(3.0));

    ASSERT_FALSE(isEmpty(sum));
    EXPECT_EQ(sum.steps.size(), 3U);
    EXPECT_EQ(sum.whole, 2U);
    EXPECT_EQ(sum.steps[sum.whole].operation, Operation::Add);
    EXPECT_EQ(answerOf(sum).value(), 5.0);
}

TEST(EquationTest, EverySmallerEquationKeepsItsOwnStepsWhenJoined) {
    const Equation left = joined(Operation::Multiply, numberOf(2.0), numberOf(3.0));
    const Equation right = joined(Operation::Multiply, numberOf(4.0), numberOf(5.0));

    const Equation both = joined(Operation::Add, left, right);

    EXPECT_EQ(both.steps.size(), 7U);
    EXPECT_EQ(answerOf(both).value(), 26.0);
}

TEST(EquationTest, ASignOrARootIsMadeOfOneStep) {
    EXPECT_TRUE(isMadeOfOne(Operation::Negate));
    EXPECT_TRUE(isMadeOfOne(Operation::Root));
    EXPECT_FALSE(isMadeOfOne(Operation::Add));
    EXPECT_EQ(answerOf(applied(Operation::Negate, numberOf(7.0))).value(), -7.0);
}

TEST(EquationTest, AnEquationMadeOfNothingIsEmpty) {
    EXPECT_TRUE(isEmpty(Equation{}));
    EXPECT_TRUE(isEmpty(joined(Operation::Add, numberOf(1.0), Equation{})));
    EXPECT_TRUE(isEmpty(applied(Operation::Root, Equation{})));
    EXPECT_FALSE(answerOf(Equation{}).has_value());
}

TEST(EquationTest, ASumNamingAStepThatIsNotThereIsRefused) {
    Equation broken = numberOf(1.0);
    broken.steps.front().kind = Equation::Kind::Operation;
    broken.steps.front().left = 40;

    EXPECT_FALSE(answerOf(broken).has_value());
}

TEST(ReadingTest, TheFourOperationsAreWorkedOutInTheUsualOrder) {
    EXPECT_EQ(answered("2+3"), "5");
    EXPECT_EQ(answered("2+3*4"), "14");
    EXPECT_EQ(answered("(2+3)*4"), "20");
    EXPECT_EQ(answered("10-4-3"), "3");
    EXPECT_EQ(answered("100/5/2"), "10");
}

TEST(ReadingTest, APowerIsWorkedOutBeforeASignAndFromTheRight) {
    EXPECT_EQ(answered("2^3"), "8");
    EXPECT_EQ(answered("-2^2"), "-4");
    EXPECT_EQ(answered("2^3^2"), "512");
    EXPECT_EQ(answered("2^-1"), "0.5");
}

TEST(ReadingTest, ARootIsReadAndWorkedOut) {
    EXPECT_EQ(answered("√9"), "3");
    EXPECT_EQ(answered("√9+7"), "10");
    EXPECT_EQ(answered("2√9"), "6");
    EXPECT_EQ(answered("√(4*4)"), "4");
}

TEST(ReadingTest, ARootCanBeTypedByItsName) {
    EXPECT_EQ(answered("sqrt(16)"), "4");
    EXPECT_EQ(answered("sqrt 25 + 1"), "6");
    EXPECT_EQ(answered("2 sqrt(9)"), "6");
}

TEST(ReadingTest, ANameThatMeansNothingIsSaidBack) {
    const Result<Equation> read = equationOf("2 + fish");

    ASSERT_FALSE(read.has_value());
    EXPECT_NE(read.error().message.find("fish"), std::string::npos);
}

TEST(ReadingTest, ANumberStandingAgainstABracketMeansMultiplication) {
    EXPECT_EQ(answered("2(3+4)"), "14");
    EXPECT_EQ(answered("(1+1)(2+2)"), "8");
}

TEST(ReadingTest, ASignInFrontOfANumberIsRead) {
    EXPECT_EQ(answered("-5+8"), "3");
    EXPECT_EQ(answered("8*-2"), "-16");
    EXPECT_EQ(answered("--3"), "3");
    EXPECT_EQ(answered("+4"), "4");
}

TEST(ReadingTest, DecimalsAreReadAndCountedWithoutATail) {
    EXPECT_EQ(answered("0.1+0.2"), "0.3");
    EXPECT_EQ(answered("1.5*4"), "6");
    EXPECT_EQ(answered(".5+.5"), "1");
}

TEST(TidyingTest, WhatAReaderMistakesForLettersIsPutRight) {
    EXPECT_EQ(tidied("l2 + 7"), "12+7");
    EXPECT_EQ(tidied("1O * 3"), "10*3");
    EXPECT_EQ(tidied("6 x 7"), "6*7");
    EXPECT_EQ(tidied("8 : 2"), "8/2");
    EXPECT_EQ(tidied("3,5 + 1"), "3.5+1");
    EXPECT_EQ(tidied("|2"), "12");
}

TEST(TidyingTest, SignsWrittenWithMoreRoomAreReadAsArithmetic) {
    EXPECT_EQ(tidied("6 × 7"), "6*7");
    EXPECT_EQ(tidied("8 ÷ 2"), "8/2");
    EXPECT_EQ(tidied("9 − 4"), "9-4");
    EXPECT_EQ(tidied("9 – 4"), "9-4");
    EXPECT_EQ(tidied("9 — 4"), "9-4");
    EXPECT_EQ(tidied("2 · 3"), "2*3");
    EXPECT_EQ(tidied("√9"), "√9");
}

TEST(TidyingTest, WhateverFollowsAnEqualsSignIsDropped) {
    EXPECT_EQ(tidied("2+3="), "2+3");
    EXPECT_EQ(tidied("2+3=6"), "2+3");
    EXPECT_EQ(answered("12 + 7 ="), "19");
}

TEST(TidyingTest, ALargeNumberWrittenInGroupsIsOneNumber) {
    EXPECT_EQ(tidied("1 000 + 1"), "1000+1");
    EXPECT_EQ(answered("1 000 + 1"), "1001");
}

TEST(ReadingTest, WhatIsNotArithmeticIsRefusedPlainly) {
    EXPECT_FALSE(worked("").has_value());
    EXPECT_FALSE(worked("hello").has_value());
    EXPECT_FALSE(worked("2+").has_value());
    EXPECT_FALSE(worked("(2+3").has_value());
    EXPECT_FALSE(worked("2+3)").has_value());
    EXPECT_FALSE(worked("*5").has_value());
    EXPECT_FALSE(worked("=5").has_value());
}

TEST(ReadingTest, ASumSaysWhichLetterItCouldNotRead) {
    const Result<Equation> read = equationOf("2 # 3");

    ASSERT_FALSE(read.has_value());
    EXPECT_NE(read.error().message.find('#'), std::string::npos);
}

TEST(ReadingTest, ALineOfNothingButBracketsIsRefusedRatherThanRunOn) {
    const std::string deep(Equation::kDeepest + 10, '(');

    EXPECT_FALSE(equationOf(deep).has_value());
}

TEST(AnswerTest, AnAnswerThatCannotBeReachedIsRefused) {
    EXPECT_FALSE(worked("1/0").has_value());
    EXPECT_FALSE(worked("√-4").has_value());
    EXPECT_FALSE(worked("9^9^9").has_value());
}

TEST(AnswerTest, AnAnswerIsWrittenWithoutATailOfNoughts) {
    EXPECT_EQ(writtenAnswer(3.0), "3");
    EXPECT_EQ(writtenAnswer(-2.5), "-2.5");
    EXPECT_EQ(writtenAnswer(0.0), "0");
    EXPECT_EQ(writtenAnswer(1.0 / 3.0), "0.333333333333");
}

}
}
