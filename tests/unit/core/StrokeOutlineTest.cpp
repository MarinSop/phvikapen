#include "core/ink/StrokeOutline.hpp"

#include "core/geometry/Distance.hpp"
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

[[nodiscard]] float area(const std::vector<Point>& outline) {
    float twice = 0.0F;
    for (std::size_t i = 0; i < outline.size(); ++i) {
        const Point& from = outline[i];
        const Point& to = outline[(i + 1) % outline.size()];
        twice += (from.x * to.y) - (to.x * from.y);
    }
    return std::abs(twice) / 2.0F;
}

TEST(StrokeOutlineTest, AnEmptyStrokeHasNoOutline) {
    EXPECT_TRUE(strokeOutline(Stroke{Uuid{}}).empty());
}

TEST(StrokeOutlineTest, ASingleSampleBecomesARoundDot) {
    Stroke stroke{Uuid{}, StrokeStyle{.width = 4.0F}};
    stroke.append(InkSample{.x = 10.0F, .y = 20.0F});

    const std::vector<Point> outline = strokeOutline(stroke);

    ASSERT_FALSE(outline.empty());
    for (const Point& corner : outline) {
        EXPECT_NEAR(std::hypot(corner.x - 10.0F, corner.y - 20.0F), 2.0F, 1e-3F);
    }
}

TEST(StrokeOutlineTest, AStraightStrokeIsAsWideAsItsPen) {
    Stroke stroke{Uuid{}, StrokeStyle{.width = 6.0F}};
    for (const float x : {0.0F, 20.0F, 40.0F}) {
        stroke.append(InkSample{.x = x, .y = 50.0F});
    }

    const std::vector<Point> outline = strokeOutline(stroke);

    ASSERT_GE(outline.size(), 4U);
    const auto [lowest, highest] = std::ranges::minmax(outline, {}, &Point::y);
    EXPECT_NEAR(lowest.y, 47.0F, 0.05F);
    EXPECT_NEAR(highest.y, 53.0F, 0.05F);
    EXPECT_NEAR(area(outline), 40.0F * 6.0F, 1.0F);
}

TEST(StrokeOutlineTest, TheOutlineNarrowsWherePressureDrops) {
    Stroke stroke{Uuid{}, StrokeStyle{.width = 10.0F}};
    stroke.append(InkSample{.x = 0.0F, .y = 0.0F, .pressure = 1.0F});
    stroke.append(InkSample{.x = 30.0F, .y = 0.0F, .pressure = 1.0F});
    stroke.append(InkSample{.x = 60.0F, .y = 0.0F, .pressure = 0.2F});

    const std::vector<Point> outline = strokeOutline(stroke);

    ASSERT_FALSE(outline.empty());
    const auto atStart = std::ranges::max(
        outline, {}, [](const Point& point) { return point.x < 1.0F ? point.y : 0.0F; });
    const auto atEnd = std::ranges::max(
        outline, {}, [](const Point& point) { return point.x > 59.0F ? point.y : 0.0F; });
    EXPECT_NEAR(atStart.y, 5.0F, 0.1F);
    EXPECT_LT(atEnd.y, atStart.y / 2.0F);
}

TEST(StrokeOutlineTest, BothSidesOfTheStrokeAreInTheOutline) {
    Stroke stroke{Uuid{}, StrokeStyle{.width = 2.0F}};
    for (const float step : {0.0F, 10.0F, 20.0F, 30.0F}) {
        stroke.append(InkSample{.x = step, .y = step});
    }

    const std::vector<Point> outline = strokeOutline(stroke);

    ASSERT_EQ(outline.size() % 2, 0U);
    const std::size_t half = outline.size() / 2;
    for (std::size_t i = 0; i < half; ++i) {
        const Point& left = outline[i];
        const Point& right = outline[outline.size() - 1 - i];
        EXPECT_NEAR(std::hypot(left.x - right.x, left.y - right.y), 2.0F, 1e-3F);
    }
}

}
}
