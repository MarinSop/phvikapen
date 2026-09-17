#include "core/ink/StrokeHitTest.hpp"

#include "core/geometry/Distance.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"

#include <gtest/gtest.h>

#include <initializer_list>

namespace phvikapen::core {
namespace {

[[nodiscard]] Stroke strokeThrough(std::initializer_list<Point> points, float width = 2.0F) {
    Uuid7Generator ids;
    Stroke stroke{ids.next(), StrokeStyle{.width = width}};
    for (const Point point : points) {
        stroke.append(InkSample{.x = point.x, .y = point.y});
    }
    return stroke;
}

TEST(StrokeHitTestTest, AnEraserRestingOnALineTouchesIt) {
    const Stroke line = strokeThrough({{.x = 0.0F, .y = 0.0F}, {.x = 100.0F, .y = 0.0F}});

    EXPECT_TRUE(touches(
        line, {.from = {.x = 50.0F, .y = 5.0F}, .to = {.x = 50.0F, .y = 5.0F}, .radius = 4.5F}));
    EXPECT_FALSE(touches(
        line, {.from = {.x = 50.0F, .y = 6.0F}, .to = {.x = 50.0F, .y = 6.0F}, .radius = 4.5F}));
}

TEST(StrokeHitTestTest, AFastSweepAcrossALineTouchesItBetweenSamples) {
    const Stroke line = strokeThrough({{.x = 0.0F, .y = 0.0F}, {.x = 100.0F, .y = 0.0F}});

    EXPECT_TRUE(touches(
        line, {.from = {.x = 50.0F, .y = -80.0F}, .to = {.x = 50.0F, .y = 80.0F}, .radius = 1.0F}));
}

TEST(StrokeHitTestTest, TheWidthOfTheStrokeCounts) {
    const Stroke thick = strokeThrough({{.x = 0.0F, .y = 0.0F}, {.x = 100.0F, .y = 0.0F}}, 20.0F);

    EXPECT_TRUE(touches(
        thick, {.from = {.x = 50.0F, .y = 12.0F}, .to = {.x = 50.0F, .y = 12.0F}, .radius = 3.0F}));
}

TEST(StrokeHitTestTest, ADotIsTouchedAroundItsOnlySample) {
    const Stroke dot = strokeThrough({{.x = 30.0F, .y = 30.0F}});

    EXPECT_TRUE(touches(
        dot, {.from = {.x = 33.0F, .y = 30.0F}, .to = {.x = 33.0F, .y = 30.0F}, .radius = 2.5F}));
    EXPECT_FALSE(touches(
        dot, {.from = {.x = 40.0F, .y = 30.0F}, .to = {.x = 40.0F, .y = 30.0F}, .radius = 2.5F}));
}

TEST(StrokeHitTestTest, AStrokeWithoutSamplesIsNeverTouched) {
    const Stroke empty = strokeThrough({});

    EXPECT_FALSE(touches(empty, {.from = {}, .to = {}, .radius = 100.0F}));
}

}
}
