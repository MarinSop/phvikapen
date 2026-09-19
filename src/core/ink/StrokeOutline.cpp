#include "core/ink/StrokeOutline.hpp"

#include "core/geometry/Distance.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeSpline.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <span>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr float kHalf = 0.5F;
constexpr float kMinSegmentLength = 1e-4F;
constexpr std::size_t kDotCorners = 16;
constexpr std::size_t kCapCorners = 12;

struct Normal {
    float x{};
    float y{};
};

[[nodiscard]] Normal normalAt(std::span<const InkSample> samples, std::size_t index,
                              Normal previous) {
    const InkSample& before = samples[index == 0 ? 0 : index - 1];
    const InkSample& after = samples[std::min(index + 1, samples.size() - 1)];
    const float dx = after.x - before.x;
    const float dy = after.y - before.y;
    const float length = std::hypot(dx, dy);
    if (length <= kMinSegmentLength) {
        return previous;
    }
    return {.x = -dy / length, .y = dx / length};
}

[[nodiscard]] std::vector<Point> dot(const InkSample& sample, const StrokeStyle& style) {
    const float radius = std::max(widthAt(style, sample.pressure) * kHalf, kMinSegmentLength);
    std::vector<Point> corners;
    corners.reserve(kDotCorners);
    for (std::size_t i = 0; i < kDotCorners; ++i) {
        const float angle = 2.0F * std::numbers::pi_v<float>
                            * static_cast<float>(i) / static_cast<float>(kDotCorners);
        corners.push_back({
            .x = sample.x + (radius * std::cos(angle)),
            .y = sample.y + (radius * std::sin(angle)),
        });
    }
    return corners;
}

// Half a pen tip, from the left side of the line round the way it faces to the right side.
void appendCap(std::vector<Point>& outline, const InkSample& sample, Normal left, float half) {
    const float start = std::atan2(left.y, left.x);
    for (std::size_t i = 1; i < kCapCorners; ++i) {
        const float angle =
            start
            - (std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(kCapCorners));
        outline.push_back({
            .x = sample.x + (half * std::cos(angle)),
            .y = sample.y + (half * std::sin(angle)),
        });
    }
}

}

std::vector<Point> strokeOutline(const Stroke& stroke) {
    const std::vector<InkSample> samples = fitSpline(stroke.samples());
    const StrokeStyle& style = stroke.style();
    if (samples.empty()) {
        return {};
    }
    if (samples.size() == 1) {
        return dot(samples.front(), style);
    }

    std::vector<Point> left;
    std::vector<Point> right;
    left.reserve(samples.size());
    right.reserve(samples.size());

    Normal normal{.x = 0.0F, .y = 1.0F};
    Normal first = normal;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        normal = normalAt(samples, i, normal);
        if (i == 0) {
            first = normal;
        }
        const InkSample& sample = samples[i];
        const float half = widthAt(style, sample.pressure) * kHalf;
        left.push_back({.x = sample.x + (normal.x * half), .y = sample.y + (normal.y * half)});
        right.push_back({.x = sample.x - (normal.x * half), .y = sample.y - (normal.y * half)});
    }

    if (style.roundEnds) {
        appendCap(left, samples.back(), normal, widthAt(style, samples.back().pressure) * kHalf);
    }
    left.insert(left.end(), right.rbegin(), right.rend());
    if (style.roundEnds) {
        appendCap(left, samples.front(), Normal{.x = -first.x, .y = -first.y},
                  widthAt(style, samples.front().pressure) * kHalf);
    }
    return left;
}

}
