#include "core/ink/StrokeTurns.hpp"

#include "core/ink/InkSample.hpp"

#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr float kNothing = 1e-6F;
// A stretch shorter than this much of the reach says too little about where the line is going.
constexpr float kEnoughOfTheReach = 0.5F;
// A turn back on itself is a corner whatever the line does either side of it.
constexpr float kReversal = 1.83F;
// How much less the line must turn a stretch either side for a turn to be a corner rather than
// part of a curve. A hand rounding a loop turns as much at every sample along it.
constexpr float kLoneTurn = 0.35F;

struct Way {
    float x{};
    float y{};
};

// The sample a stretch of the line back from this one, or nothing where the line is too short.
[[nodiscard]] std::size_t backFrom(std::span<const float> along, std::size_t index,
                                   float reach) noexcept {
    std::size_t found = index;
    while (found > 0 && along[index] - along[found] < reach) {
        --found;
    }
    return found;
}

[[nodiscard]] std::size_t foreFrom(std::span<const float> along, std::size_t index,
                                   float reach) noexcept {
    std::size_t found = index;
    const std::size_t last = along.size() - 1;
    while (found < last && along[found] - along[index] < reach) {
        ++found;
    }
    return found;
}

[[nodiscard]] float turnBetween(Way in, Way out) noexcept {
    return std::abs(std::atan2((in.x * out.y) - (in.y * out.x), (in.x * out.x) + (in.y * out.y)));
}

}

std::vector<float> alongOf(std::span<const InkSample> samples) {
    std::vector<float> along;
    along.reserve(samples.size());
    float walked = 0.0F;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        if (i > 0) {
            walked += std::hypot(samples[i].x - samples[i - 1].x, samples[i].y - samples[i - 1].y);
        }
        along.push_back(walked);
    }
    return along;
}

float turnAt(std::span<const InkSample> samples, std::span<const float> along, std::size_t index,
             float reach) noexcept {
    if (index >= samples.size() || samples.size() != along.size() || samples.size() < 3
        || reach <= kNothing) {
        return 0.0F;
    }
    const std::size_t back = backFrom(along, index, reach);
    const std::size_t fore = foreFrom(along, index, reach);
    const float behind = along[index] - along[back];
    const float ahead = along[fore] - along[index];
    const float enough = reach * kEnoughOfTheReach;
    if (behind < enough || ahead < enough) {
        return 0.0F;
    }
    const Way in{
        .x = samples[index].x - samples[back].x,
        .y = samples[index].y - samples[back].y,
    };
    const Way out{
        .x = samples[fore].x - samples[index].x,
        .y = samples[fore].y - samples[index].y,
    };
    return turnBetween(in, out);
}

bool isCornerAt(std::span<const float> turns, std::span<const float> along, std::size_t index,
                float reach) noexcept {
    if (index >= turns.size() || turns.size() != along.size() || turns[index] < kCornerTurn) {
        return false;
    }
    // A hand drawing slowly leaves a run of samples that all straddle the same corner. Only the
    // sharpest of them stands at it, so that the line either side is still evened out.
    for (std::size_t i = index; i > 0 && along[index] - along[i - 1] <= reach; --i) {
        if (turns[i - 1] > turns[index]) {
            return false;
        }
    }
    for (std::size_t i = index + 1; i < turns.size() && along[i] - along[index] <= reach; ++i) {
        // The later of two turns that are just as sharp gives way, so one of them is kept.
        if (turns[i] >= turns[index]) {
            return false;
        }
    }
    if (turns[index] >= kReversal) {
        return true;
    }
    // A turn the line does not share a stretch either side of it, which is what tells a corner
    // from a hand rounding a loop a few samples at a time.
    const float lonely = kLoneTurn * turns[index];
    return turns[backFrom(along, index, reach)] <= lonely
           && turns[foreFrom(along, index, reach)] <= lonely;
}

std::vector<bool> cornersAlong(std::span<const InkSample> samples, float reach) {
    const std::vector<float> along = alongOf(samples);
    std::vector<float> turns;
    turns.reserve(samples.size());
    for (std::size_t i = 0; i < samples.size(); ++i) {
        turns.push_back(turnAt(samples, along, i, reach));
    }

    std::vector<bool> corners(samples.size(), false);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        corners[i] = isCornerAt(turns, along, i, reach);
    }
    return corners;
}

}
