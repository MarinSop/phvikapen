#pragma once

#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <vector>

namespace phvikapen::core {

struct InkVertex {
    float x{};
    float y{};
    float red{};
    float green{};
    float blue{};
    float alpha{};

    friend constexpr bool operator==(const InkVertex&, const InkVertex&) = default;
};

// TODO(M2): Replace with spline-fitted geometry.
void appendSegment(std::vector<InkVertex>& vertices, const InkSample& from, const InkSample& to,
                   const StrokeStyle& style);

}
