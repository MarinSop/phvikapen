#pragma once

#include "core/Error.hpp"
#include "core/math/Reading.hpp"

#include <utility>
#include <vector>

namespace phvikapen::core {

// A place on a graph, counted the way arithmetic counts rather than the way a screen does.
struct Spot {
    double across{};
    double up{};

    friend bool operator==(const Spot&, const Spot&) = default;
};

// How far a graph reaches to begin with, before the reader moves it or makes it larger.
inline constexpr double kFrameReach = 10.0;

// How much of the graph is being looked at.
struct Frame {
    double left{-kFrameReach};
    double right{kFrameReach};
    double bottom{-kFrameReach};
    double top{kFrameReach};

    friend bool operator==(const Frame&, const Frame&) = default;
};

[[nodiscard]] constexpr double widthOf(const Frame& frame) noexcept {
    return frame.right - frame.left;
}

[[nodiscard]] constexpr double heightOf(const Frame& frame) noexcept {
    return frame.top - frame.bottom;
}

[[nodiscard]] bool isDrawable(const Frame& frame) noexcept;

// The curve a statement draws. A curve that breaks — where it climbs out of sight, or where there
// is nothing to draw at all — is kept as more than one run, so that the two halves are never
// joined by a line that is not there.
struct Curve {
    std::vector<std::vector<Spot>> runs;

    friend bool operator==(const Curve&, const Curve&) = default;
};

inline constexpr int kFewestSamples = 8;
inline constexpr int kMostSamples = 4096;

// Which letter runs across the graph and which runs up it: x and y where they are there, and the
// one letter a statement holds where it holds only one.
[[nodiscard]] Result<std::pair<char, char>> axesOf(const Statement& statement);

// About how many rules a graph is given across its shorter side, before the step is rounded to a
// number a reader counts in.
inline constexpr int kWantedRules = 8;

// The step between the rules of a graph: one, two or five times a power of ten, so that the numbers
// the rules fall on are ones a reader counts in. A span of nothing at all is given a step of one.
[[nodiscard]] double ruleStep(double span);

// Where the rules fall between two numbers, at whole multiples of the step, smallest first. Neither
// end is left out where it falls on a multiple.
[[nodiscard]] std::vector<double> rulesBetween(double from, double to, double step);

// The curve a statement draws over a frame. For every place across the frame the statement is
// solved for the letter that runs up, so a straight line, a square and a circle are all drawn the
// same way, and a curve with two halves comes back as two runs.
[[nodiscard]] Result<Curve> plotted(const Statement& statement, char across, char up,
                                    const Frame& frame, int samples);

}
