#include "core/math/Solving.hpp"

#include "core/Error.hpp"
#include "core/math/Answer.hpp"
#include "core/math/Equation.hpp"
#include "core/math/Reading.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

// How high a power of one letter an equation may be gathered into. Beyond this the numbers a run
// of powers holds stop meaning anything useful.
constexpr int kHighestPower = 8;

// Up to a square there is a formula; above it the numbers are searched for.
constexpr int kHighestByFormula = 2;
constexpr double kAlmostNothing = 1e-12;

// How near nothing a searched answer must bring the equation before it is called an answer.
constexpr double kNearEnough = 1e-9;

// How finely each stretch between two turning points is halved. Each step halves the stretch, so
// sixty steps take any stretch a double can hold down to nothing.
constexpr int kHalvings = 60;

// Two answers nearer than this are the same answer found twice.
constexpr double kSameAnswer = 1e-7;

// A stretch is halved, so its middle is the two ends brought together and split in two.
constexpr double kBothHalves = 2.0;

[[nodiscard]] bool isNothing(double value) noexcept {
    return std::abs(value) < kAlmostNothing;
}

[[nodiscard]] const Standing* standingFor(std::span<const Standing> standing, char letter) {
    const auto found = std::ranges::find(standing, letter, &Standing::letter);
    return found == standing.end() ? nullptr : &*found;
}

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
        if (isNothing(right)) {
            return makeError(ErrorCode::InvalidArgument, "nothing can be divided by nothing");
        }
        return left / right;
    case Operation::Power:
        return std::pow(left, right);
    default:
        return makeError(ErrorCode::InvalidArgument, "that is not something done to two numbers");
    }
}

[[nodiscard]] Result<double> worthOf(const Equation& equation, std::size_t at,
                                     std::span<const Standing> standing, int deep) {
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
    if (step.kind == Equation::Kind::Unknown) {
        const Standing* const stands = standingFor(standing, step.letter);
        if (stands == nullptr) {
            return makeError(ErrorCode::InvalidArgument,
                             std::string{"there is nothing standing for "} + step.letter);
        }
        return stands->value;
    }
    const Result<double> left = worthOf(equation, step.left, standing, deep + 1);
    if (!left) {
        return left;
    }
    if (isMadeOfOne(step.operation)) {
        return madeOfOne(step.operation, *left);
    }
    const Result<double> right = worthOf(equation, step.right, standing, deep + 1);
    if (!right) {
        return right;
    }
    return madeOfTwo(step.operation, *left, *right);
}

// A run of powers with the noughts above the highest one taken off, so that a run that came to
// nothing reads as nothing rather than as a line of noughts.
void trimmed(std::vector<double>& powers) {
    while (powers.size() > 1 && isNothing(powers.back())) {
        powers.pop_back();
    }
    if (powers.size() == 1 && isNothing(powers.front())) {
        powers.clear();
    }
}

[[nodiscard]] std::vector<double> added(const std::vector<double>& left,
                                        const std::vector<double>& right) {
    std::vector<double> sum(std::max(left.size(), right.size()), 0.0);
    for (std::size_t step = 0; step < left.size(); ++step) {
        sum[step] += left[step];
    }
    for (std::size_t step = 0; step < right.size(); ++step) {
        sum[step] += right[step];
    }
    trimmed(sum);
    return sum;
}

[[nodiscard]] std::vector<double> negated(std::vector<double> powers) {
    for (double& worth : powers) {
        worth = -worth;
    }
    return powers;
}

[[nodiscard]] Result<std::vector<double>> multiplied(const std::vector<double>& left,
                                                     const std::vector<double>& right) {
    if (left.empty() || right.empty()) {
        return std::vector<double>{};
    }
    const std::size_t reach = left.size() + right.size() - 1;
    if (static_cast<int>(reach) - 1 > kHighestPower) {
        return makeError(ErrorCode::Unsupported,
                         "that power of a letter is too high to be worked out here");
    }
    std::vector<double> product(reach, 0.0);
    for (std::size_t along = 0; along < left.size(); ++along) {
        for (std::size_t up = 0; up < right.size(); ++up) {
            product[along + up] += left[along] * right[up];
        }
    }
    trimmed(product);
    return product;
}

// What a run of powers comes to at one place: 2x + 5 at 3 is 11.
[[nodiscard]] double powersAt(std::span<const double> powers, double where) {
    double worth = 0.0;
    for (std::size_t step = powers.size(); step > 0; --step) {
        worth = (worth * where) + powers[step - 1];
    }
    return worth;
}

