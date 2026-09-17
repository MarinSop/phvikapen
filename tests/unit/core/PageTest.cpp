#include "core/model/Page.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>

#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

using test::makeStroke;

[[nodiscard]] std::vector<Uuid> idsOn(const Page& page) {
    std::vector<Uuid> ids;
    for (const PlacedStroke& placed : page.strokes()) {
        ids.push_back(placed.stroke.id());
    }
    return ids;
}

TEST(PageTest, StartsEmptyWithTheFirstPlaceFree) {
    Uuid7Generator ids;
    const Page page{ids.next()};

    EXPECT_TRUE(page.strokes().empty());
    EXPECT_EQ(page.nextOrdinal(), 0);
}

TEST(PageTest, KeepsStrokesInDrawingOrderWhateverOrderTheyArriveIn) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const PlacedStroke first{.ordinal = 0, .stroke = makeStroke(ids, 0.0F)};
    const PlacedStroke second{.ordinal = 4, .stroke = makeStroke(ids, 10.0F)};
    const PlacedStroke third{.ordinal = 9, .stroke = makeStroke(ids, 20.0F)};

    ASSERT_TRUE(page.insert(third));
    ASSERT_TRUE(page.insert(first));
    ASSERT_TRUE(page.insert(second));

    EXPECT_EQ(idsOn(page),
              (std::vector<Uuid>{first.stroke.id(), second.stroke.id(), third.stroke.id()}));
    EXPECT_EQ(page.nextOrdinal(), 10);
}

TEST(PageTest, RefusesAPlaceThatIsTaken) {
    Uuid7Generator ids;
    Page page{ids.next()};
    ASSERT_TRUE(page.insert({.ordinal = 3, .stroke = makeStroke(ids, 0.0F)}));

    const Result<void> inserted = page.insert({.ordinal = 3, .stroke = makeStroke(ids, 10.0F)});

    ASSERT_FALSE(inserted.has_value());
    EXPECT_EQ(inserted.error().code, ErrorCode::InvalidArgument);
    EXPECT_EQ(page.strokes().size(), 1U);
}

TEST(PageTest, RefusesTheSameStrokeTwice) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const PlacedStroke placed{.ordinal = 0, .stroke = makeStroke(ids, 0.0F)};
    ASSERT_TRUE(page.insert(placed));

    const Result<void> inserted = page.insert({.ordinal = 1, .stroke = placed.stroke});

    ASSERT_FALSE(inserted.has_value());
    EXPECT_EQ(inserted.error().code, ErrorCode::InvalidArgument);
}

TEST(PageTest, RemovesAStrokeAndRemembersItsPlace) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const PlacedStroke first{.ordinal = 0, .stroke = makeStroke(ids, 0.0F)};
    const PlacedStroke second{.ordinal = 1, .stroke = makeStroke(ids, 10.0F)};
    const PlacedStroke third{.ordinal = 2, .stroke = makeStroke(ids, 20.0F)};
    ASSERT_TRUE(page.insert(first));
    ASSERT_TRUE(page.insert(second));
    ASSERT_TRUE(page.insert(third));

    Result<PlacedStroke> removed = page.remove(second.stroke.id());
    ASSERT_TRUE(removed.has_value()) << removed.error().message;
    EXPECT_EQ(removed->ordinal, 1);
    EXPECT_EQ(idsOn(page), (std::vector<Uuid>{first.stroke.id(), third.stroke.id()}));

    ASSERT_TRUE(page.insert(std::move(*removed)));
    EXPECT_EQ(idsOn(page),
              (std::vector<Uuid>{first.stroke.id(), second.stroke.id(), third.stroke.id()}));
}

TEST(PageTest, ReportsAStrokeItDoesNotHold) {
    Uuid7Generator ids;
    Page page{ids.next()};

    const Result<PlacedStroke> removed = page.remove(ids.next());

    ASSERT_FALSE(removed.has_value());
    EXPECT_EQ(removed.error().code, ErrorCode::NotFound);
}

TEST(PageTest, HandsOverEverythingAtOnce) {
    Uuid7Generator ids;
    Page page{ids.next()};
    ASSERT_TRUE(page.insert({.ordinal = 0, .stroke = makeStroke(ids, 0.0F)}));
    ASSERT_TRUE(page.insert({.ordinal = 1, .stroke = makeStroke(ids, 10.0F)}));

    const std::vector<PlacedStroke> taken = page.takeAll();

    EXPECT_EQ(taken.size(), 2U);
    EXPECT_TRUE(page.strokes().empty());
    EXPECT_EQ(page.nextOrdinal(), 0);
}

}
}
