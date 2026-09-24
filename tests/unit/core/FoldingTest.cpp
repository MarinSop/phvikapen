#include "core/text/Folding.hpp"

#include <gtest/gtest.h>

namespace phvikapen::core {
namespace {

TEST(FoldingTest, LeavesPlainLettersAsTheyAreButLowerCased) {
    EXPECT_EQ(folded("Ekotoksikologija"), "ekotoksikologija");
    EXPECT_EQ(folded("PAGE 42"), "page42");
}

TEST(FoldingTest, FoldsTheLettersCroatianWritingUses) {
    EXPECT_EQ(folded("Čehoslovačka"), "cehoslovacka");
    EXPECT_EQ(folded("đak"), "dak");
    EXPECT_EQ(folded("žuč"), "zuc");
    EXPECT_EQ(folded("ŠEŠIR"), "sesir");
    EXPECT_EQ(folded("Ćiril"), "ciril");
}

TEST(FoldingTest, FoldsTheLettersOfOtherLatinWriting) {
    EXPECT_EQ(folded("Müller"), "muller");
    EXPECT_EQ(folded("résumé"), "resume");
    EXPECT_EQ(folded("piñata"), "pinata");
    EXPECT_EQ(folded("Łódź"), "lodz");
    EXPECT_EQ(folded("straße"), "strasse");
}

TEST(FoldingTest, DropsWhatNobodyWouldTypeInASearch) {
    EXPECT_EQ(folded("  word, "), "word");
    EXPECT_EQ(folded("re-read"), "reread");
    EXPECT_EQ(folded("!?..."), "");
    EXPECT_EQ(folded(""), "");
}

TEST(FoldingTest, KeepsWritingItDoesNotKnowOutOfTheWay) {
    EXPECT_EQ(folded("привет"), "");
    EXPECT_EQ(folded("a привет b"), "ab");
}

}
}