// The run of powers of the slope: 3x^2 + 2x + 5 becomes 6x + 2.
[[nodiscard]] std::vector<double> slopeOf(std::span<const double> powers) {
    if (powers.size() <= 1) {
        return {};
    }
    std::vector<double> slope(powers.size() - 1, 0.0);
    for (std::size_t step = 1; step < powers.size(); ++step) {
        slope[step - 1] = powers[step] * static_cast<double>(step);
    }
    return slope;
}

// How far out any answer can lie. Beyond this the highest power outgrows everything else, so
// nothing out there can come to nothing.
[[nodiscard]] double asFarAsAnswersGo(std::span<const double> powers) {
    const double highest = powers.back();
    double most = 0.0;
    for (std::size_t step = 0; step + 1 < powers.size(); ++step) {
        most = std::max(most, std::abs(powers[step] / highest));
    }
    return 1.0 + most;
}

// The one place between two turning points where a run of powers comes to nothing, found by
// halving the stretch. Between two turning points the run only climbs or only falls, so there is
// at most one, and it is here if the two ends lie on opposite sides of nothing.
[[nodiscard]] std::optional<double> nothingBetween(std::span<const double> powers, double from,
                                                   double to) {
    double low = from;
    double high = to;
    double atLow = powersAt(powers, low);
    const double atHigh = powersAt(powers, high);
    if (isNothing(atLow)) {
        return low;
    }
    if (isNothing(atHigh)) {
        return high;
    }
    if ((atLow < 0.0) == (atHigh < 0.0)) {
        return std::nullopt;
    }
    for (int step = 0; step < kHalvings; ++step) {
        const double middle = low + ((high - low) / kBothHalves);
        const double atMiddle = powersAt(powers, middle);
        if (isNothing(atMiddle)) {
            return middle;
        }
        if ((atMiddle < 0.0) == (atLow < 0.0)) {
            low = middle;
            atLow = atMiddle;
        } else {
            high = middle;
        }
    }
    return low + ((high - low) / kBothHalves);
}

void keepAnswer(std::vector<double>& answers, double answer) {
    const auto already = std::ranges::find_if(
        answers, [answer](double kept) { return std::abs(kept - answer) <= kSameAnswer; });
    if (already == answers.end()) {
        answers.push_back(answer);
    }
}

// Every number a run of powers comes to nothing at, smallest first. The turning points cut the run
// into stretches that each climb or fall throughout, so each stretch holds at most one answer and
// halving finds it. A turning point that is itself an answer is a number the run touches without
// crossing, which no halving would find.
[[nodiscard]] std::vector<double> nothingsOf(std::span<const double> powers) {
    std::vector<double> answers;
    if (powers.size() < 2) {
        return answers;
    }
    if (powers.size() == 2) {
        answers.push_back(-powers[0] / powers[1]);
        return answers;
    }
    const std::vector<double> slope = slopeOf(powers);
    const std::vector<double> turning = nothingsOf(slope);
    const double reach = asFarAsAnswersGo(powers);
    std::vector<double> edges;
    edges.reserve(turning.size() + 2);
    edges.push_back(-reach);
    for (const double at : turning) {
        if (at > -reach && at < reach) {
            edges.push_back(at);
        }
    }
    edges.push_back(reach);
    std::ranges::sort(edges);
    for (std::size_t step = 0; step + 1 < edges.size(); ++step) {
        if (const std::optional<double> found =
                nothingBetween(powers, edges[step], edges[step + 1])) {
            keepAnswer(answers, *found);
        }
    }
    for (const double at : turning) {
        if (std::abs(powersAt(powers, at)) <= kNearEnough) {
            keepAnswer(answers, at);
        }
    }
    std::ranges::sort(answers);
    return answers;
}

[[nodiscard]] bool isPlainNumber(const std::vector<double>& powers) {
    return powers.size() <= 1;
}

[[nodiscard]] double plainNumber(const std::vector<double>& powers) {
    return powers.empty() ? 0.0 : powers.front();
}

class Gatherer {
public:
    Gatherer(char letter, std::span<const Standing> standing) noexcept
        : m_letter{letter}, m_standing{standing} {}

