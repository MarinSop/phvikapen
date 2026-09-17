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

[[nodiscard]] std::vector<Point> dot(const InkSample& sample, float width) {
    const float radius = std::max(width * sample.pressure * kHalf, kMinSegmentLength);
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

}

std::vector<Point> strokeOutline(const Stroke& stroke) {
    const std::vector<InkSample> samples = fitSpline(stroke.samples());
    const float width = stroke.style().width;
    if (samples.empty()) {
        return {};
    }
    if (samples.size() == 1) {
        return dot(samples.front(), width);
    }

    std::vector<Point> left;
    std::vector<Point> right;
    left.reserve(samples.size());
    right.reserve(samples.size());

    Normal normal{.x = 0.0F, .y = 1.0F};
    for (std::size_t i = 0; i < samples.size(); ++i) {
        normal = normalAt(samples, i, normal);
        const InkSample& sample = samples[i];
        const float half = width * sample.pressure * kHalf;
        left.push_back({.x = sample.x + (normal.x * half), .y = sample.y + (normal.y * half)});
        right.push_back({.x = sample.x - (normal.x * half), .y = sample.y - (normal.y * half)});
    }

    left.insert(left.end(), right.rbegin(), right.rend());
    return left;
}

}
