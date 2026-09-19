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

TEST(StrokeMeshTest, ZeroLengthSegmentProducesARoundDot) {
    std::vector<InkVertex> vertices;
    const InkSample point{.x = 5.0F, .y = 5.0F};

    appendSegment(vertices, point, point, StrokeStyle{.width = 2.0F});

    ASSERT_FALSE(vertices.empty());
    ASSERT_EQ(vertices.size() % 3, 0U);
    for (const InkVertex& vertex : vertices) {
        EXPECT_LE(std::hypot(vertex.x - 5.0F, vertex.y - 5.0F), 1.0F + 0.01F);
    }
    const auto [minX, maxX] = std::ranges::minmax(vertices, {}, &InkVertex::x);
    const auto [minY, maxY] = std::ranges::minmax(vertices, {}, &InkVertex::y);
    EXPECT_FLOAT_EQ(minX.x, 4.0F);
    EXPECT_FLOAT_EQ(maxX.x, 6.0F);
    EXPECT_FLOAT_EQ(minY.y, 4.0F);
    EXPECT_FLOAT_EQ(maxY.y, 6.0F);
}

TEST(StrokeMeshTest, AStraightStrokeStaysWithinItsOwnWidth) {
    std::vector<InkVertex> vertices;
    Stroke stroke{Uuid{}, StrokeStyle{.width = 4.0F}};
    for (const float x : {0.0F, 10.0F, 20.0F}) {
        stroke.append(InkSample{.x = x, .y = 10.0F});
    }

    appendStroke(vertices, stroke);

    ASSERT_FALSE(vertices.empty());
    ASSERT_EQ(vertices.size() % 3, 0U);
    for (const InkVertex& vertex : vertices) {
        EXPECT_LE(std::abs(vertex.y - 10.0F), 2.0F + 0.01F);
        EXPECT_GE(vertex.x, -2.0F - 0.01F);
        EXPECT_LE(vertex.x, 22.0F + 0.01F);
    }
}

TEST(StrokeMeshTest, BothEndsOfAStrokeAreRounded) {
    std::vector<InkVertex> vertices;
    Stroke stroke{Uuid{}, StrokeStyle{.width = 4.0F}};
    stroke.append(InkSample{.x = 0.0F, .y = 0.0F});
    stroke.append(InkSample{.x = 20.0F, .y = 0.0F});

    appendStroke(vertices, stroke);

    const auto reaches = [&vertices](float x) {
        return std::ranges::any_of(vertices, [x](const InkVertex& vertex) {
            return std::abs(vertex.x - x) < 0.01F && std::abs(vertex.y) < 0.01F;
        });
    };
    EXPECT_TRUE(reaches(-2.0F));
    EXPECT_TRUE(reaches(22.0F));
}

TEST(StrokeMeshTest, ACornerMeetsAtAPointInsteadOfBeingRoundedOff) {
    std::vector<InkVertex> vertices;
    Stroke corner{Uuid{}, StrokeStyle{.width = 6.0F}};
    corner.append(InkSample{.x = 0.0F, .y = 0.0F});
    corner.append(InkSample{.x = 20.0F, .y = 0.0F});
    corner.append(InkSample{.x = 20.0F, .y = 20.0F});

    appendStroke(vertices, corner);

    // The outside of a square turn reaches half the width out along both sides.
    EXPECT_TRUE(std::ranges::any_of(vertices, [](const InkVertex& vertex) {
        return std::abs(vertex.x - 23.0F) < 0.2F && std::abs(vertex.y + 3.0F) < 0.2F;
    })) << "the corner was rounded off instead of meeting at a point";
}

TEST(StrokeMeshTest, AVerySharpTurnKeepsThePenTipInstead) {
    std::vector<InkVertex> straight;
    Stroke line{Uuid{}, StrokeStyle{.width = 6.0F}};
    line.append(InkSample{.x = 0.0F, .y = 0.0F});
    line.append(InkSample{.x = 40.0F, .y = 0.0F});
    appendStroke(straight, line);

    std::vector<InkVertex> folded;
    Stroke back{Uuid{}, StrokeStyle{.width = 6.0F}};
    back.append(InkSample{.x = 0.0F, .y = 0.0F});
    back.append(InkSample{.x = 20.0F, .y = 0.0F});
    back.append(InkSample{.x = 0.0F, .y = 2.0F});
    appendStroke(folded, back);

    EXPECT_GT(folded.size(), straight.size());
}

TEST(StrokeMeshTest, SquareEndsStopWhereTheLineDoes) {
    std::vector<InkVertex> vertices;
    Stroke stroke{Uuid{}, StrokeStyle{.width = 4.0F, .roundEnds = false}};
    stroke.append(InkSample{.x = 0.0F, .y = 0.0F});
    stroke.append(InkSample{.x = 20.0F, .y = 0.0F});

    appendStroke(vertices, stroke);

    for (const InkVertex& vertex : vertices) {
        EXPECT_GE(vertex.x, -0.01F);
        EXPECT_LE(vertex.x, 20.01F);
    }
}

TEST(StrokeMeshTest, AStrokeOfOneSampleIsARoundDot) {
    std::vector<InkVertex> vertices;
    Stroke stroke{Uuid{}, StrokeStyle{.width = 2.0F}};
    stroke.append(InkSample{.x = 5.0F, .y = 5.0F});

    appendStroke(vertices, stroke);

    ASSERT_FALSE(vertices.empty());
    for (const InkVertex& vertex : vertices) {
        EXPECT_LE(std::hypot(vertex.x - 5.0F, vertex.y - 5.0F), 1.0F + 0.01F);
    }
}
}
}
