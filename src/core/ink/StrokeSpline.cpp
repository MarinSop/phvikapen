#include "core/ink/StrokeSpline.hpp"

#include "core/ink/InkSample.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr float kMinimumKnotInterval = 1e-4F;
constexpr float kMinimumDistance = 1e-3F;
constexpr float kMinimumSpacing = 0.1F;
constexpr std::size_t kMaximumSubdivisions = 32;

struct Vector {
    float x{};
    float y{};
};

[[nodiscard]] Vector position(const InkSample& sample) noexcept {
    return {.x = sample.x, .y = sample.y};
}

[[nodiscard]] Vector blend(Vector first, float firstWeight, Vector second,
                           float secondWeight) noexcept {
    return {
        .x = (first.x * firstWeight) + (second.x * secondWeight),
        .y = (first.y * firstWeight) + (second.y * secondWeight),
    };
}

[[nodiscard]] float knotInterval(Vector from, Vector to) noexcept {
    return std::max(std::sqrt(std::hypot(to.x - from.x, to.y - from.y)), kMinimumKnotInterval);
}

[[nodiscard]] Vector lerp(Vector from, Vector to, float fromKnot, float toKnot,
                          float knot) noexcept {
    const float span = toKnot - fromKnot;
    return blend(from, (toKnot - knot) / span, to, (knot - fromKnot) / span);
}

[[nodiscard]] Vector centripetal(std::span<const Vector, 4> points, float fraction) noexcept {
    const float t0 = 0.0F;
    const float t1 = t0 + knotInterval(points[0], points[1]);
    const float t2 = t1 + knotInterval(points[1], points[2]);
    const float t3 = t2 + knotInterval(points[2], points[3]);
    const float t = t1 + ((t2 - t1) * fraction);

    const Vector a1 = lerp(points[0], points[1], t0, t1, t);
    const Vector a2 = lerp(points[1], points[2], t1, t2, t);
    const Vector a3 = lerp(points[2], points[3], t2, t3, t);
    const Vector b1 = lerp(a1, a2, t0, t2, t);
    const Vector b2 = lerp(a2, a3, t1, t3, t);
    return lerp(b1, b2, t1, t2, t);
}

[[nodiscard]] InkSample interpolate(const InkSample& from, const InkSample& to, Vector at,
                                    float fraction) noexcept {
    const auto mix = [fraction](float first, float second) {
        return first + ((second - first) * fraction);
    };
    const auto elapsed = static_cast<float>((to.timestamp - from.timestamp).count());
    return InkSample{
        .x = at.x,
        .y = at.y,
        .pressure = mix(from.pressure, to.pressure),
        .tiltX = mix(from.tiltX, to.tiltX),
        .tiltY = mix(from.tiltY, to.tiltY),
        .timestamp = from.timestamp
                     + std::chrono::microseconds{static_cast<std::chrono::microseconds::rep>(
                         std::lround(elapsed * fraction))},
    };
}

}

std::vector<InkSample> fitSpline(std::span<const InkSample> samples, float spacing) {
    std::vector<InkSample> distinct;
    distinct.reserve(samples.size());
    for (const InkSample& sample : samples) {
        if (distinct.empty()
            || std::hypot(sample.x - distinct.back().x, sample.y - distinct.back().y)
                   > kMinimumDistance) {
            distinct.push_back(sample);
        }
    }
    if (distinct.size() < 2) {
        return distinct;
    }
    const float step = std::max(spacing, kMinimumSpacing);

    std::vector<InkSample> fitted;
    fitted.push_back(distinct.front());
    const std::size_t last = distinct.size() - 1;
    for (std::size_t i = 0; i < last; ++i) {
        const InkSample& from = distinct[i];
        const InkSample& to = distinct[i + 1];
        const Vector start = position(from);
        const Vector end = position(to);
        const std::array<Vector, 4> points{
            i == 0 ? start : position(distinct[i - 1]),
            start,
            end,
            i + 1 == last ? end : position(distinct[i + 2]),
        };

        const float length = std::hypot(end.x - start.x, end.y - start.y);
        const auto subdivisions = std::clamp<std::size_t>(
            static_cast<std::size_t>(std::ceil(length / step)), 1, kMaximumSubdivisions);
        for (std::size_t k = 1; k < subdivisions; ++k) {
            const float fraction = static_cast<float>(k) / static_cast<float>(subdivisions);
            fitted.push_back(interpolate(from, to, centripetal(points, fraction), fraction));
        }
        fitted.push_back(to);
    }
    return fitted;
}

}
