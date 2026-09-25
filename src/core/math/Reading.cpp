#include "core/math/Reading.hpp"

#include "core/Error.hpp"
#include "core/math/Equation.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr std::string_view kRootSign = "√";

// Signs that mean arithmetic but are written with more than one letter's worth of room.
constexpr std::array<std::pair<std::string_view, char>, 8> kSpelledOut{
    std::pair<std::string_view, char>{"×", '*'}, // a times sign
    std::pair<std::string_view, char>{"÷", '/'}, // a division sign
    std::pair<std::string_view, char>{"·", '*'}, // a middle dot
    std::pair<std::string_view, char>{"∙", '*'}, // a bullet
    std::pair<std::string_view, char>{"⋅", '*'}, // a dot operator
    std::pair<std::string_view, char>{"−", '-'}, // a minus sign
    std::pair<std::string_view, char>{"–", '-'}, // a dash
    std::pair<std::string_view, char>{"—", '-'}, // a long dash
};

[[nodiscard]] bool isDigit(char letter) noexcept {
    return letter >= '0' && letter <= '9';
}

[[nodiscard]] bool isSpace(char letter) noexcept {
    return letter == ' ' || letter == '\t' || letter == '\n' || letter == '\r';
}

// What a shape that was read as a letter means when it stands among numbers.
[[nodiscard]] char amongNumbers(char letter) noexcept {
    switch (letter) {
    case 'l':
    case 'I':
    case '|':
        return '1';
    case 'O':
    case 'o':
        return '0';
    case 'x':
    case 'X':
        return '*';
    case ':':
        return '/';
    case ',':
        return '.';
    default:
        return letter;
    }
}

enum class Mark : std::uint8_t {
    Number,
    Plus,
    Minus,
    Times,
    Divide,
    Power,
    RootSign,
    Open,
    Close,
};

struct Token {
    Mark mark{Mark::Number};
    double number{};
};

[[nodiscard]] Result<Token> numberAt(std::string_view written, std::size_t& at) {
    double mantissa = 0.0;
    int places = 0;
    constexpr double kTen = 10.0;
    while (at < written.size() && isDigit(written[at])) {
        mantissa = (mantissa * kTen) + static_cast<double>(written[at] - '0');
        ++at;
    }
    if (at < written.size() && written[at] == '.') {
        ++at;
        while (at < written.size() && isDigit(written[at])) {
            mantissa = (mantissa * kTen) + static_cast<double>(written[at] - '0');
            ++places;
            ++at;
        }
    }
    const double value = places == 0 ? mantissa : mantissa / std::pow(kTen, places);
    if (!std::isfinite(value)) {
        return makeError(ErrorCode::InvalidArgument, "a number here is too large to work with");
    }
    return Token{.mark = Mark::Number, .number = value};
}

[[nodiscard]] Result<Mark> signAt(char letter) {
    switch (letter) {
    case '+':
        return Mark::Plus;
    case '-':
        return Mark::Minus;
    case '*':
        return Mark::Times;
    case '/':
        return Mark::Divide;
    case '^':
        return Mark::Power;
    case '(':
        return Mark::Open;
    case ')':
        return Mark::Close;
    default:
        return makeError(ErrorCode::InvalidArgument,
                         std::string{"there is a "} + letter + " where arithmetic should be");
    }
}

[[nodiscard]] Result<std::vector<Token>> marksOf(std::string_view written) {
    std::vector<Token> tokens;
    std::size_t at = 0;
    while (at < written.size()) {
        if (isSpace(written[at])) {
            ++at;
            continue;
        }
        if (written.substr(at).starts_with(kRootSign)) {
            tokens.push_back(Token{.mark = Mark::RootSign, .number = 0.0});
            at += kRootSign.size();
            continue;
        }
        if (isDigit(written[at]) || written[at] == '.') {
            Result<Token> number = numberAt(written, at);
            if (!number) {
                return std::unexpected{number.error()};
            }
            tokens.push_back(*number);
            continue;
        }
        Result<Mark> sign = signAt(written[at]);
        if (!sign) {
            return std::unexpected{sign.error()};
        }
        tokens.push_back(Token{.mark = *sign, .number = 0.0});
        ++at;
    }
    return tokens;
}

// How deeply the reading has gone into brackets, so that a line of nothing but brackets cannot run
// the machine out of room.
class Deeper {
public:
    explicit Deeper(int& deep) noexcept : m_deep{&deep} { ++*m_deep; }

    ~Deeper() { --*m_deep; }

    Deeper(const Deeper&) = delete;
    Deeper& operator=(const Deeper&) = delete;
    Deeper(Deeper&&) = delete;
    Deeper& operator=(Deeper&&) = delete;

    [[nodiscard]] bool tooDeep() const noexcept { return *m_deep > Equation::kDeepest; }

private:
    int* m_deep;
};

class Reader {
public:
    explicit Reader(std::span<const Token> tokens) noexcept : m_tokens{tokens} {}

