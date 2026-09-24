#include "core/text/WrittenText.hpp"

#include "core/geometry/Rect.hpp"
#include "core/model/TextBox.hpp"
#include "core/text/InkWord.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] InkWord wordAt(std::string text, float left, float top, float width = 40.0F,
                             float height = 20.0F) {
    return InkWord{
        .text = std::move(text),
        .box =
            Rect{
                .left = left,
                .top = top,
                .right = left + width,
                .bottom = top + height,
            },
        .strokes = {},
    };
}

TEST(WrittenTextTest, MakesNothingOfNoWords) {
    EXPECT_FALSE(textOf({}).has_value());
}

TEST(WrittenTextTest, PutsTheWordsOfOneLineInReadingOrder) {
    const std::vector<InkWord> words{
        wordAt("world", 60.0F, 10.0F),
        wordAt("Hello", 10.0F, 12.0F),
    };

    const std::optional<TextBlock> block = textOf(words);

    ASSERT_TRUE(block.has_value());
    EXPECT_EQ(block->text, "Hello world");
}

TEST(WrittenTextTest, BreaksALineWhereTheWritingDropsToTheNextOne) {
    const std::vector<InkWord> words{
        wordAt("Hello", 10.0F, 10.0F),
        wordAt("world", 60.0F, 11.0F),
        wordAt("again", 10.0F, 60.0F),
    };

    const std::optional<TextBlock> block = textOf(words);

    ASSERT_TRUE(block.has_value());
    EXPECT_EQ(block->text, "Hello world\nagain");
}

TEST(WrittenTextTest, KeepsWordsTogetherWhenTheyOnlyLeanOnTheLine) {
    const std::vector<InkWord> words{
        wordAt("low", 10.0F, 10.0F, 40.0F, 20.0F),
        wordAt("high", 60.0F, 4.0F, 40.0F, 26.0F),
    };

    const std::optional<TextBlock> block = textOf(words);

    ASSERT_TRUE(block.has_value());
    EXPECT_EQ(block->text, "low high");
}

TEST(WrittenTextTest, SaysWhereTheWritingSatAndHowWideToLeaveIt) {
    const std::vector<InkWord> words{
        wordAt("Hello", 10.0F, 10.0F),
        wordAt("world", 60.0F, 10.0F),
        wordAt("again", 10.0F, 60.0F),
    };

    const std::optional<TextBlock> block = textOf(words);

    ASSERT_TRUE(block.has_value());
    EXPECT_FLOAT_EQ(block->area.left, 10.0F);
    EXPECT_FLOAT_EQ(block->area.top, 10.0F);
    EXPECT_FLOAT_EQ(block->area.right, 100.0F);
    EXPECT_FLOAT_EQ(block->area.bottom, 80.0F);
    EXPECT_FLOAT_EQ(block->width, 90.0F * kRoomToBreathe);
}

TEST(WrittenTextTest, TakesTheSizeOfTypeFromTheTallerWords) {
    const std::vector<InkWord> words{
        wordAt("aa", 10.0F, 10.0F, 40.0F, 16.0F),
        wordAt("bb", 60.0F, 10.0F, 40.0F, 24.0F),
    };

    const std::optional<TextBlock> block = textOf(words);

    ASSERT_TRUE(block.has_value());
    EXPECT_FLOAT_EQ(block->size, pointsOfPageUnits(24.0F));
}

TEST(WrittenTextTest, KeepsTheSizeWithinWhatTypeCanBe) {
    const std::vector<InkWord> words{wordAt("tiny", 0.0F, 0.0F, 4.0F, 1.0F)};

    const std::optional<TextBlock> block = textOf(words);

    ASSERT_TRUE(block.has_value());
    EXPECT_FLOAT_EQ(block->size, TextStyle::kSmallestSize);
    EXPECT_FLOAT_EQ(block->width, TextBox::kNarrowest);
}

}
}
