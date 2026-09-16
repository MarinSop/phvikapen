#include "core/ink/StrokeMesh.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace phvikapen::core {
namespace {

TEST(StrokeMeshTest, SegmentWidthFollowsPressureAtEachEnd) {
    std::vector<InkVertex> vertices;
    const StrokeStyle style{.color = Color{.red = 255}, .width = 4.0F};

    appendSegment(vertices, InkSample{.x = 0.0F, .y = 10.0F, .pressure = 1.0F},
                  InkSample{.x = 8.0F, .y = 10.0F, .pressure = 0.5F}, style);

    ASSERT_EQ(vertices.size(), 6U);
    // Triangles: (fromLeft, fromRight, toLeft) and (toLeft, fromRight, toRight).
    EXPECT_EQ(vertices.front(), (InkVertex{.x = 0.0F, .y = 12.0F, .red = 1.0F, .alpha = 1.0F}));
    EXPECT_FLOAT_EQ(vertices.at(1).y, 8.0F);
    EXPECT_FLOAT_EQ(vertices.at(2).x, 8.0F);
    EXPECT_FLOAT_EQ(vertices.at(2).y, 11.0F);
    EXPECT_FLOAT_EQ(vertices.back().y, 9.0F);
}

TEST(StrokeMeshTest, ZeroLengthSegmentProducesSquareDot) {
    std::vector<InkVertex> vertices;
    const InkSample point{.x = 5.0F, .y = 5.0F};

    appendSegment(vertices, point, point, StrokeStyle{.width = 2.0F});

    ASSERT_EQ(vertices.size(), 6U);
    const auto [minX, maxX] = std::ranges::minmax(vertices, {}, &InkVertex::x);
    const auto [minY, maxY] = std::ranges::minmax(vertices, {}, &InkVertex::y);
    EXPECT_FLOAT_EQ(minX.x, 4.0F);
    EXPECT_FLOAT_EQ(maxX.x, 6.0F);
    EXPECT_FLOAT_EQ(minY.y, 4.0F);
    EXPECT_FLOAT_EQ(maxY.y, 6.0F);
}

} // namespace
} // namespace phvikapen::core
