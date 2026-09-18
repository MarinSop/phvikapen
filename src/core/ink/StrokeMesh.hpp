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

void appendSegment(std::vector<InkVertex>& vertices, const InkSample& from, const InkSample& to,
                   const StrokeStyle& style);

// The round tip of a pen, left where a line starts, ends or turns a corner.
void appendDisc(std::vector<InkVertex>& vertices, float x, float y, float radius,
                const Color& color);

void appendStroke(std::vector<InkVertex>& vertices, const Stroke& stroke);

}
