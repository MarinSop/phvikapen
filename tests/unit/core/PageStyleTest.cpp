#include "core/model/PageStyle.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <optional>

namespace phvikapen::core {
namespace {

TEST(PageStyleTest, AnInfinitePageHasNoSize) {
    EXPECT_FALSE(paperSize(Paper::Infinite, Orientation::Portrait).has_value());
    EXPECT_FALSE(paperSize(Paper::Infinite, Orientation::Landscape).has_value());
}

TEST(PageStyleTest, PaperSizesAreInPageUnitsOfOneNinetySixthOfAnInch) {
    const std::optional<PaperSize> a4 = paperSize(Paper::A4, Orientation::Portrait);
    ASSERT_TRUE(a4.has_value());
    EXPECT_NEAR(a4->width, 793.7F, 0.1F);
    EXPECT_NEAR(a4->height, 1122.5F, 0.1F);

    const std::optional<PaperSize> letter = paperSize(Paper::Letter, Orientation::Portrait);
    ASSERT_TRUE(letter.has_value());
    EXPECT_FLOAT_EQ(letter->width, 816.0F);
    EXPECT_FLOAT_EQ(letter->height, 1056.0F);
}

TEST(PageStyleTest, LandscapeSwapsWidthAndHeight) {
    const std::optional<PaperSize> portrait = paperSize(Paper::A5, Orientation::Portrait);
    const std::optional<PaperSize> landscape = paperSize(Paper::A5, Orientation::Landscape);
    ASSERT_TRUE(portrait.has_value());
    ASSERT_TRUE(landscape.has_value());

    EXPECT_FLOAT_EQ(landscape->width, portrait->height);
    EXPECT_FLOAT_EQ(landscape->height, portrait->width);
}

TEST(PageStyleTest, EveryFixedPaperIsTallerThanWideInPortrait) {
    for (const Paper paper : {Paper::A3, Paper::A4, Paper::A5, Paper::Letter, Paper::Legal}) {
        const std::optional<PaperSize> size = paperSize(paper, Orientation::Portrait);
        ASSERT_TRUE(size.has_value());
        EXPECT_GT(size->height, size->width);
    }
}

TEST(PageStyleTest, KeepsTheLineSpacingWithinUsableBounds) {
    EXPECT_FLOAT_EQ(normalized(PageStyle{.spacing = 0.0F}).spacing, PageStyle::kMinimumSpacing);
    EXPECT_FLOAT_EQ(normalized(PageStyle{.spacing = 1e6F}).spacing, PageStyle::kMaximumSpacing);
    EXPECT_FLOAT_EQ(
        normalized(PageStyle{.spacing = std::numeric_limits<float>::quiet_NaN()}).spacing,
        PageStyle::kDefaultSpacing);

    const PageStyle usual{};
    EXPECT_EQ(normalized(usual), usual);
}

}
}