    [[nodiscard]] Result<std::vector<double>> of(const Equation& equation, std::size_t at,
                                                 int deep) {
        if (deep > Equation::kDeepest) {
            return makeError(ErrorCode::InvalidArgument, "the sum is nested too deeply");
        }
        if (at >= equation.steps.size()) {
            return makeError(ErrorCode::InvalidArgument, "the sum names a step that is not there");
        }
        const Equation::Step& step = equation.steps[at];
        if (step.kind == Equation::Kind::Number) {
            return isNothing(step.number) ? std::vector<double>{}
                                          : std::vector<double>{step.number};
        }
        if (step.kind == Equation::Kind::Unknown) {
            if (step.letter == m_letter) {
                return std::vector<double>{0.0, 1.0};
            }
            const Standing* const stands = standingFor(m_standing, step.letter);
            if (stands == nullptr) {
                return makeError(ErrorCode::InvalidArgument,
                                 std::string{"there is nothing standing for "} + step.letter);
            }
            return isNothing(stands->value) ? std::vector<double>{}
                                            : std::vector<double>{stands->value};
        }

        Result<std::vector<double>> left = of(equation, step.left, deep + 1);
        if (!left) {
            return left;
        }
        if (isMadeOfOne(step.operation)) {
            return madeOfOneRun(step.operation, std::move(*left));
        }
        Result<std::vector<double>> right = of(equation, step.right, deep + 1);
        if (!right) {
            return right;
        }
        return madeOfTwoRuns(step.operation, std::move(*left), std::move(*right));
    }

private:
    [[nodiscard]] Result<std::vector<double>> madeOfOneRun(Operation operation,
                                                           std::vector<double> of) const {
        if (operation == Operation::Negate) {
            return negated(std::move(of));
        }
        if (!isPlainNumber(of)) {
            return makeError(ErrorCode::Unsupported, std::string{"a root of something holding "}
                                                         + m_letter + " is not worked out here");
        }
        const Result<double> root = madeOfOne(operation, plainNumber(of));
        if (!root) {
            return std::unexpected{root.error()};
        }
        return isNothing(*root) ? std::vector<double>{} : std::vector<double>{*root};
    }

    [[nodiscard]] Result<std::vector<double>>
    madeOfTwoRuns(Operation operation, std::vector<double> left, std::vector<double> right) {
        switch (operation) {
        case Operation::Add:
            return added(left, right);
        case Operation::Subtract:
            return added(left, negated(std::move(right)));
        case Operation::Multiply:
            return multiplied(left, right);
        case Operation::Divide:
            if (!isPlainNumber(right)) {
                return makeError(ErrorCode::Unsupported,
                                 std::string{"dividing by something holding "} + m_letter
                                     + " is not worked out here");
            }
            if (isNothing(plainNumber(right))) {
                return makeError(ErrorCode::InvalidArgument, "nothing can be divided by nothing");
            }
            for (double& worth : left) {
                worth /= plainNumber(right);
            }
            trimmed(left);
            return left;
        case Operation::Power:
            return raised(left, right);
        default:
            return makeError(ErrorCode::InvalidArgument,
                             "that is not something done to two numbers");
        }
    }

    [[nodiscard]] Result<std::vector<double>> raised(const std::vector<double>& base,
                                                     const std::vector<double>& exponent) const {
        if (!isPlainNumber(exponent)) {
            return makeError(ErrorCode::Unsupported, std::string{"raising to a power holding "}
                                                         + m_letter + " is not worked out here");
        }
        const double times = plainNumber(exponent);
        if (isPlainNumber(base)) {
            const Result<double> worth = madeOfTwo(Operation::Power, plainNumber(base), times);
            if (!worth) {
                return std::unexpected{worth.error()};
            }
            return isNothing(*worth) ? std::vector<double>{} : std::vector<double>{*worth};
        }
        if (times != std::floor(times) || times < 0.0 || times > kHighestPower) {
            return makeError(ErrorCode::Unsupported,
                             "that power of a letter is too high to be worked out here");
        }
        std::vector<double> product{1.0};
        for (int step = 0; step < static_cast<int>(times); ++step) {
            Result<std::vector<double>> next = multiplied(product, base);
            if (!next) {
                return next;
            }
            product = std::move(*next);
        }
        return product;
    }

    char m_letter;
    std::span<const Standing> m_standing;
};

[[nodiscard]] std::string writtenPiece(double worth, int power, char letter, bool first) {
    std::string said;
    if (first) {
        if (worth < 0.0) {
            said.push_back('-');
        }
    } else {
        said += worth < 0.0 ? " - " : " + ";
    }
    const double size = std::abs(worth);
    const bool needsNumber = power == 0 || !isNothing(size - 1.0);
    if (needsNumber) {
        said += writtenAnswer(size);
    }
    if (power >= 1) {
        said.push_back(letter);
    }
    if (power >= 2) {
        said += "^" + writtenAnswer(static_cast<double>(power));
    }
    return said;
}

}

