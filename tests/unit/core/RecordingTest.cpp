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

}
}
