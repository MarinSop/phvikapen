#pragma once

#include "core/Error.hpp"
#include "core/math/Equation.hpp"

#include <string>
#include <string_view>

namespace phvikapen::core {

// What a reader of handwriting hands back, put into the letters arithmetic is written in: a letter
// l becomes a one, a letter O a nought, a letter x a times sign, a colon a division and a comma a
// decimal point. Whatever follows an equals sign is dropped, because what is asked for is the
// answer to what stands before it, and the spaces go, so that a large number written in groups is
// read as one number.
[[nodiscard]] std::string tidied(std::string_view written);

// Arithmetic written on one line, read into the structure that holds it. It understands the four
// operations, powers, square roots, brackets, a sign in front of a number, a letter standing for a
// number that is not known yet, and a number standing against a bracket or a letter meaning
// multiplication.
[[nodiscard]] Result<Equation> equationOf(std::string_view written);

// Two sides of an equals sign, each read on its own. A line with no equals sign is all left side,
// and the right side is nothing at all.
struct Statement {
    Equation left;
    Equation right;
    bool balanced{};

    friend bool operator==(const Statement&, const Statement&) = default;
};

[[nodiscard]] Result<Statement> statementOf(std::string_view written);

// The letters standing for numbers on both sides of a statement, each named once.
[[nodiscard]] std::string lettersOf(const Statement& statement);

}
