#include "core/model/StrokeGrid.hpp"

#include "core/geometry/Rect.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace phvikapen::core {
namespace {

TEST(StrokeGridTest, FindsWhatLiesInTheCellsOfAnArea) {
    StrokeGrid grid{100.0F};
    grid.insert(1, {.left = 10.0F, .top = 10.0F, .right = 20.0F, .bottom = 20.0F});
    grid.insert(2, {.left = 510.0F, .top = 510.0F, .right = 520.0F, .bottom = 520.0F});

    EXPECT_EQ(grid.query({.left = 0.0F, .top = 0.0F, .right = 50.0F, .bottom = 50.0F}),
              std::vector<std::int64_t>{1});
    EXPECT_EQ(grid.query({.left = 0.0F, .top = 0.0F, .right = 600.0F, .bottom = 600.0F}),
              (std::vector<std::int64_t>{1, 2}));
    EXPECT_TRUE(
        grid.query({.left = 300.0F, .top = 300.0F, .right = 350.0F, .bottom = 350.0F}).empty());
}

TEST(StrokeGridTest, ReportsAStrokeSpanningManyCellsOnce) {
    StrokeGrid grid{100.0F};
    grid.insert(7, {.left = -250.0F, .top = -250.0F, .right = 250.0F, .bottom = 250.0F});

    EXPECT_EQ(grid.query({.left = -300.0F, .top = -300.0F, .right = 300.0F, .bottom = 300.0F}),
              std::vector<std::int64_t>{7});
    EXPECT_EQ(grid.query({.left = -210.0F, .top = -210.0F, .right = -205.0F, .bottom = -205.0F}),
              std::vector<std::int64_t>{7});
}

TEST(StrokeGridTest, ForgetsWhatWasRemoved) {
    StrokeGrid grid{100.0F};
    const Rect bounds{.left = 0.0F, .top = 0.0F, .right = 350.0F, .bottom = 20.0F};
    grid.insert(1, bounds);
    grid.insert(2, bounds);

    grid.remove(1, bounds);

    EXPECT_EQ(grid.query(bounds), std::vector<std::int64_t>{2});

    grid.clear();

    EXPECT_TRUE(grid.query(bounds).empty());
}

}
}
