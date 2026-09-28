#include "core/geometry/Rect.hpp"

#include <gtest/gtest.h>

namespace phvikapen::core {
namespace {

constexpr Rect kLoop{.left = 0.0F, .top = 0.0F, .right = 100.0F, .bottom = 100.0F};

TEST(RectTest, HoldsABoxWhollyInsideIt) {
    EXPECT_TRUE(kLoop.contains({.left = 10.0F, .top = 10.0F, .right = 20.0F, .bottom = 20.0F}));
}

TEST(RectTest, HoldsABoxOnItsVeryEdge) {
    EXPECT_TRUE(kLoop.contains(kLoop));
    EXPECT_TRUE(kLoop.contains({.left = 0.0F, .top = 0.0F, .right = 100.0F, .bottom = 10.0F}));
}

TEST(RectTest, DoesNotHoldABoxThatHangsOverAnEdge) {
    EXPECT_FALSE(kLoop.contains({.left = 90.0F, .top = 10.0F, .right = 110.0F, .bottom = 20.0F}));
    EXPECT_FALSE(kLoop.contains({.left = -1.0F, .top = 10.0F, .right = 20.0F, .bottom = 20.0F}));
    EXPECT_FALSE(kLoop.contains({.left = 10.0F, .top = -1.0F, .right = 20.0F, .bottom = 20.0F}));
    EXPECT_FALSE(kLoop.contains({.left = 10.0F, .top = 10.0F, .right = 20.0F, .bottom = 101.0F}));
}

TEST(RectTest, DoesNotHoldABoxBesideIt) {
    EXPECT_FALSE(
        kLoop.contains({.left = 200.0F, .top = 200.0F, .right = 210.0F, .bottom = 210.0F}));
}

// Touching is not holding: two boxes that share an edge cross without one being inside the other.
TEST(RectTest, TellsHoldingApartFromCrossing) {
    constexpr Rect kOver{.left = 50.0F, .top = 50.0F, .right = 150.0F, .bottom = 150.0F};

    EXPECT_TRUE(kLoop.intersects(kOver));
    EXPECT_FALSE(kLoop.contains(kOver));
}

}
}
