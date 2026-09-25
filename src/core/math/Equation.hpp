#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace phvikapen::core {

enum class Operation : std::uint8_t {
    Add,
    Subtract,
    Multiply,
    Divide,
    Power,
    // A sign in front of what follows, and a square root of it: both are made of one step rather
    // than two.
    Negate,
    Root,
};

[[nodiscard]] constexpr bool isMadeOfOne(Operation operation) noexcept {
    return operation == Operation::Negate || operation == Operation::Root;
}

// An equation kept as a structure rather than as writing: the numbers and what is done to them.
// The steps are kept flat, without pointers, so that an equation can be copied, compared and
// carried about like every other value here. A step names the steps it is made of by where they
// sit, and a step is always made of steps that come before it.
struct Equation {
    static constexpr std::size_t kNothing = static_cast<std::size_t>(-1);
    // How deep an equation may be worked out, so that one that names itself cannot run on
    // for ever.
    static constexpr int kDeepest = 256;

    enum class Kind : std::uint8_t {
        Number,
        Operation,
    };

    struct Step {
        Kind kind{Kind::Number};
        Operation operation{Operation::Add};
        double number{};
        std::size_t left{kNothing};
        std::size_t right{kNothing};

        friend bool operator==(const Step&, const Step&) = default;
    };

    std::vector<Step> steps;
    // Which step is the whole equation.
    std::size_t whole{kNothing};

    friend bool operator==(const Equation&, const Equation&) = default;
};

[[nodiscard]] bool isEmpty(const Equation& equation) noexcept;

[[nodiscard]] Equation numberOf(double value);

// One equation made of two, with what is done to them on top.
[[nodiscard]] Equation joined(Operation operation, const Equation& left, const Equation& right);

// One equation with a sign or a root put in front of it.
[[nodiscard]] Equation applied(Operation operation, const Equation& of);

}
