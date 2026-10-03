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

TEST(FoldingTest, AWordTheAskingIsFoundInsideReadsAsIt) {
    EXPECT_TRUE(readsAs("reykjavik", "reykjavik"));
    EXPECT_TRUE(readsAs("reykjavikurflugvollur", "reykjavik"));
    EXPECT_TRUE(readsAs("reykjavik", "javi"));
}

// A reader of handwriting mistakes about a letter, so an asking of some length forgives one put in,
// taken out or written as another.
TEST(FoldingTest, AnAskingOfSomeLengthForgivesOneLetter) {
    EXPECT_TRUE(readsAs("reykjauik", "reykjavik"));
    EXPECT_TRUE(readsAs("reykjavk", "reykjavik"));
    EXPECT_TRUE(readsAs("reykjaviik", "reykjavik"));
    EXPECT_TRUE(readsAs("in reykjauik today", "reykjavik"));
}

TEST(FoldingTest, TwoLettersWrongIsADifferentWord) {
    EXPECT_FALSE(readsAs("reykjauuk", "reykjavik"));
    EXPECT_FALSE(readsAs("gothenburg", "reykjavik"));
}

// Below a few letters one wrong letter is a different word altogether, so nothing is forgiven.
TEST(FoldingTest, AShortAskingForgivesNothing) {
    EXPECT_FALSE(readsAs("car", "cat"));
    EXPECT_FALSE(readsAs("hand", "band"));
    EXPECT_TRUE(readsAs("the cat sat", "cat"));
}

TEST(FoldingTest, AnAskingIsLookedUpByItsFirstLetters) {
    EXPECT_EQ(stemOf("reykjavik"), "reyk");
    EXPECT_EQ(stemOf("cat"), "cat");
    EXPECT_EQ(stemOf(""), "");
}
}
}
