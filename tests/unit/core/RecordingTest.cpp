#include "core/model/Recording.hpp"

#include "core/id/Uuid7Generator.hpp"
#include "core/model/Page.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] Recording madeOne(Uuid7Generator& ids, std::int64_t madeAt, std::int64_t length) {
    Recording made;
    made.id = ids.next();
    made.name = "A recording";
    made.length = length;
    made.madeAt = madeAt;
    return made;
}

TEST(RecordingTest, ARecordingIsPutOnThePageAndFoundAgain) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const Recording made = madeOne(ids, 1000, 5000);

    ASSERT_TRUE(page.addRecording(made).has_value());

    ASSERT_EQ(page.recordings().size(), 1U);
    ASSERT_NE(page.recording(made.id), nullptr);
    EXPECT_EQ(page.recording(made.id)->name, "A recording");
}

TEST(RecordingTest, TheSameRecordingIsNeverPutOnTwice) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const Recording made = madeOne(ids, 1000, 5000);
    ASSERT_TRUE(page.addRecording(made).has_value());

    EXPECT_FALSE(page.addRecording(made).has_value());
}

TEST(RecordingTest, RecordingsStandOldestFirst) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const Recording later = madeOne(ids, 2000, 100);
    const Recording earlier = madeOne(ids, 1000, 100);

    ASSERT_TRUE(page.addRecording(later).has_value());
    ASSERT_TRUE(page.addRecording(earlier).has_value());

    EXPECT_EQ(page.recordings().front().id, earlier.id);
}

TEST(RecordingTest, ARecordingIsRenamedWithoutMovingIt) {
    Uuid7Generator ids;
    Page page{ids.next()};
    Recording made = madeOne(ids, 1000, 5000);
    ASSERT_TRUE(page.addRecording(made).has_value());

    made.name = "The meeting";
    ASSERT_TRUE(page.changeRecording(made.id, made).has_value());

    EXPECT_EQ(page.recording(made.id)->name, "The meeting");
}

TEST(RecordingTest, ANameTooLongIsCutDown) {
    Uuid7Generator ids;
    Page page{ids.next()};
    Recording made = madeOne(ids, 1000, 100);
    made.name = std::string(Recording::kLongestName + 40, 'a');

    ASSERT_TRUE(page.addRecording(made).has_value());

    EXPECT_EQ(page.recording(made.id)->name.size(), Recording::kLongestName);
}

TEST(RecordingTest, TakingARecordingAwayTakesWhatWasTiedToItAsWell) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const Recording made = madeOne(ids, 1000, 5000);
    const Uuid thing = ids.next();
    ASSERT_TRUE(page.addRecording(made).has_value());
    ASSERT_TRUE(page.addMark(Mark{.recording = made.id, .thing = thing, .at = 300}).has_value());
    ASSERT_EQ(page.marks().size(), 1U);

    const Result<Recording> gone = page.removeRecording(made.id);

    ASSERT_TRUE(gone.has_value());
    EXPECT_TRUE(page.recordings().empty());
    EXPECT_TRUE(page.marks().empty());
}

TEST(RecordingTest, AThingIsNeverTiedToTheSameRecordingTwice) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const Recording made = madeOne(ids, 1000, 5000);
    const Uuid thing = ids.next();
    ASSERT_TRUE(page.addRecording(made).has_value());

    ASSERT_TRUE(page.addMark(Mark{.recording = made.id, .thing = thing, .at = 300}).has_value());
    ASSERT_TRUE(page.addMark(Mark{.recording = made.id, .thing = thing, .at = 900}).has_value());

    ASSERT_EQ(page.marks().size(), 1U);
    EXPECT_EQ(page.marks().front().at, 900);
}

TEST(RecordingTest, AMarkOnARecordingThatIsNotThereIsRefused) {
    Uuid7Generator ids;
    Page page{ids.next()};

    EXPECT_FALSE(
        page.addMark(Mark{.recording = ids.next(), .thing = ids.next(), .at = 0}).has_value());
}

TEST(RecordingTest, WhatWasBeingWrittenAboutAtAMomentIsTheLastThingMarkedBeforeIt) {
    Uuid7Generator ids;
    const Uuid recording = ids.next();
    const Uuid first = ids.next();
    const Uuid second = ids.next();
    const std::array<Mark, 2> marks{
        Mark{.recording = recording, .thing = first, .at = 1000},
        Mark{.recording = recording, .thing = second, .at = 5000},
    };

    EXPECT_EQ(markAt(marks, recording, 0), nullptr);
    ASSERT_NE(markAt(marks, recording, 2000), nullptr);
    EXPECT_EQ(markAt(marks, recording, 2000)->thing, first);
    ASSERT_NE(markAt(marks, recording, 9000), nullptr);
    EXPECT_EQ(markAt(marks, recording, 9000)->thing, second);
}

