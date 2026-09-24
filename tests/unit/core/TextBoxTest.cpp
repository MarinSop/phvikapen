#include "core/model/TextBox.hpp"

#include "core/Error.hpp"
#include "core/geometry/Distance.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/Page.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] TextBox makeBox(Uuid7Generator& ids, float x, float y, const char* what = "Hello") {
    return TextBox{
        .id = ids.next(),
        .at = Point{.x = x, .y = y},
        .width = 100.0F,
        .height = 20.0F,
        .text = what,
        .style = TextStyle{},
    };
}

TEST(TextBoxTest, MeasuresTheRoomItTakesOnThePage) {
    Uuid7Generator ids;
    const TextBox box = makeBox(ids, 10.0F, 20.0F);

    const Rect area = areaOf(box);

    EXPECT_FLOAT_EQ(area.left, 10.0F);
    EXPECT_FLOAT_EQ(area.top, 20.0F);
    EXPECT_FLOAT_EQ(area.right, 110.0F);
    EXPECT_FLOAT_EQ(area.bottom, 40.0F);
}

TEST(TextBoxTest, TurnsPointsIntoPageUnitsAndBack) {
    EXPECT_FLOAT_EQ(pageUnitsOfPoints(72.0F), 96.0F);
    EXPECT_FLOAT_EQ(pointsOfPageUnits(96.0F), 72.0F);
    EXPECT_FLOAT_EQ(pointsOfPageUnits(pageUnitsOfPoints(14.0F)), 14.0F);
}

TEST(TextBoxTest, BringsWildValuesBackIntoRange) {
    TextStyle style;
    style.size = std::numeric_limits<float>::quiet_NaN();
    style.lineHeight = 100.0F;

    const TextStyle kept = normalized(style);

    EXPECT_FLOAT_EQ(kept.size, TextStyle::kDefaultSize);
    EXPECT_FLOAT_EQ(kept.lineHeight, TextStyle::kLoosestLines);
}

TEST(TextBoxTest, KeepsABoxWideEnoughToHoldWriting) {
    Uuid7Generator ids;
    TextBox box = makeBox(ids, 0.0F, 0.0F);
    box.width = 1.0F;
    box.height = -5.0F;

    const TextBox kept = normalized(box);

    EXPECT_FLOAT_EQ(kept.width, TextBox::kNarrowest);
    EXPECT_FLOAT_EQ(kept.height, 0.0F);
}

TEST(PageTextTest, StartsWithNoTextAndTheFirstPlaceFree) {
    Uuid7Generator ids;
    const Page page{ids.next()};

    EXPECT_TRUE(page.texts().empty());
    EXPECT_EQ(page.nextTextOrdinal(), 0);
}

TEST(PageTextTest, HoldsTextInTheOrderItWasPut) {
    Uuid7Generator ids;
    const PlacedText early{.ordinal = 1, .box = makeBox(ids, 0.0F, 0.0F, "first")};
    const PlacedText late{.ordinal = 4, .box = makeBox(ids, 0.0F, 40.0F, "second")};

    const Page page{ids.next(), {}, {late, early}};

    ASSERT_EQ(page.texts().size(), 2U);
    EXPECT_EQ(page.texts()[0].box.text, "first");
    EXPECT_EQ(page.texts()[1].box.text, "second");
    EXPECT_EQ(page.nextTextOrdinal(), 5);
}

TEST(PageTextTest, RefusesTwoBoxesInTheSamePlace) {
    Uuid7Generator ids;
    Page page{ids.next()};
    ASSERT_TRUE(page.insertText(PlacedText{.ordinal = 0, .box = makeBox(ids, 0.0F, 0.0F)}));

    const Result<void> again =
        page.insertText(PlacedText{.ordinal = 0, .box = makeBox(ids, 5.0F, 5.0F)});

    ASSERT_FALSE(again);
    EXPECT_EQ(again.error().code, ErrorCode::InvalidArgument);
}

TEST(PageTextTest, RefusesTheSameBoxTwice) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const TextBox box = makeBox(ids, 0.0F, 0.0F);
    ASSERT_TRUE(page.insertText(PlacedText{.ordinal = 0, .box = box}));

    const Result<void> again = page.insertText(PlacedText{.ordinal = 1, .box = box});

    ASSERT_FALSE(again);
    EXPECT_EQ(again.error().code, ErrorCode::InvalidArgument);
}

TEST(PageTextTest, TakesABoxBackOff) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const TextBox box = makeBox(ids, 0.0F, 0.0F);
    ASSERT_TRUE(page.insertText(PlacedText{.ordinal = 0, .box = box}));

    const Result<PlacedText> removed = page.removeText(box.id);

    ASSERT_TRUE(removed);
    EXPECT_EQ(removed->box.id, box.id);
    EXPECT_TRUE(page.texts().empty());
    EXPECT_FALSE(page.removeText(box.id));
}

TEST(PageTextTest, WritesOverABoxThatIsAlreadyThere) {
    Uuid7Generator ids;
    Page page{ids.next()};
    TextBox box = makeBox(ids, 0.0F, 0.0F);
    ASSERT_TRUE(page.insertText(PlacedText{.ordinal = 0, .box = box}));

    box.text = "changed";
    box.style.bold = true;
    ASSERT_TRUE(page.replaceText(box));

    const TextBox* const kept = page.textAt(box.id);
    ASSERT_NE(kept, nullptr);
    EXPECT_EQ(kept->text, "changed");
    EXPECT_TRUE(kept->style.bold);
}

TEST(PageTextTest, SaysNothingOfABoxItDoesNotHold) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const TextBox stranger = makeBox(ids, 0.0F, 0.0F);

    EXPECT_EQ(page.textAt(stranger.id), nullptr);
    const Result<void> written = page.replaceText(stranger);
    ASSERT_FALSE(written);
    EXPECT_EQ(written.error().code, ErrorCode::NotFound);
}

TEST(PageTextTest, FindsTheBoxUnderATapAndPrefersTheLastOneWritten) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const TextBox under = makeBox(ids, 0.0F, 0.0F, "under");
    const TextBox over = makeBox(ids, 0.0F, 0.0F, "over");
    ASSERT_TRUE(page.insertText(PlacedText{.ordinal = 0, .box = under}));
    ASSERT_TRUE(page.insertText(PlacedText{.ordinal = 1, .box = over}));

    const TextBox* const hit = page.textUnder(Point{.x = 50.0F, .y = 10.0F});

    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit->text, "over");
    EXPECT_EQ(page.textUnder(Point{.x = 500.0F, .y = 10.0F}), nullptr);
}

}
}
