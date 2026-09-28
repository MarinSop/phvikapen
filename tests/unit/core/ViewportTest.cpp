#include "core/geometry/Viewport.hpp"

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/model/PageStyle.hpp"

#include <gtest/gtest.h>

#include <optional>

namespace phvikapen::core {
namespace {

constexpr ViewSize kLaptopView{.width = 1280.0F, .height = 760.0F};
constexpr Point kCorner{.x = 0.0F, .y = 0.0F};

TEST(ViewportTest, StartsShowingThePageAtItsNaturalSize) {
    const Viewport viewport;

    EXPECT_EQ(viewport.toPage({.x = 100.0F, .y = 50.0F}), (Point{.x = 100.0F, .y = 50.0F}));
    EXPECT_FLOAT_EQ(viewport.scale(), 1.0F);
}

TEST(ViewportTest, PanningMovesThePageWithTheFingers) {
    Viewport viewport;

    viewport.panBy(-30.0F, -40.0F);

    EXPECT_EQ(viewport.toPage({.x = 0.0F, .y = 0.0F}), (Point{.x = 30.0F, .y = 40.0F}));
}

TEST(ViewportTest, ZoomingKeepsThePointUnderTheCursorInPlace) {
    Viewport viewport;
    viewport.panBy(-100.0F, -100.0F);
    const Point cursor{.x = 400.0F, .y = 300.0F};
    const Point before = viewport.toPage(cursor);

    viewport.zoomAround(cursor, 2.0F);

    EXPECT_FLOAT_EQ(viewport.scale(), 2.0F);
    const Point after = viewport.toPage(cursor);
    EXPECT_FLOAT_EQ(after.x, before.x);
    EXPECT_FLOAT_EQ(after.y, before.y);
    const Point back = viewport.toView(after);
    EXPECT_FLOAT_EQ(back.x, cursor.x);
    EXPECT_FLOAT_EQ(back.y, cursor.y);
}

TEST(ViewportTest, KeepsTheZoomWithinBounds) {
    Viewport viewport;

    viewport.zoomAround(kCorner, 1000.0F);
    EXPECT_FLOAT_EQ(viewport.scale(), Viewport::kMaximumScale);

    viewport.zoomAround(kCorner, 0.0001F);
    EXPECT_FLOAT_EQ(viewport.scale(), Viewport::kMinimumScale);
}

TEST(ViewportTest, FitsAnInfiniteCanvasAtItsNaturalSize) {
    Viewport viewport;
    viewport.zoomAround(kCorner, 3.0F);

    viewport.fit(kLaptopView, std::nullopt);

    EXPECT_EQ(viewport, Viewport{});
}

TEST(ViewportTest, CentersPaperThatIsNarrowerThanTheView) {
    Viewport viewport;
    const PaperSize a4 = paperSize(PageStyle{}).value_or(PaperSize{});

    viewport.fit(kLaptopView, a4);

    EXPECT_FLOAT_EQ(viewport.scale(), 1.0F);
    const Point left = viewport.toView({.x = 0.0F, .y = 0.0F});
    const Point right = viewport.toView({.x = a4.width, .y = 0.0F});
    EXPECT_NEAR(left.x, kLaptopView.width - right.x, 0.01F);
    EXPECT_NEAR(left.y, Viewport::kPaperMargin, 0.01F);
}

TEST(ViewportTest, ShrinksPaperThatIsWiderThanTheView) {
    Viewport viewport;
    const ViewSize narrow{.width = 600.0F, .height = 800.0F};
    const PaperSize a4 = paperSize(PageStyle{}).value_or(PaperSize{});

    viewport.fit(narrow, a4);

    EXPECT_LT(viewport.scale(), 1.0F);
    const Point left = viewport.toView({.x = 0.0F, .y = 0.0F});
    const Point right = viewport.toView({.x = a4.width, .y = 0.0F});
    EXPECT_NEAR(left.x, Viewport::kPaperMargin, 0.01F);
    EXPECT_NEAR(right.x, narrow.width - Viewport::kPaperMargin, 0.01F);
}

TEST(ViewportTest, KeepsPaperFromDriftingOutOfView) {
    Viewport viewport;
    const PaperSize a4 = paperSize(PageStyle{}).value_or(PaperSize{});
    viewport.fit(kLaptopView, a4);

    viewport.panBy(5000.0F, 5000.0F);
    viewport.keepPaperInView(kLaptopView, a4);
    const Rect farUp = viewport.visiblePage(kLaptopView);
    EXPECT_TRUE(
        farUp.intersects({.left = 0.0F, .top = 0.0F, .right = a4.width, .bottom = a4.height}));

    viewport.panBy(-10000.0F, -10000.0F);
    viewport.keepPaperInView(kLaptopView, a4);
    const Rect farDown = viewport.visiblePage(kLaptopView);
    EXPECT_TRUE(
        farDown.intersects({.left = 0.0F, .top = 0.0F, .right = a4.width, .bottom = a4.height}));
}

TEST(ViewportTest, LeavesAnAreaAlreadyInViewWhereItStands) {
    Viewport viewport;
    const Rect seen = viewport.visiblePage(kLaptopView);
    const Viewport was = viewport;

    viewport.bringIntoView(kLaptopView,
                           {.left = 200.0F, .top = 200.0F, .right = 260.0F, .bottom = 220.0F});

    EXPECT_EQ(viewport, was);
    EXPECT_EQ(viewport.visiblePage(kLaptopView), seen);
}

TEST(ViewportTest, MovesTheLeastItCanToBringAnAreaBelowIntoView) {
    Viewport viewport;
    const Rect area{.left = 100.0F, .top = 2000.0F, .right = 180.0F, .bottom = 2030.0F};

    viewport.bringIntoView(kLaptopView, area);

    const Rect seen = viewport.visiblePage(kLaptopView);
    EXPECT_TRUE(seen.top <= area.top && seen.bottom >= area.bottom);
    // The area is brought to the bottom edge, not to the middle, because that is the least move.
    EXPECT_NEAR(seen.bottom - area.bottom, Viewport::kPaperMargin, 0.01F);
}

TEST(ViewportTest, MovesTheLeastItCanToBringAnAreaAboveIntoView) {
    Viewport viewport;
    viewport.showTop(3000.0F);
    const Rect area{.left = 100.0F, .top = 1000.0F, .right = 180.0F, .bottom = 1030.0F};

    viewport.bringIntoView(kLaptopView, area);

    const Rect seen = viewport.visiblePage(kLaptopView);
    EXPECT_TRUE(seen.top <= area.top && seen.bottom >= area.bottom);
    EXPECT_NEAR(area.top - seen.top, Viewport::kPaperMargin, 0.01F);
}

TEST(ViewportTest, BringsAnAreaToTheSideIntoView) {
    Viewport viewport;
    const Rect area{.left = 4000.0F, .top = 100.0F, .right = 4080.0F, .bottom = 130.0F};

    viewport.bringIntoView(kLaptopView, area);

    const Rect seen = viewport.visiblePage(kLaptopView);
    EXPECT_TRUE(seen.left <= area.left && seen.right >= area.right);
    EXPECT_NEAR(seen.top, 0.0F, 0.01F);
}

TEST(ViewportTest, ShowsAnAreaTooLargeToFitFromItsStart) {
    Viewport viewport;
    const Rect area{.left = 0.0F, .top = 500.0F, .right = 4000.0F, .bottom = 5000.0F};

    viewport.bringIntoView(kLaptopView, area);

    const Rect seen = viewport.visiblePage(kLaptopView);
    EXPECT_NEAR(seen.left, area.left, 0.01F);
    EXPECT_NEAR(seen.top, area.top, 0.01F);
}

TEST(ViewportTest, ReadsAnAreaGivenTheOtherWayAround) {
    Viewport viewport;
    Viewport backwards;
    const Rect area{.left = 100.0F, .top = 2000.0F, .right = 180.0F, .bottom = 2030.0F};

    viewport.bringIntoView(kLaptopView, area);
    backwards.bringIntoView(
        kLaptopView,
        {.left = area.right, .top = area.bottom, .right = area.left, .bottom = area.top});

    EXPECT_EQ(viewport, backwards);
}

TEST(ViewportTest, BringsNothingIntoAViewWithNoRoom) {
    Viewport viewport;
    const Viewport was = viewport;

    viewport.bringIntoView({.width = 0.0F, .height = 0.0F},
                           {.left = 0.0F, .top = 9000.0F, .right = 10.0F, .bottom = 9010.0F});

    EXPECT_EQ(viewport, was);
}

TEST(ViewportTest, LeavesAnInfiniteCanvasFreeToRoam) {
    Viewport viewport;

    viewport.panBy(-5000.0F, 7000.0F);
    viewport.keepPaperInView(kLaptopView, std::nullopt);

    EXPECT_EQ(viewport.toPage({}), (Point{.x = 5000.0F, .y = -7000.0F}));
}

}
}
