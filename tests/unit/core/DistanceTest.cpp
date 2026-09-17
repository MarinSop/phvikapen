#include "core/geometry/Distance.hpp"

#include <gtest/gtest.h>

namespace phvikapen::core {
namespace {

TEST(DistanceTest, MeasuresToTheNearestPointOfASegment) {
    const Point start{.x = 0.0F, .y = 0.0F};
    const Point end{.x = 10.0F, .y = 0.0F};

    EXPECT_FLOAT_EQ(squaredDistanceToSegment({.x = 5.0F, .y = 3.0F}, start, end), 9.0F);
    EXPECT_FLOAT_EQ(squaredDistanceToSegment({.x = -3.0F, .y = 4.0F}, start, end), 25.0F);
    EXPECT_FLOAT_EQ(squaredDistanceToSegment({.x = 13.0F, .y = 0.0F}, start, end), 9.0F);
}

TEST(DistanceTest, TreatsASegmentWithoutLengthAsAPoint) {
    const Point dot{.x = 2.0F, .y = 2.0F};

    EXPECT_FLOAT_EQ(squaredDistanceToSegment({.x = 5.0F, .y = 6.0F}, dot, dot), 25.0F);
}

TEST(DistanceTest, CrossingSegmentsAreNoDistanceApart) {
    EXPECT_FLOAT_EQ(squaredDistanceBetweenSegments({.x = 0.0F, .y = 0.0F}, {.x = 10.0F, .y = 10.0F},
                                                   {.x = 0.0F, .y = 10.0F},
                                                   {.x = 10.0F, .y = 0.0F}),
                    0.0F);
}

TEST(DistanceTest, MeasuresBetweenSegmentsThatDoNotCross) {
    EXPECT_FLOAT_EQ(squaredDistanceBetweenSegments({.x = 0.0F, .y = 0.0F}, {.x = 10.0F, .y = 0.0F},
                                                   {.x = 0.0F, .y = 4.0F}, {.x = 10.0F, .y = 4.0F}),
                    16.0F);
    EXPECT_FLOAT_EQ(squaredDistanceBetweenSegments({.x = 0.0F, .y = 0.0F}, {.x = 10.0F, .y = 0.0F},
                                                   {.x = 13.0F, .y = -5.0F},
                                                   {.x = 13.0F, .y = 5.0F}),
                    9.0F);
}

TEST(DistanceTest, SegmentsThatTouchAtAnEndAreNoDistanceApart) {
    EXPECT_FLOAT_EQ(squaredDistanceBetweenSegments({.x = 0.0F, .y = 0.0F}, {.x = 10.0F, .y = 0.0F},
                                                   {.x = 10.0F, .y = 0.0F},
                                                   {.x = 10.0F, .y = 8.0F}),
                    0.0F);
}

}
}
