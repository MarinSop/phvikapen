#pragma once

#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <vector>

namespace phvikapen::core {

/// Vertex of tessellated ink: position in page coordinates and straight-alpha color in [0, 1].
struct InkVertex {
    float x{};
    float y{};
    float red{};
    float green{};
    float blue{};
    float alpha{};

    friend constexpr bool operator==(const InkVertex&, const InkVertex&) = default;
};

/// Appends two triangles (six vertices) covering the segment from @p from to @p to.
///
/// The half width at each end is the nominal style width scaled by that sample's pressure.
/// A zero-length segment produces a square dot. Joins between segments are not rounded.
/// TODO(M2): Replace with filtered, spline-fitted geometry.
void appendSegment(std::vector<InkVertex>& vertices, const InkSample& from, const InkSample& to,
                   const StrokeStyle& style);

} // namespace phvikapen::core
