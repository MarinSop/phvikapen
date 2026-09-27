#include "core/math/Plotting.hpp"

#include "core/Error.hpp"
#include "core/math/Reading.hpp"
#include "core/math/Solving.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

// How far above and below the frame a curve is still followed, so that a line leaving the top and
// coming back in keeps its shape, and how far it may climb before it counts as gone.
constexpr double kRoomOutside = 4.0;

constexpr double kTens = 10.0;
constexpr double kFive = 5.0;
constexpr double kTwo = 2.0;

// A graph is never given more rules than this, however small the step it is asked for.
constexpr int kMostRules = 512;
constexpr int kMostBranches = 2;

[[nodiscard]] bool isWorthKeeping(double up, const Frame& frame) noexcept {
    const double room = heightOf(frame) * kRoomOutside;
    return std::isfinite(up) && up > frame.bottom - room && up < frame.top + room;
}

}

bool isDrawable(const Frame& frame) noexcept {
    return std::isfinite(frame.left) && std::isfinite(frame.right) && std::isfinite(frame.bottom)
           && std::isfinite(frame.top) && widthOf(frame) > 0.0 && heightOf(frame) > 0.0;
}

double ruleStep(double span) {
    if (!(span > 0.0) || !std::isfinite(span)) {
        return 1.0;
    }
    const double rough = span / static_cast<double>(kWantedRules);
    const double power = std::pow(kTens, std::floor(std::log10(rough)));
    const double times = rough / power;
    if (times >= kFive) {
        return power * kFive;
    }
    return power * (times >= kTwo ? kTwo : 1.0);
}

std::vector<double> rulesBetween(double from, double to, double step) {
    std::vector<double> rules;
    if (!(step > 0.0) || !std::isfinite(step) || !(to > from)) {
        return rules;
    }
    const double span = to - from;
    if (span / step > static_cast<double>(kMostRules)) {
        return rules;
    }
    // Every rule is worked out from the step itself rather than added on to the one before, because
    // a step of a tenth walked from a long way out drifts.
    const auto first = static_cast<std::int64_t>(std::ceil(from / step));
    const auto last = static_cast<std::int64_t>(std::floor(to / step));
    for (std::int64_t multiple = first; multiple <= last; ++multiple) {
        const double whole = static_cast<double>(multiple) * step;
        if (whole >= from && whole <= to) {
            rules.push_back(whole);
        }
    }
    return rules;
}

Result<std::pair<char, char>> axesOf(const Statement& statement) {
    const std::string named = lettersOf(statement);
    if (named.empty()) {
        return makeError(ErrorCode::InvalidArgument, "there is nothing here to draw");
    }
    if (named.size() == 1) {
        const char only = named.front();
        return only == 'y' ? std::pair<char, char>{'x', 'y'} : std::pair<char, char>{only, 'y'};
    }
    if (named == "xy") {
        return std::pair<char, char>{'x', 'y'};
    }
    return makeError(ErrorCode::Unsupported,
                     "a graph is drawn for two letters at a time, and these are not x and y");
}

Result<Curve> plotted(const Statement& statement, char across, char up, const Frame& frame,
                      int samples) {
    if (across == up) {
        return makeError(ErrorCode::InvalidArgument,
                         "the two ways across a graph cannot be the same letter");
    }
    if (!isDrawable(frame)) {
        return makeError(ErrorCode::InvalidArgument, "there is no room to draw a graph in");
    }
    const int steps = std::clamp(samples, kFewestSamples, kMostSamples);

    // One run is being followed for each branch of the curve: a circle has a top and a bottom, and
    // the two are never joined by a line that is not there.
    std::array<std::vector<Spot>, kMostBranches> following;
    Curve curve;
    const auto breakOff = [&curve, &following](std::size_t branch) {
        if (following.at(branch).size() > 1) {
            curve.runs.push_back(std::move(following.at(branch)));
        }
        following.at(branch).clear();
    };

    for (int step = 0; step < steps; ++step) {
        const double along =
            frame.left + (widthOf(frame) * static_cast<double>(step) / (steps - 1.0));
        const std::array<Standing, 1> standing{Standing{.letter = across, .value = along}};
        const Result<Solution> solution = solvedFor(statement, up, standing);
        if (!solution || solution->always || solution->answers.empty()) {
            for (std::size_t branch = 0; branch < kMostBranches; ++branch) {
                breakOff(branch);
            }
            continue;
        }
        for (std::size_t branch = 0; branch < kMostBranches; ++branch) {
            if (branch >= solution->answers.size()) {
                breakOff(branch);
                continue;
            }
            const double height = solution->answers[branch];
            if (!isWorthKeeping(height, frame)) {
                breakOff(branch);
                continue;
            }
            following.at(branch).push_back(Spot{.across = along, .up = height});
        }
    }
    for (std::size_t branch = 0; branch < kMostBranches; ++branch) {
        breakOff(branch);
    }
    if (curve.runs.empty()) {
        return makeError(ErrorCode::InvalidArgument,
                         "there is nothing of this to see in the part of the graph being shown");
    }
    return curve;
}

}
