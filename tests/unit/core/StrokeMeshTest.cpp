#include "core/ink/StrokeMesh.hpp"

#include "core/id/Uuid.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace phvikapen::core {
namespace {

TEST(StrokeMeshTest, SegmentWidthFollowsPressureAtEachEnd) {
    std::vector<InkVertex> vertices;
    const StrokeStyle style{.color = Color{.red = 255}, .width = 4.0F};

    appendSegment(vertices, InkSample{.x = 0.0F, .y = 10.0F, .pressure = 1.0F},
                  InkSample{.x = 8.0F, .y = 10.0F, .pressure = 0.5F}, style);

    ASSERT_EQ(vertices.size(), 6U);
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

TEST(StrokeMeshTest, AStrokeIsOneStripWithoutGapsBetweenSegments) {
    std::vector<InkVertex> vertices;
    Stroke stroke{Uuid{}, StrokeStyle{.width = 4.0F}};
    for (const float x : {0.0F, 10.0F, 20.0F}) {
        stroke.append(InkSample{.x = x, .y = 10.0F});
    }

    appendStroke(vertices, stroke);

    ASSERT_FALSE(vertices.empty());
    ASSERT_EQ(vertices.size() % 6, 0U);
    for (std::size_t first = 0; first + 6 < vertices.size(); first += 6) {
        EXPECT_EQ(vertices[first + 2], vertices[first + 6]);
        EXPECT_EQ(vertices[first + 5], vertices[first + 7]);
    }
    for (const InkVertex& vertex : vertices) {
        EXPECT_NEAR(std::abs(vertex.y - 10.0F), 2.0F, 0.01F);
    }
}

TEST(StrokeMeshTest, AStrokeOfOneSampleIsADot) {
    std::vector<InkVertex> vertices;
    Stroke stroke{Uuid{}, StrokeStyle{.width = 2.0F}};
    stroke.append(InkSample{.x = 5.0F, .y = 5.0F});

    appendStroke(vertices, stroke);

    EXPECT_EQ(vertices.size(), 6U);
}

}
}
