#pragma once

#include "core/Error.hpp"
#include "core/math/Equation.hpp"
#include "core/math/Reading.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace phvikapen::core {

// What a letter standing for a number is worth while a sum is being worked out.
struct Standing {
    char letter{};
    double value{};

    friend bool operator==(const Standing&, const Standing&) = default;
};

// What an equation comes to when the letters in it stand for numbers. A letter with nothing
// standing for it is refused rather than guessed at.
[[nodiscard]] Result<double> answerWith(const Equation& equation,
                                        std::span<const Standing> standing);

// An equation seen as a run of powers of one letter, lowest power first: 2x + 5 is {5, 2}. Every
// other letter must have something standing for it. An equation that is not a run of powers — a
// letter under a root, a letter in a divisor, a letter raised to a letter — is refused, because
// the working that follows only knows how to undo powers.
[[nodiscard]] Result<std::vector<double>> powersOf(const Equation& equation, char letter,
                                                   std::span<const Standing> standing);

// One line of the working shown beside an answer. The reason is named rather than written out, so
// that the window says it in the language the reader asked for.
struct Working {
    enum class Reason : std::uint8_t {
        // Everything was brought to one side of the equals sign.
        Gathered,
        // Both sides were divided by what stands against the letter.
        Divided,
        // The formula for an equation with a square in it was used.
        Formula,
        // What the letter stands for.
        Answered,
    };

    Reason reason{Reason::Gathered};
    // The statement as it stands at this point, written out.
    std::string said;
    // The number the reason needed, where it needed one.
    double number{};

    friend bool operator==(const Working&, const Working&) = default;
};

struct Solution {
    char letter{};
    // How high a power of the letter the statement holds: 1 for a straight line, 2 for a square.
    int power{};
    std::vector<Working> working;
    // What the letter can stand for, smallest first. A statement that holds for every number has
    // none of these and says so through `always`.
    std::vector<double> answers;
    bool always{};

    friend bool operator==(const Solution&, const Solution&) = default;
};

// A run of powers written out the way it is read: {5, 2} against x is "2x + 5".
[[nodiscard]] std::string writtenPowers(std::span<const double> powers, char letter);

// What the letter must stand for if the statement is to hold. Only a straight line and a square
// are worked out; anything higher is refused plainly rather than half answered.
[[nodiscard]] Result<Solution> solvedFor(const Statement& statement, char letter,
                                         std::span<const Standing> standing);

// The one letter a statement is worth solving for when the reader has not said which: the only
// letter in it, or x where there is more than one.
[[nodiscard]] Result<char> letterToSolveFor(const Statement& statement);

}
