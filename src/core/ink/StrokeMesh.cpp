#include "core/ink/StrokeMesh.hpp"

#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeSpline.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <span>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr float kHalf = 0.5F;
constexpr float kMaxChannel = 255.0F;
constexpr float kMinSegmentLength = 1e-4F;
constexpr std::size_t kVerticesPerSegment = 6;
constexpr std::size_t kDiscCorners = 16;
// Past this much of a turn the corner would run away from the line, so the pen tip fills it.
constexpr float kSharpestCorner = 0.35F;

[[nodiscard]] float normalizedChannel(std::uint8_t channel) {
    return static_cast<float>(channel) / kMaxChannel;
}

struct Normal {
    float x{};
    float y{};
};

[[nodiscard]] Normal normalOf(const InkSample& from, const InkSample& to, Normal previous) {
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float length = std::hypot(dx, dy);
    if (length <= kMinSegmentLength) {
        return previous;
    }
    return {.x = -dy / length, .y = dx / length};
}

// Where a line turns, its two sides meet at a point further out than half its width: that is what
// makes a corner a corner instead of a round tip.
struct Join {
    Normal normal;
    float reach{1.0F};
    bool tipped{};
};

[[nodiscard]] Join joinOf(Normal before, Normal after) {
    const float x = before.x + after.x;
    const float y = before.y + after.y;
    const float length = std::hypot(x, y);
    if (length <= kMinSegmentLength) {
        return {.normal = after, .reach = 1.0F, .tipped = true};
    }
    const Normal bisector{.x = x / length, .y = y / length};
    const float closeness = (bisector.x * before.x) + (bisector.y * before.y);
    if (closeness < kSharpestCorner) {
        return {.normal = bisector, .reach = 1.0F, .tipped = true};
    }
    return {.normal = bisector, .reach = 1.0F / closeness, .tipped = false};
}

}

void appendDisc(std::vector<InkVertex>& vertices, float x, float y, float radius,
                const Color& color) {
    if (radius <= kMinSegmentLength) {
        return;
    }
    const InkVertex tint{
        .red = normalizedChannel(color.red),
        .green = normalizedChannel(color.green),
        .blue = normalizedChannel(color.blue),
        .alpha = normalizedChannel(color.alpha),
    };
    const auto around = [&](std::size_t corner) {
        const float angle = 2.0F * std::numbers::pi_v<float>
                            * static_cast<float>(corner) / static_cast<float>(kDiscCorners);
        InkVertex vertex = tint;
        vertex.x = x + (radius * std::cos(angle));
        vertex.y = y + (radius * std::sin(angle));
        return vertex;
    };
    InkVertex middle = tint;
    middle.x = x;
    middle.y = y;
    vertices.reserve(vertices.size() + (kDiscCorners * 3));
    for (std::size_t corner = 0; corner < kDiscCorners; ++corner) {
        vertices.insert(vertices.end(), {middle, around(corner), around(corner + 1)});
    }
}

void appendSegment(std::vector<InkVertex>& vertices, const InkSample& from, const InkSample& to,
                   const StrokeStyle& style) {
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float length = std::hypot(dx, dy);

    const float halfFrom = style.width * from.pressure * kHalf;
    const float halfTo = style.width * to.pressure * kHalf;

    if (length <= kMinSegmentLength) {
        appendDisc(vertices, from.x, from.y, std::max(halfFrom, halfTo), style.color);
        return;
    }

    const float directionX = dx / length;
    const float directionY = dy / length;
    const float normalX = -directionY;
    const float normalY = directionX;

    const float red = normalizedChannel(style.color.red);
    const float green = normalizedChannel(style.color.green);
    const float blue = normalizedChannel(style.color.blue);
    const float alpha = normalizedChannel(style.color.alpha);
    const auto vertex = [&](float x, float y) {
        return InkVertex{.x = x, .y = y, .red = red, .green = green, .blue = blue, .alpha = alpha};
    };

    const InkVertex fromLeft = vertex(from.x + (normalX * halfFrom), from.y + (normalY * halfFrom));
    const InkVertex fromRight =
        vertex(from.x - (normalX * halfFrom), from.y - (normalY * halfFrom));
    const InkVertex toLeft = vertex(to.x + (normalX * halfTo), to.y + (normalY * halfTo));
    const InkVertex toRight = vertex(to.x - (normalX * halfTo), to.y - (normalY * halfTo));

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
    const auto edges = [&](std::size_t index, Normal normal, float reach) {
        const InkSample& sample = samples[index];
        const float half = style.width * sample.pressure * kHalf * reach;
        InkVertex left = color;
        left.x = sample.x + (normal.x * half);
        left.y = sample.y + (normal.y * half);
        InkVertex right = color;
        right.x = sample.x - (normal.x * half);
        right.y = sample.y - (normal.y * half);
        return std::pair{left, right};
    };

    const auto halfAt = [&](std::size_t index) {
        return style.width * samples[index].pressure * kHalf;
    };
    const auto tipAt = [&](std::size_t index) {
        appendDisc(vertices, samples[index].x, samples[index].y, halfAt(index), style.color);
    };

    vertices.reserve(vertices.size() + ((samples.size() - 1) * kVerticesPerSegment));
    tipAt(0);
    tipAt(samples.size() - 1);
    const std::size_t last = samples.size() - 1;
    Normal held{.x = 0.0F, .y = 1.0F};
    const auto normalAt = [&](std::size_t index) {
        held = normalOf(samples[index], samples[index + 1], held);
        return held;
    };

    Normal incoming = normalAt(0);
    auto [fromLeft, fromRight] = edges(0, incoming, 1.0F);
    for (std::size_t i = 1; i <= last; ++i) {
        const Normal outgoing = i == last ? incoming : normalAt(i);
        const Join join = i == last ? Join{.normal = incoming, .reach = 1.0F, .tipped = false}
                                    : joinOf(incoming, outgoing);
        if (join.tipped) {
            tipAt(i);
        }
        const auto [toLeft, toRight] = edges(i, join.normal, join.reach);
        vertices.insert(vertices.end(), {fromLeft, fromRight, toLeft, toLeft, fromRight, toRight});
        incoming = outgoing;
        fromLeft = toLeft;
        fromRight = toRight;
    }
}

}
