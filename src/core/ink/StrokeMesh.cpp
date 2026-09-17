#include "core/ink/StrokeMesh.hpp"

#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeSpline.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr float kHalf = 0.5F;
constexpr float kMaxChannel = 255.0F;
constexpr float kMinSegmentLength = 1e-4F;
constexpr std::size_t kVerticesPerSegment = 6;

[[nodiscard]] float normalizedChannel(std::uint8_t channel) {
    return static_cast<float>(channel) / kMaxChannel;
}

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

}

void appendSegment(std::vector<InkVertex>& vertices, const InkSample& from, const InkSample& to,
                   const StrokeStyle& style) {
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float length = std::hypot(dx, dy);
    const bool isDot = length <= kMinSegmentLength;

    const float halfFrom = style.width * from.pressure * kHalf;
    const float halfTo = style.width * to.pressure * kHalf;

    const float directionX = isDot ? 1.0F : dx / length;
    const float directionY = isDot ? 0.0F : dy / length;
    const float normalX = -directionY;
    const float normalY = directionX;
    const float extendFrom = isDot ? halfFrom : 0.0F;
    const float extendTo = isDot ? halfTo : 0.0F;

    const float red = normalizedChannel(style.color.red);
    const float green = normalizedChannel(style.color.green);
    const float blue = normalizedChannel(style.color.blue);
    const float alpha = normalizedChannel(style.color.alpha);
    const auto vertex = [&](float x, float y) {
        return InkVertex{.x = x, .y = y, .red = red, .green = green, .blue = blue, .alpha = alpha};
    };

    const float fromX = from.x - (directionX * extendFrom);
    const float fromY = from.y - (directionY * extendFrom);
    const float toX = to.x + (directionX * extendTo);
    const float toY = to.y + (directionY * extendTo);

    const InkVertex fromLeft = vertex(fromX + (normalX * halfFrom), fromY + (normalY * halfFrom));
    const InkVertex fromRight = vertex(fromX - (normalX * halfFrom), fromY - (normalY * halfFrom));
    const InkVertex toLeft = vertex(toX + (normalX * halfTo), toY + (normalY * halfTo));
    const InkVertex toRight = vertex(toX - (normalX * halfTo), toY - (normalY * halfTo));

    vertices.insert(vertices.end(), {fromLeft, fromRight, toLeft, toLeft, fromRight, toRight});
}

void appendStroke(std::vector<InkVertex>& vertices, const Stroke& stroke) {
    const std::vector<InkSample> samples = fitSpline(stroke.samples());
    const StrokeStyle& style = stroke.style();
    if (samples.empty()) {
        return;
    }
    if (samples.size() == 1) {
        appendSegment(vertices, samples.front(), samples.front(), style);
        return;
    }

    const InkVertex color{
        .red = normalizedChannel(style.color.red),
        .green = normalizedChannel(style.color.green),
        .blue = normalizedChannel(style.color.blue),
        .alpha = normalizedChannel(style.color.alpha),
    };
    const auto edges = [&](std::size_t index, Normal normal) {
        const InkSample& sample = samples[index];
        const float half = style.width * sample.pressure * kHalf;
        InkVertex left = color;
        left.x = sample.x + (normal.x * half);
        left.y = sample.y + (normal.y * half);
        InkVertex right = color;
        right.x = sample.x - (normal.x * half);
        right.y = sample.y - (normal.y * half);
        return std::pair{left, right};
    };

    vertices.reserve(vertices.size() + ((samples.size() - 1) * kVerticesPerSegment));
    Normal normal = normalAt(samples, 0, Normal{.x = 0.0F, .y = 1.0F});
    auto [fromLeft, fromRight] = edges(0, normal);
    for (std::size_t i = 1; i < samples.size(); ++i) {
        normal = normalAt(samples, i, normal);
        const auto [toLeft, toRight] = edges(i, normal);
        vertices.insert(vertices.end(), {fromLeft, fromRight, toLeft, toLeft, fromRight, toRight});
        fromLeft = toLeft;
        fromRight = toRight;
    }
}

}
