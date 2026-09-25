#include "core/math/Answer.hpp"

#include "core/Error.hpp"
#include "core/math/Equation.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <string>
#include <system_error>

namespace phvikapen::core {
namespace {

constexpr std::size_t kRoomForAnswer = 64;

[[nodiscard]] Result<double> madeOfOne(Operation operation, double of) {
    if (operation == Operation::Negate) {
        return -of;
    }
    if (of < 0.0) {
        return makeError(ErrorCode::InvalidArgument,
                         "there is no square root of less than nothing");
    }
    return std::sqrt(of);
}

[[nodiscard]] Result<double> madeOfTwo(Operation operation, double left, double right) {
    switch (operation) {
    case Operation::Add:
        return left + right;
    case Operation::Subtract:
        return left - right;
    case Operation::Multiply:
        return left * right;
    case Operation::Divide:
        if (right == 0.0) {
            return makeError(ErrorCode::InvalidArgument, "nothing can be divided by nothing");
        }
        return left / right;
    case Operation::Power:
        return std::pow(left, right);
    default:
        return makeError(ErrorCode::InvalidArgument, "that is not something done to two numbers");
    }
}

[[nodiscard]] Result<double> worthOf(const Equation& equation, std::size_t at, int deep) {
    if (deep > Equation::kDeepest) {
        return makeError(ErrorCode::InvalidArgument, "the sum is nested too deeply");
    }
    if (at >= equation.steps.size()) {
        return makeError(ErrorCode::InvalidArgument, "the sum names a step that is not there");
    }
    const Equation::Step& step = equation.steps[at];
    if (step.kind == Equation::Kind::Number) {
        return step.number;
    }

    const Result<double> left = worthOf(equation, step.left, deep + 1);
    if (!left) {
        return left;
    }
    if (isMadeOfOne(step.operation)) {
        return madeOfOne(step.operation, *left);
    }
    const Result<double> right = worthOf(equation, step.right, deep + 1);
    if (!right) {
        return right;
    }
    return madeOfTwo(step.operation, *left, *right);
}

}

Result<double> answerOf(const Equation& equation) {
    if (isEmpty(equation)) {
        return makeError(ErrorCode::InvalidArgument, "there is no sum here");
    }
    const Result<double> answer = worthOf(equation, equation.whole, 0);
    if (!answer) {
        return answer;
    }
    if (!std::isfinite(*answer)) {
        return makeError(ErrorCode::InvalidArgument, "the answer is too large to hold");
    }
    return answer;
}

std::string writtenAnswer(double answer) {
    if (!std::isfinite(answer)) {
        return {};
    }
    std::array<char, kRoomForAnswer> room{};
    const std::to_chars_result written =
        std::to_chars(room.data(), std::next(room.data(), static_cast<std::ptrdiff_t>(room.size())),
                      answer, std::chars_format::general, kFiguresShown);
    if (written.ec != std::errc{}) {
        return {};
    }
    return std::string{room.data(), written.ptr};
}

}
