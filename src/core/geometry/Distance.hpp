#pragma once

namespace phvikapen::core {

struct Point {
    float x{};
    float y{};

    friend constexpr bool operator==(const Point&, const Point&) = default;
};

[[nodiscard]] float squaredDistanceToSegment(Point point, Point start, Point end) noexcept;

[[nodiscard]] float squaredDistanceBetweenSegments(Point firstStart, Point firstEnd,
                                                   Point secondStart, Point secondEnd) noexcept;

}
