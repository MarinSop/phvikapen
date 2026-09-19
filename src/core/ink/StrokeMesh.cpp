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
#include <tuple>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr float kHalf = 0.5F;
constexpr float kMaxChannel = 255.0F;
constexpr float kMinSegmentLength = 1e-4F;
constexpr std::size_t kVerticesPerSegment = 6;
// A round tip has enough sides that none strays from the circle by more than this, in page units.
constexpr float kRoundness = 0.02F;
constexpr std::size_t kFewestSides = 12;
constexpr std::size_t kMostSides = 96;
constexpr std::size_t kQuarter = 4;
// Past this much of a turn the corner would run away from the line, so the pen tip fills it.
constexpr float kSharpestCorner = 0.35F;
// A pen that turns further than about forty degrees shows its round tip in the turn.
constexpr float kRoundTurn = 0.94F;

[[nodiscard]] float normalizedChannel(std::uint8_t channel) {
    return static_cast<float>(channel) / kMaxChannel;
}

[[nodiscard]] InkVertex tintOf(const Color& color) {
    return InkVertex{
        .red = normalizedChannel(color.red),
        .green = normalizedChannel(color.green),
        .blue = normalizedChannel(color.blue),
        .alpha = normalizedChannel(color.alpha),
    };
}

[[nodiscard]] std::size_t sidesFor(float radius) {
    const float strayed = std::min(kRoundness / radius, 1.0F);
    const auto wanted =
        static_cast<std::size_t>(std::ceil(std::numbers::pi_v<float> / std::acos(1.0F - strayed)));
    const std::size_t sides = std::clamp(wanted, kFewestSides, kMostSides);
    return ((sides + kQuarter - 1) / kQuarter) * kQuarter;
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
    float closeness{1.0F};
    bool tipped{};
};

[[nodiscard]] Join joinOf(Normal before, Normal after) {
    const float x = before.x + after.x;
    const float y = before.y + after.y;
    const float length = std::hypot(x, y);
    if (length <= kMinSegmentLength) {
        return {.normal = after, .reach = 1.0F, .closeness = 0.0F, .tipped = true};
    }
    const Normal bisector{.x = x / length, .y = y / length};
    const float closeness = (bisector.x * before.x) + (bisector.y * before.y);
    if (closeness < kSharpestCorner) {
        return {.normal = bisector, .reach = 1.0F, .closeness = closeness, .tipped = true};
    }
    return {.normal = bisector, .reach = 1.0F / closeness, .closeness = closeness, .tipped = false};
}

void appendSamples(std::vector<InkVertex>& vertices, std::span<const InkSample> samples,
                   const StrokeStyle& style) {
    if (samples.empty()) {
        return;
    }
    if (samples.size() == 1) {
        appendSegment(vertices, samples.front(), samples.front(), style);
        return;
    }

    const InkVertex tint = tintOf(style.color);
    const auto halfAt = [&](std::size_t index) {
        return widthAt(style, samples[index].pressure) * kHalf;
    };
    const auto edges = [&](std::size_t index, Normal normal, float reach) {
        const InkSample& sample = samples[index];
        const float half = halfAt(index) * reach;
        InkVertex left = tint;
        left.x = sample.x + (normal.x * half);
        left.y = sample.y + (normal.y * half);
        InkVertex right = tint;
        right.x = sample.x - (normal.x * half);
        right.y = sample.y - (normal.y * half);
        return std::pair{left, right};
    };
    const auto tipAt = [&](std::size_t index) {
        appendDisc(vertices, samples[index].x, samples[index].y, halfAt(index), style.color);
    };

    vertices.reserve(vertices.size() + ((samples.size() - 1) * kVerticesPerSegment));
    const std::size_t last = samples.size() - 1;
    if (style.roundEnds) {
        tipAt(0);
        tipAt(last);
    }
    Normal held{.x = 0.0F, .y = 1.0F};
    const auto normalAt = [&](std::size_t index) {
        held = normalOf(samples[index], samples[index + 1], held);
        return held;
    };

    Normal incoming = normalAt(0);
    auto [fromLeft, fromRight] = edges(0, incoming, 1.0F);
    for (std::size_t i = 1; i <= last; ++i) {
        const Normal outgoing = i == last ? incoming : normalAt(i);
        const Join join = i == last ? Join{.normal = incoming} : joinOf(incoming, outgoing);
        if (style.roundEnds && join.closeness < kRoundTurn) {
            // The piece before the turn ends square and the pen tip rounds the outside of it.
            const auto [endLeft, endRight] = edges(i, incoming, 1.0F);
            vertices.insert(vertices.end(),
                            {fromLeft, fromRight, endLeft, endLeft, fromRight, endRight});
            tipAt(i);
            std::tie(fromLeft, fromRight) = edges(i, outgoing, 1.0F);
        } else {
            if (join.tipped) {
                tipAt(i);
            }
            const auto [toLeft, toRight] = edges(i, join.normal, join.reach);
            vertices.insert(vertices.end(),
                            {fromLeft, fromRight, toLeft, toLeft, fromRight, toRight});
            fromLeft = toLeft;
            fromRight = toRight;
        }
        incoming = outgoing;
    }
}

}

