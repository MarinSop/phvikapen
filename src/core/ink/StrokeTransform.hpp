#pragma once

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/ink/Stroke.hpp"

#include <span>
#include <vector>

namespace phvikapen::core {

// How a piece of a page is moved, sized and turned. Everything happens about the pivot, which
// stays where it is, and in the units the page is measured in. One shape of change for every kind
// of thing a page can hold, so that ink, words and whatever comes later are handled the same way.
struct Transform {
    Point pivot{};
    float dx{};
    float dy{};
    float wide{1.0F};
    float tall{1.0F};
    float turn{};

    friend bool operator==(const Transform&, const Transform&) = default;
};

inline constexpr float kSmallestScale = 0.02F;
inline constexpr float kFullTurn = 360.0F;

[[nodiscard]] Transform normalized(Transform transform) noexcept;

// Whether the change would leave everything exactly where it was.
[[nodiscard]] bool leavesAsItWas(const Transform& transform) noexcept;

[[nodiscard]] Point placed(const Transform& transform, Point point) noexcept;

// What a rectangle covers once it has been turned: the box around the four moved corners.
[[nodiscard]] Rect placed(const Transform& transform, const Rect& area) noexcept;

// How much wider a line is drawn afterwards. Sizing a drawing unevenly cannot make a line thick
// one way and thin the other, so the width follows both together.
[[nodiscard]] float widthFactor(const Transform& transform) noexcept;

[[nodiscard]] Stroke transformed(const Stroke& stroke, const Transform& transform);

[[nodiscard]] std::vector<Stroke> transformed(std::span<const Stroke> strokes,
                                              const Transform& transform);

// The change that puts a box of the given size where the reader has dragged it to.
[[nodiscard]] Transform sizingTo(const Rect& from, const Rect& to) noexcept;

}
