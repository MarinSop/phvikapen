#include "core/ink/StrokeMesh.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr float kHalf = 0.5F;
constexpr float kMaxChannel = 255.0F;
constexpr float kMinSegmentLength = 1e-4F;

[[nodiscard]] float normalizedChannel(std::uint8_t channel) {
    return static_cast<float>(channel) / kMaxChannel;
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

}
