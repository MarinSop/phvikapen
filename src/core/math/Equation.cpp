#include "core/math/Equation.hpp"

#include <cstddef>

namespace phvikapen::core {
namespace {

// The same step as it stands once it has been put after some others: what it is made of has moved
// along with it.
[[nodiscard]] Equation::Step moved(Equation::Step step, std::size_t along) noexcept {
    if (step.left != Equation::kNothing) {
        step.left += along;
    }
    if (step.right != Equation::kNothing) {
        step.right += along;
    }
    return step;
}

void appendSteps(Equation& into, const Equation& from, std::size_t along) {
    for (const Equation::Step& step : from.steps) {
        into.steps.push_back(moved(step, along));
    }
}

}

bool isEmpty(const Equation& equation) noexcept {
    return equation.whole == Equation::kNothing || equation.whole >= equation.steps.size();
}

Equation numberOf(double value) {
    Equation equation;
    equation.steps.push_back(Equation::Step{
        .kind = Equation::Kind::Number,
        .operation = Operation::Add,
        .number = value,
        .left = Equation::kNothing,
        .right = Equation::kNothing,
    });
    equation.whole = 0;
    return equation;
}

Equation joined(Operation operation, const Equation& left, const Equation& right) {
    if (isEmpty(left) || isEmpty(right)) {
        return {};
    }
    Equation equation;
    equation.steps.reserve(left.steps.size() + right.steps.size() + 1);
    appendSteps(equation, left, 0);
    const std::size_t along = left.steps.size();
    appendSteps(equation, right, along);
    equation.steps.push_back(Equation::Step{
        .kind = Equation::Kind::Operation,
        .operation = operation,
        .number = 0.0,
        .left = left.whole,
        .right = right.whole + along,
    });
    equation.whole = equation.steps.size() - 1;
    return equation;
}

Equation applied(Operation operation, const Equation& of) {
    if (isEmpty(of)) {
        return {};
    }
    Equation equation = of;
    equation.steps.push_back(Equation::Step{
        .kind = Equation::Kind::Operation,
        .operation = operation,
        .number = 0.0,
        .left = of.whole,
        .right = Equation::kNothing,
    });
    equation.whole = equation.steps.size() - 1;
    return equation;
}

}