TEST(RecordingTest, WhereAThingWasWrittenIsFoundFromTheThing) {
    Uuid7Generator ids;
    const Uuid recording = ids.next();
    const Uuid thing = ids.next();
    const std::array<Mark, 1> marks{Mark{.recording = recording, .thing = thing, .at = 4200}};

    ASSERT_NE(markOfThing(marks, thing), nullptr);
    EXPECT_EQ(markOfThing(marks, thing)->at, 4200);
    EXPECT_EQ(markOfThing(marks, ids.next()), nullptr);
}

TEST(SayingTest, TheSayingBeingHeardAtAMomentIsFound) {
    const std::array<Saying, 2> sayings{
        Saying{.from = 0, .to = 2000, .text = "We need to finish by Friday."},
        Saying{.from = 2000, .to = 5000, .text = "The budget has been approved."},
    };

    EXPECT_EQ(sayingAt(sayings, 500), 0U);
    EXPECT_EQ(sayingAt(sayings, 2000), 1U);
    EXPECT_EQ(sayingAt(sayings, 4999), 1U);
    EXPECT_EQ(sayingAt(sayings, 5000), kNoSaying);
}

TEST(RecordingTest, ARecordingWithNoNameOfItsOwnIsNamedAfterWhenItWasMade) {
    const std::string named = plainRecordingName(0);

    EXPECT_FALSE(named.empty());
    EXPECT_NE(named.find("1970"), std::string::npos);
}

TEST(SayingTest, WordsSpokenTogetherAreGatheredIntoOneLine) {
    const std::array<Saying, 3> words{
        Saying{.from = 0, .to = 300, .text = "the"},
        Saying{.from = 320, .to = 700, .text = "budget"},
        Saying{.from = 720, .to = 1100, .text = "meeting"},
    };

    const std::vector<Saying> lines = linesOf(words);

    ASSERT_EQ(lines.size(), 1U);
    EXPECT_EQ(lines.front().text, "the budget meeting");
    EXPECT_EQ(lines.front().from, 0);
    EXPECT_EQ(lines.front().to, 1100);
}

TEST(SayingTest, ASilenceBreaksALine) {
    const std::array<Saying, 3> words{
        Saying{.from = 0, .to = 300, .text = "one"},
        Saying{.from = 320, .to = 600, .text = "two"},
        Saying{.from = 4000, .to = 4300, .text = "three"},
    };

    const std::vector<Saying> lines = linesOf(words);

    ASSERT_EQ(lines.size(), 2U);
    EXPECT_EQ(lines[0].text, "one two");
    EXPECT_EQ(lines[1].text, "three");
    EXPECT_EQ(lines[1].from, 4000);
}

TEST(SayingTest, AFullStopBreaksALine) {
    const std::array<Saying, 3> words{
        Saying{.from = 0, .to = 300, .text = "done."},
        Saying{.from = 310, .to = 600, .text = "next"},
        Saying{.from = 610, .to = 900, .text = "one"},
    };

    const std::vector<Saying> lines = linesOf(words);

    ASSERT_EQ(lines.size(), 2U);
    EXPECT_EQ(lines[0].text, "done.");
    EXPECT_EQ(lines[1].text, "next one");
}

TEST(SayingTest, ALineThatHasGrownTooLongIsBroken) {
    std::vector<Saying> words;
    for (std::int64_t step = 0; step < 40; ++step) {
        const std::int64_t at = step * 100;
        words.push_back(Saying{.from = at, .to = at + 90, .text = "word"});
    }

    const std::vector<Saying> lines = linesOf(words);

    ASSERT_GT(lines.size(), 1U);
    for (const Saying& line : lines) {
        EXPECT_LE(line.text.size(), kLongestLine + std::string{"word"}.size() + 1U);
    }
}

TEST(SayingTest, WordsWithNothingSaidInThemAreLeftOut) {
    const std::array<Saying, 3> words{
        Saying{.from = 0, .to = 100, .text = ""},
        Saying{.from = 110, .to = 300, .text = "here"},
        Saying{.from = 310, .to = 500, .text = ""},
    };

    const std::vector<Saying> lines = linesOf(words);

    ASSERT_EQ(lines.size(), 1U);
    EXPECT_EQ(lines.front().text, "here");
}

TEST(SayingTest, NothingHeardMakesNoLines) {
    EXPECT_TRUE(linesOf({}).empty());
}

TEST(SayingTest, EveryLineCanBeFoundAgainByTheMomentItWasSaid) {
    const std::array<Saying, 4> words{
        Saying{.from = 0, .to = 300, .text = "first"},
        Saying{.from = 320, .to = 600, .text = "line."},
        Saying{.from = 5000, .to = 5300, .text = "second"},
        Saying{.from = 5320, .to = 5600, .text = "line"},
    };

    const std::vector<Saying> lines = linesOf(words);

    ASSERT_EQ(lines.size(), 2U);
    EXPECT_EQ(sayingAt(lines, 100), 0U);
    EXPECT_EQ(sayingAt(lines, 5400), 1U);
    EXPECT_EQ(sayingAt(lines, 3000), kNoSaying);
}

}
}
