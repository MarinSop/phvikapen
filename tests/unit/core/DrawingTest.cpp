#include "core/math/Drawing.hpp"

#include "core/math/Equation.hpp"
#include "core/math/Reading.hpp"
#include "core/model/TextBox.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>

namespace phvikapen::core {
namespace {

constexpr float kSize = 12.0F;
// Every letter is reckoned half as wide as the type is tall, so that what is laid out can be
// checked without asking a window how wide anything really is.
constexpr float kLetterShare = 0.5F;

[[nodiscard]] Measure plainly() {
    return [](std::string_view said, float size) {
        return static_cast<float>(said.size()) * pageUnitsOfPoints(size) * kLetterShare;
    };
}

[[nodiscard]] Drawing drawnOf(std::string_view written) {
    const Result<Equation> equation = equationOf(tidied(written));
    return equation ? laidOut(*equation, kSize, plainly()) : Drawing{};
}

[[nodiscard]] std::string saidBy(const Drawing& drawing) {
    std::string said;
    for (const Glyph& glyph : drawing.glyphs) {
        said.append(glyph.text);
    }
    return said;
}

[[nodiscard]] const Glyph* glyphSaying(const Drawing& drawing, std::string_view said) {
    const auto found = std::ranges::find_if(drawing.glyphs,
                                            [&](const Glyph& glyph) { return glyph.text == said; });
    return found == drawing.glyphs.end() ? nullptr : &*found;
}

TEST(DrawingTest, ASumIsLaidOutAcrossWithItsSignBetween) {
    const Drawing drawn = drawnOf("2+3");

    EXPECT_EQ(saidBy(drawn), "2+3");
    ASSERT_EQ(drawn.glyphs.size(), 3U);
    // Each piece stands to the right of the one before it.
    EXPECT_LT(drawn.glyphs[0].at.x, drawn.glyphs[1].at.x);
    EXPECT_LT(drawn.glyphs[1].at.x, drawn.glyphs[2].at.x);
    EXPECT_TRUE(drawn.bars.empty());
    EXPECT_GT(drawn.width, 0.0F);
}

TEST(DrawingTest, DivisionIsDrawnAsAFractionWithABarBetween) {
    const Drawing drawn = drawnOf("3/4");

    ASSERT_EQ(drawn.bars.size(), 1U);
    const Glyph* const over = glyphSaying(drawn, "3");
    const Glyph* const under = glyphSaying(drawn, "4");
    ASSERT_NE(over, nullptr);
    ASSERT_NE(under, nullptr);
    // One part stands over the bar and the other under it.
    EXPECT_LT(over->at.y, drawn.bars.front().area.top);
    EXPECT_GT(under->at.y, drawn.bars.front().area.bottom);
    EXPECT_FLOAT_EQ(drawn.bars.front().area.left, 0.0F);
    EXPECT_FLOAT_EQ(drawn.bars.front().area.right, drawn.width);
}

TEST(DrawingTest, AFractionIsTallerThanWhatItIsMadeOf) {
    const Drawing plain = drawnOf("3");
    const Drawing fraction = drawnOf("3/4");

    EXPECT_GT(fraction.height, plain.height * 2.0F);
    // The middle of a fraction is its bar, so that a sum beside it lines up there.
    EXPECT_NEAR(fraction.middle, fraction.bars.front().area.top, fraction.height);
}

TEST(DrawingTest, APowerIsSetSmallerAndRaised) {
    const Drawing drawn = drawnOf("2^3");

    const Glyph* const base = glyphSaying(drawn, "2");
    const Glyph* const raised = glyphSaying(drawn, "3");
    ASSERT_NE(base, nullptr);
    ASSERT_NE(raised, nullptr);
    EXPECT_LT(raised->size, base->size);
    EXPECT_LT(raised->at.y, base->at.y);
    EXPECT_GT(raised->at.x, base->at.x);
}

TEST(DrawingTest, ARootIsDrawnUnderARoof) {
    const Drawing drawn = drawnOf("sqrt(9)");

    ASSERT_EQ(drawn.bars.size(), 1U);
    const Glyph* const sign = glyphSaying(drawn, "√");
    const Glyph* const under = glyphSaying(drawn, "9");
    ASSERT_NE(sign, nullptr);
    ASSERT_NE(under, nullptr);
    // The roof runs over what the root covers and no further.
    EXPECT_GT(drawn.bars.front().area.left, 0.0F);
    EXPECT_FLOAT_EQ(drawn.bars.front().area.right, drawn.width);
    EXPECT_GT(under->at.y, drawn.bars.front().area.bottom);
}

TEST(DrawingTest, BracketsArePutBackWhereTheShapeNeedsThem) {
    EXPECT_EQ(saidBy(drawnOf("(2+3)*4")), "(2+3)×4");
    EXPECT_EQ(saidBy(drawnOf("2*3+4")), "2×3+4");
    EXPECT_EQ(saidBy(drawnOf("2-(3-4)")), "2−(3−4)");
}

TEST(DrawingTest, AnythingButAPlainNumberIsBracketedBeforeItIsRaised) {
    EXPECT_EQ(saidBy(drawnOf("2^3")), "23");
    EXPECT_EQ(saidBy(drawnOf("(1+2)^3")), "(1+2)3");
    // A fraction raised to a power says nothing without brackets.
    EXPECT_EQ(saidBy(drawnOf("(3/4)^2")), "(34)2");
}

TEST(DrawingTest, APowerSitsNearTheTopOfWhatItIsRaisedOver) {
    const Drawing drawn = drawnOf("(3/4)^2");

    const Glyph* const raised = glyphSaying(drawn, "2");
    ASSERT_NE(raised, nullptr);
    // Well above the bar of the fraction, rather than beside it.
    ASSERT_FALSE(drawn.bars.empty());
    EXPECT_LT(raised->at.y, drawn.bars.front().area.top);
}

TEST(DrawingTest, AFractionNeedsNoBracketsOfItsOwn) {
    // The bar holds the parts together, so nothing is bracketed inside or around it.
    EXPECT_EQ(saidBy(drawnOf("(1+2)/(3+4)")), "1+23+4");
    EXPECT_EQ(saidBy(drawnOf("1/2+3")), "12+3");
}

TEST(DrawingTest, NothingAtAllIsDrawnForAnEquationThatIsNotThere) {
    const Drawing drawn = laidOut(Equation{}, kSize, plainly());

    EXPECT_TRUE(drawn.glyphs.empty());
    EXPECT_FLOAT_EQ(drawn.width, 0.0F);
    EXPECT_TRUE(laidOut(numberOf(1.0), kSize, Measure{}).glyphs.empty());
}

TEST(DrawingTest, ADrawingCarriedToAnotherPlaceTakesEveryPieceWithIt) {
    const Drawing drawn = drawnOf("1/2");
    const Drawing moved = movedBy(drawn, 10.0F, 20.0F);

    ASSERT_EQ(moved.glyphs.size(), drawn.glyphs.size());
    EXPECT_FLOAT_EQ(moved.glyphs.front().at.x, drawn.glyphs.front().at.x + 10.0F);
    EXPECT_FLOAT_EQ(moved.glyphs.front().at.y, drawn.glyphs.front().at.y + 20.0F);
    EXPECT_FLOAT_EQ(moved.bars.front().area.left, drawn.bars.front().area.left + 10.0F);
    EXPECT_FLOAT_EQ(moved.bars.front().area.top, drawn.bars.front().area.top + 20.0F);
}

}
}
