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
// operations, powers, square roots, brackets, a sign in front of a number, and a number standing
// against a bracket meaning multiplication.
[[nodiscard]] Result<Equation> equationOf(std::string_view written);

}
