#include "core/ink/Stroke.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace phvikapen::core {
namespace {

[[nodiscard]] InkSample sampleAt(float x, float y) {
    return InkSample{.x = x, .y = y};
}

TEST(StrokeTest, EmptyStrokeHasNoBoundingBox) {
    const Stroke stroke{Uuid{}};

    EXPECT_TRUE(stroke.empty());
    EXPECT_FALSE(stroke.boundingBox().has_value());
}

TEST(StrokeTest, BoundingBoxCoversAllSamplesGrownByHalfTheWidth) {
    Stroke stroke{Uuid{}, StrokeStyle{.width = 4.0F}};
    stroke.append(sampleAt(10.0F, 20.0F));
    stroke.append(sampleAt(30.0F, 5.0F));
    stroke.append(sampleAt(-4.0F, 12.0F));

    EXPECT_EQ(stroke.boundingBox(),
              (Rect{.left = -6.0F, .top = 3.0F, .right = 32.0F, .bottom = 22.0F}));
}

TEST(StrokeTest, SingleSampleBoundingBoxIsAsWideAsTheStroke) {
    Stroke stroke{Uuid{}, StrokeStyle{.width = 3.0F}};
    stroke.append(sampleAt(7.0F, 9.0F));

    const auto box = stroke.boundingBox();
    ASSERT_TRUE(box.has_value());
    EXPECT_FLOAT_EQ(box->width(), 3.0F);
    EXPECT_FLOAT_EQ(box->height(), 3.0F);
}

TEST(StrokeTest, KeepsSamplesInInputOrder) {
    Stroke stroke{Uuid{}};
    const InkSample first{
        .x = 1.0F,
        .y = 2.0F,
        .pressure = 0.25F,
        .tiltX = 10.0F,
        .tiltY = -5.0F,
        .timestamp = std::chrono::microseconds{100},
    };
    const InkSample second{
        .x = 3.0F,
        .y = 4.0F,
        .pressure = 0.75F,
        .timestamp = std::chrono::microseconds{8'433},
    };
    stroke.append(first);
    stroke.append(second);

    ASSERT_EQ(stroke.samples().size(), 2U);
    EXPECT_EQ(stroke.samples().front(), first);
    EXPECT_EQ(stroke.samples().back(), second);
}

} // namespace
} // namespace phvikapen::core
