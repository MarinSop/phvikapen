#include "core/geometry/Distance.hpp"

#include <algorithm>

namespace phvikapen::core {
namespace {

[[nodiscard]] float sideOf(Point point, Point from, Point to) noexcept {
    return ((to.x - from.x) * (point.y - from.y)) - ((to.y - from.y) * (point.x - from.x));
}

[[nodiscard]] bool haveOppositeSigns(float first, float second) noexcept {
    return (first > 0.0F && second < 0.0F) || (first < 0.0F && second > 0.0F);
}

[[nodiscard]] bool segmentsCross(Point firstStart, Point firstEnd, Point secondStart,
                                 Point secondEnd) noexcept {
    return haveOppositeSigns(sideOf(firstStart, secondStart, secondEnd),
                             sideOf(firstEnd, secondStart, secondEnd))
           && haveOppositeSigns(sideOf(secondStart, firstStart, firstEnd),
                                sideOf(secondEnd, firstStart, firstEnd));
}

}

float squaredDistanceToSegment(Point point, Point start, Point end) noexcept {
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float lengthSquared = (dx * dx) + (dy * dy);
    float t = 0.0F;
    if (lengthSquared > 0.0F) {
        t = std::clamp((((point.x - start.x) * dx) + ((point.y - start.y) * dy)) / lengthSquared,
                       0.0F, 1.0F);
    }
    const float offsetX = point.x - (start.x + (t * dx));
    const float offsetY = point.y - (start.y + (t * dy));
    return (offsetX * offsetX) + (offsetY * offsetY);
}

float squaredDistanceBetweenSegments(Point firstStart, Point firstEnd, Point secondStart,
                                     Point secondEnd) noexcept {
    if (segmentsCross(firstStart, firstEnd, secondStart, secondEnd)) {
        return 0.0F;
    }
    return std::min({
        squaredDistanceToSegment(firstStart, secondStart, secondEnd),
        squaredDistanceToSegment(firstEnd, secondStart, secondEnd),
        squaredDistanceToSegment(secondStart, firstStart, firstEnd),
        squaredDistanceToSegment(secondEnd, firstStart, firstEnd),
    });
}

}