Result<double> answerWith(const Equation& equation, std::span<const Standing> standing) {
    if (isEmpty(equation)) {
        return makeError(ErrorCode::InvalidArgument, "there is no sum here");
    }
    const Result<double> answer = worthOf(equation, equation.whole, standing, 0);
    if (!answer) {
        return answer;
    }
    if (!std::isfinite(*answer)) {
        return makeError(ErrorCode::InvalidArgument, "the answer is too large to hold");
    }
    return answer;
}

Result<std::vector<double>> powersOf(const Equation& equation, char letter,
                                     std::span<const Standing> standing) {
    if (isEmpty(equation)) {
        return std::vector<double>{};
    }
    Gatherer gatherer{letter, standing};
    Result<std::vector<double>> powers = gatherer.of(equation, equation.whole, 0);
    if (!powers) {
        return powers;
    }
    if (std::ranges::any_of(*powers, [](double worth) { return !std::isfinite(worth); })) {
        return makeError(ErrorCode::InvalidArgument, "the answer is too large to hold");
    }
    return powers;
}

std::string writtenPowers(std::span<const double> powers, char letter) {
    std::string said;
    for (std::size_t step = powers.size(); step > 0; --step) {
        const double worth = powers[step - 1];
        if (isNothing(worth)) {
            continue;
        }
        said += writtenPiece(worth, static_cast<int>(step) - 1, letter, said.empty());
    }
    return said.empty() ? "0" : said;
}

Result<char> letterToSolveFor(const Statement& statement) {
    const std::string named = lettersOf(statement);
    if (named.empty()) {
        return makeError(ErrorCode::InvalidArgument, "there is nothing here to solve for");
    }
    if (named.size() == 1) {
        return named.front();
    }
    if (named.contains('x')) {
        return 'x';
    }
    return makeError(ErrorCode::InvalidArgument,
                     "say which letter to solve for: there is more than one here");
}

Result<Solution> solvedFor(const Statement& statement, char letter,
                           std::span<const Standing> standing) {
    Result<std::vector<double>> left = powersOf(statement.left, letter, standing);
    if (!left) {
        return std::unexpected{left.error()};
    }
    Result<std::vector<double>> right = powersOf(statement.right, letter, standing);
    if (!right) {
        return std::unexpected{right.error()};
    }

    Solution solution;
    solution.letter = letter;
    std::vector<double> gathered = added(*left, negated(std::move(*right)));
    solution.power = gathered.empty() ? 0 : static_cast<int>(gathered.size()) - 1;
    solution.working.push_back(Working{
        .reason = Working::Reason::Gathered,
        .said = writtenPowers(gathered, letter) + " = 0",
        .number = 0.0,
    });

    if (gathered.empty()) {
        solution.always = true;
        return solution;
    }
    if (solution.power == 0) {
        return makeError(ErrorCode::InvalidArgument, "the two sides can never be equal");
    }
    if (solution.power > kHighestByFormula) {
        solution.answers = nothingsOf(gathered);
        solution.working.push_back(Working{
            .reason = Working::Reason::Searched,
            .said = writtenPowers(gathered, letter) + " = 0",
            .number = static_cast<double>(solution.power),
        });
        if (solution.answers.empty()) {
            return makeError(ErrorCode::InvalidArgument, "there is no number this can stand for");
        }
    } else if (solution.power == 1) {
        const double against = gathered[1];
        const double alone = gathered[0];
        solution.working.push_back(Working{
            .reason = Working::Reason::Divided,
            .said = std::string{letter} + " = " + writtenAnswer(-alone) + " / "
                    + writtenAnswer(against),
            .number = against,
        });
        solution.answers.push_back(-alone / against);
    } else {
        const double square = gathered[2];
        const double straight = gathered[1];
        const double alone = gathered[0];
        const double under = (straight * straight) - (4.0 * square * alone);
        solution.working.push_back(Working{
            .reason = Working::Reason::Formula,
            .said = "b^2 - 4ac = " + writtenAnswer(under),
            .number = under,
        });
        if (under < 0.0) {
            return makeError(ErrorCode::InvalidArgument,
                             "there is no number this can stand for: what is under the root is "
                             "less than nothing");
        }
        const double root = std::sqrt(under);
        const double first = (-straight - root) / (2.0 * square);
        const double second = (-straight + root) / (2.0 * square);
        solution.answers.push_back(std::min(first, second));
        if (!isNothing(root)) {
            solution.answers.push_back(std::max(first, second));
        }
    }

    for (const double answer : solution.answers) {
        if (!std::isfinite(answer)) {
            return makeError(ErrorCode::InvalidArgument, "the answer is too large to hold");
        }
        solution.working.push_back(Working{
            .reason = Working::Reason::Answered,
            .said = std::string{letter} + " = " + writtenAnswer(answer),
            .number = answer,
        });
    }
    return solution;
}

}
