#pragma once

#include <algorithm>

namespace phvikapen::core {

struct Rect {
    float left{};
    float top{};
    float right{};
    float bottom{};

    [[nodiscard]] constexpr float width() const noexcept { return right - left; }

    [[nodiscard]] constexpr float height() const noexcept { return bottom - top; }

    [[nodiscard]] constexpr bool intersects(const Rect& other) const noexcept {
        return left <= other.right && other.left <= right && top <= other.bottom
               && other.top <= bottom;
    }

    // Whether another box stands wholly inside this one. A box on the very edge is inside.
    [[nodiscard]] constexpr bool contains(const Rect& other) const noexcept {
        return other.left >= left && other.right <= right && other.top >= top
               && other.bottom <= bottom;
    }

    [[nodiscard]] constexpr Rect united(const Rect& other) const noexcept {
        return {
            .left = std::min(left, other.left),
            .top = std::min(top, other.top),
            .right = std::max(right, other.right),
            .bottom = std::max(bottom, other.bottom),
        };
    }

    [[nodiscard]] constexpr Rect inflated(float margin) const noexcept {
        return {
            .left = left - margin,
            .top = top - margin,
            .right = right + margin,
            .bottom = bottom + margin,
        };
    }

    friend constexpr bool operator==(const Rect&, const Rect&) = default;
};

}