void appendDisc(std::vector<InkVertex>& vertices, float x, float y, float radius,
                const Color& color) {
    if (radius <= kMinSegmentLength) {
        return;
    }
    const InkVertex tint = tintOf(color);
    const std::size_t sides = sidesFor(radius);
    const auto around = [&](std::size_t corner) {
        const float angle = 2.0F * std::numbers::pi_v<float>
                            * static_cast<float>(corner) / static_cast<float>(sides);
        InkVertex vertex = tint;
        vertex.x = x + (radius * std::cos(angle));
        vertex.y = y + (radius * std::sin(angle));
        return vertex;
    };
    InkVertex middle = tint;
    middle.x = x;
    middle.y = y;
    vertices.reserve(vertices.size() + (sides * 3));
    for (std::size_t corner = 0; corner < sides; ++corner) {
        vertices.insert(vertices.end(), {middle, around(corner), around(corner + 1)});
    }
}

void appendSegment(std::vector<InkVertex>& vertices, const InkSample& from, const InkSample& to,
                   const StrokeStyle& style) {
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float length = std::hypot(dx, dy);

    const float halfFrom = widthAt(style, from.pressure) * kHalf;
    const float halfTo = widthAt(style, to.pressure) * kHalf;

    if (length <= kMinSegmentLength) {
        appendDisc(vertices, from.x, from.y, std::max(halfFrom, halfTo), style.color);
        return;
    }

    const float normalX = -dy / length;
    const float normalY = dx / length;
    const InkVertex tint = tintOf(style.color);
    const auto vertex = [&](float x, float y) {
        InkVertex made = tint;
        made.x = x;
        made.y = y;
        return made;
    };

    const InkVertex fromLeft = vertex(from.x + (normalX * halfFrom), from.y + (normalY * halfFrom));
    const InkVertex fromRight =
        vertex(from.x - (normalX * halfFrom), from.y - (normalY * halfFrom));
    const InkVertex toLeft = vertex(to.x + (normalX * halfTo), to.y + (normalY * halfTo));
    const InkVertex toRight = vertex(to.x - (normalX * halfTo), to.y - (normalY * halfTo));

    vertices.insert(vertices.end(), {fromLeft, fromRight, toLeft, toLeft, fromRight, toRight});
}

void appendStroke(std::vector<InkVertex>& vertices, const Stroke& stroke) {
    appendSamples(vertices, fitSpline(stroke.samples()), stroke.style());
}

void appendStroke(std::vector<InkVertex>& vertices, std::span<const InkSample> samples,
                  const StrokeStyle& style) {
    appendSamples(vertices, fitSpline(samples), style);
}

}