    [[nodiscard]] Result<Equation> whole() {
        Result<Equation> equation = sums();
        if (!equation) {
            return equation;
        }
        if (!done()) {
            return makeError(ErrorCode::InvalidArgument, "the sum goes on where it should end");
        }
        if (isEmpty(*equation)) {
            return makeError(ErrorCode::InvalidArgument, "there is no sum here");
        }
        return equation;
    }

private:
    [[nodiscard]] bool done() const noexcept { return m_at >= m_tokens.size(); }

    [[nodiscard]] bool at(Mark mark) const noexcept {
        return !done() && m_tokens[m_at].mark == mark;
    }

    [[nodiscard]] bool standsAgainst() const noexcept {
        return at(Mark::Open) || at(Mark::RootSign);
    }

    void skip() noexcept { ++m_at; }

    [[nodiscard]] Result<Equation> sums() {
        Result<Equation> left = products();
        if (!left) {
            return left;
        }
        while (at(Mark::Plus) || at(Mark::Minus)) {
            const Operation operation = at(Mark::Plus) ? Operation::Add : Operation::Subtract;
            skip();
            Result<Equation> right = products();
            if (!right) {
                return right;
            }
            left = joined(operation, *left, *right);
        }
        return left;
    }

    [[nodiscard]] Result<Equation> products() {
        Result<Equation> left = signs();
        if (!left) {
            return left;
        }
        while (true) {
            Operation operation = Operation::Multiply;
            if (at(Mark::Times)) {
                skip();
            } else if (at(Mark::Divide)) {
                operation = Operation::Divide;
                skip();
            } else if (!standsAgainst()) {
                return left;
            }
            Result<Equation> right = signs();
            if (!right) {
                return right;
            }
            left = joined(operation, *left, *right);
        }
    }

    // A sign in front of what follows binds less tightly than a power, so that minus two squared
    // is minus four, the way it is written everywhere else.
    [[nodiscard]] Result<Equation> signs() {
        const Deeper deeper{m_deep};
        if (deeper.tooDeep()) {
            return makeError(ErrorCode::InvalidArgument, "the sum is nested too deeply");
        }
        if (at(Mark::Plus)) {
            skip();
            return signs();
        }
        if (at(Mark::Minus)) {
            skip();
            Result<Equation> of = signs();
            if (!of) {
                return of;
            }
            return applied(Operation::Negate, *of);
        }
        return powers();
    }

    [[nodiscard]] Result<Equation> powers() {
        Result<Equation> base = single();
        if (!base || !at(Mark::Power)) {
            return base;
        }
        skip();
        Result<Equation> exponent = signs();
        if (!exponent) {
            return exponent;
        }
        return joined(Operation::Power, *base, *exponent);
    }

    [[nodiscard]] Result<Equation> single() {
        if (done()) {
            return makeError(ErrorCode::InvalidArgument, "the sum breaks off");
        }
        const Token token = m_tokens[m_at];
        skip();
        if (token.mark == Mark::Number) {
            return numberOf(token.number);
        }
        if (token.mark == Mark::RootSign) {
            Result<Equation> of = signs();
            if (!of) {
                return of;
            }
            return applied(Operation::Root, *of);
        }
        if (token.mark == Mark::Open) {
            return bracketed();
        }
        return makeError(ErrorCode::InvalidArgument,
                         "the sum has something where a number should be");
    }

    [[nodiscard]] Result<Equation> bracketed() {
        Result<Equation> inside = sums();
        if (!inside) {
            return inside;
        }
        if (!at(Mark::Close)) {
            return makeError(ErrorCode::InvalidArgument, "a bracket is never closed");
        }
        skip();
        return inside;
    }

    std::span<const Token> m_tokens;
    std::size_t m_at{0};
    int m_deep{0};
};

}

std::string tidied(std::string_view written) {
    std::string put;
    put.reserve(written.size());
    std::size_t at = 0;
    while (at < written.size()) {
        if (written[at] == '=') {
            break;
        }
        if (isSpace(written[at])) {
            ++at;
            continue;
        }
        if (written.substr(at).starts_with(kRootSign)) {
            put.append(kRootSign);
            at += kRootSign.size();
            continue;
        }
        // Walked plainly rather than looked up, because what a search over a table hands back
        // differs from one compiler to the next.
        bool spelled = false;
        for (const auto& [sign, means] : kSpelledOut) {
            if (!written.substr(at).starts_with(sign)) {
                continue;
            }
            put.push_back(means);
            at += sign.size();
            spelled = true;
            break;
        }
        if (spelled) {
            continue;
        }
        put.push_back(amongNumbers(written[at]));
        ++at;
    }
    return put;
}

Result<Equation> equationOf(std::string_view written) {
    const Result<std::vector<Token>> tokens = marksOf(written);
    if (!tokens) {
        return std::unexpected{tokens.error()};
    }
    if (tokens->empty()) {
        return makeError(ErrorCode::InvalidArgument, "there is no sum here");
    }
    Reader reader{*tokens};
    return reader.whole();
}

}
