#pragma once

#include "core/Error.hpp"
#include "core/math/Equation.hpp"

#include <string>

namespace phvikapen::core {

// How many figures of an answer are worth showing. Counting in the way a machine counts leaves a
// tail on answers that ought to be plain, and a tenth added to two tenths should read as three
// tenths rather than as what the counting left behind.
inline constexpr int kFiguresShown = 12;

// What an equation comes to. An answer that cannot be reached — divided by nothing, a root of less
// than nothing, a number too large to hold — is refused rather than handed back as nonsense.
[[nodiscard]] Result<double> answerOf(const Equation& equation);

// An answer as it goes on the page: no tail of noughts, and a full stop for the decimal point
// wherever the application is run.
[[nodiscard]] std::string writtenAnswer(double answer);

}
