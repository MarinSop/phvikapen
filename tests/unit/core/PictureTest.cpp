#include "core/model/Picture.hpp"

#include "core/Error.hpp"
#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/ContentId.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/Page.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

namespace phvikapen::core {
namespace {

[[nodiscard]] Picture pictureAt(Uuid7Generator& ids, float x, float y) {
    return Picture{
        .id = ids.next(),
        .source = ContentId{ContentId::Bytes{7, 7, 7}},
        .at = Point{.x = x, .y = y},
        .width = 100.0F,
        .height = 60.0F,
        .turn = 0.0F,
    };
}

TEST(PictureTest, CoversTheBoxItWasGiven) {
    Uuid7Generator ids;
    const Rect area = areaOf(pictureAt(ids, 10.0F, 20.0F));

    EXPECT_FLOAT_EQ(area.left, 10.0F);
    EXPECT_FLOAT_EQ(area.top, 20.0F);
    EXPECT_FLOAT_EQ(area.right, 110.0F);
    EXPECT_FLOAT_EQ(area.bottom, 80.0F);
}

TEST(PictureTest, IsNeverSmallerThanSomethingThatCanBeTakenHoldOf) {
    Uuid7Generator ids;
    Picture tiny = pictureAt(ids, 0.0F, 0.0F);
    tiny.width = 0.0F;
    tiny.height = -5.0F;

    const Picture put = normalized(tiny);

    EXPECT_FLOAT_EQ(put.width, Picture::kSmallest);
    EXPECT_FLOAT_EQ(put.height, Picture::kSmallest);
}

TEST(PictureTest, NumbersThatAreNoNumberArePutRight) {
    Uuid7Generator ids;
    Picture broken = pictureAt(ids, 0.0F, 0.0F);
    broken.at = Point{.x = std::numeric_limits<float>::quiet_NaN(), .y = 0.0F};
    broken.width = std::numeric_limits<float>::infinity();
    broken.turn = std::numeric_limits<float>::quiet_NaN();

    const Picture put = normalized(broken);

    EXPECT_TRUE(std::isfinite(put.at.x));
    EXPECT_FLOAT_EQ(put.width, Picture::kDefaultWidth);
    EXPECT_FLOAT_EQ(put.turn, 0.0F);
}

TEST(PictureTest, TurningRoundAndRoundIsKeptWithinOneTurn) {
    Uuid7Generator ids;
    Picture wound = pictureAt(ids, 0.0F, 0.0F);
    wound.turn = 725.0F;

    EXPECT_NEAR(normalized(wound).turn, 5.0F, 0.001F);
}

TEST(PagePicturesTest, KeepsPicturesInTheOrderTheyWerePutDown) {
    Uuid7Generator ids;
    Page page{ids.next()};

    ASSERT_TRUE(page.insertPicture({.ordinal = 1, .picture = pictureAt(ids, 30.0F, 0.0F)}));
    ASSERT_TRUE(page.insertPicture({.ordinal = 0, .picture = pictureAt(ids, 10.0F, 0.0F)}));

    ASSERT_EQ(page.pictures().size(), 2U);
    EXPECT_FLOAT_EQ(page.pictures().front().picture.at.x, 10.0F);
    EXPECT_FLOAT_EQ(page.pictures().back().picture.at.x, 30.0F);
    EXPECT_EQ(page.nextPictureOrdinal(), 2);
}

TEST(PagePicturesTest, RefusesTwoPicturesInTheSamePlaceInTheOrder) {
    Uuid7Generator ids;
    Page page{ids.next()};
    ASSERT_TRUE(page.insertPicture({.ordinal = 0, .picture = pictureAt(ids, 10.0F, 0.0F)}));

    const Result<void> again =
        page.insertPicture({.ordinal = 0, .picture = pictureAt(ids, 20.0F, 0.0F)});

    ASSERT_FALSE(again.has_value());
    EXPECT_EQ(again.error().code, ErrorCode::InvalidArgument);
}

TEST(PagePicturesTest, TakesAPictureOffAndPutsItBack) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const Picture picture = pictureAt(ids, 10.0F, 10.0F);
    ASSERT_TRUE(page.insertPicture({.ordinal = 0, .picture = picture}));

    const Result<PlacedPicture> taken = page.removePicture(picture.id);

    ASSERT_TRUE(taken.has_value());
    EXPECT_EQ(taken->picture, picture);
    EXPECT_TRUE(page.pictures().empty());
    EXPECT_EQ(page.pictureAt(picture.id), nullptr);
}

TEST(PagePicturesTest, WritesOverAPictureThatIsAlreadyThere) {
    Uuid7Generator ids;
    Page page{ids.next()};
    Picture picture = pictureAt(ids, 10.0F, 10.0F);
    ASSERT_TRUE(page.insertPicture({.ordinal = 0, .picture = picture}));

    picture.turn = 45.0F;
    picture.width = 200.0F;
    ASSERT_TRUE(page.replacePicture(picture));

    ASSERT_NE(page.pictureAt(picture.id), nullptr);
    EXPECT_EQ(*page.pictureAt(picture.id), picture);
}

TEST(PagePicturesTest, ATapLandsOnTheLastPicturePutDown) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const Picture under = pictureAt(ids, 0.0F, 0.0F);
    const Picture over = pictureAt(ids, 10.0F, 10.0F);
    ASSERT_TRUE(page.insertPicture({.ordinal = 0, .picture = under}));
    ASSERT_TRUE(page.insertPicture({.ordinal = 1, .picture = over}));

    const Picture* const found = page.pictureUnder(Point{.x = 20.0F, .y = 20.0F});

    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id, over.id);
    EXPECT_EQ(page.pictureUnder(Point{.x = 500.0F, .y = 500.0F}), nullptr);
}

TEST(PagePicturesTest, ClearingTakesThePicturesWithIt) {
    Uuid7Generator ids;
    Page page{ids.next()};
    ASSERT_TRUE(page.insertPicture({.ordinal = 0, .picture = pictureAt(ids, 0.0F, 0.0F)}));

    EXPECT_EQ(page.takeAllPictures().size(), 1U);
    EXPECT_TRUE(page.pictures().empty());
}

}
}
