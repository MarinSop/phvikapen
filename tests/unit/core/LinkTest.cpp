#include "core/model/Link.hpp"

#include "core/id/Uuid7Generator.hpp"
#include "core/model/Layer.hpp"
#include "core/model/Page.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] Link webLink(Uuid7Generator& ids, float left, float top) {
    return Link{
        .id = ids.next(),
        .at = Point{.x = left, .y = top},
        .width = 40.0F,
        .height = 20.0F,
        .kind = LinkKind::Web,
        .page = Uuid{},
        .where = "https://example.org",
        .label = "Somewhere",
    };
}

TEST(LinkTest, OnlyTheWaysOutThatAreSafeAreFollowed) {
    EXPECT_TRUE(isSafeToFollow("https://example.org"));
    EXPECT_TRUE(isSafeToFollow("HTTP://EXAMPLE.ORG"));
    EXPECT_TRUE(isSafeToFollow("mailto:someone@example.org"));
    EXPECT_FALSE(isSafeToFollow("file:///etc/passwd"));
    EXPECT_FALSE(isSafeToFollow("javascript:alert(1)"));
    EXPECT_FALSE(isSafeToFollow("C:\\Windows\\System32\\cmd.exe"));
    EXPECT_FALSE(isSafeToFollow(""));
}

TEST(LinkTest, ALinkPointingNowhereIsNeverFollowed) {
    Uuid7Generator ids;
    Link link = webLink(ids, 0.0F, 0.0F);
    link.where = "file:///etc/passwd";

    EXPECT_FALSE(goesSomewhere(link));

    link.kind = LinkKind::Page;
    EXPECT_FALSE(goesSomewhere(link)) << "a page link with no page named goes nowhere";

    link.page = ids.next();
    EXPECT_TRUE(goesSomewhere(link));
}

TEST(LinkTest, ALinkIsNeverSmallerThanItCanBeTapped) {
    Uuid7Generator ids;
    Link link = webLink(ids, 0.0F, 0.0F);
    link.width = 0.0F;
    link.height = -4.0F;

    const Link put = normalized(link);

    EXPECT_FLOAT_EQ(put.width, Link::kSmallest);
    EXPECT_FLOAT_EQ(put.height, Link::kSmallest);
}

TEST(LinkTest, WhatIsWrittenDownIsCutToWhatWillBeKept) {
    Uuid7Generator ids;
    Link link = webLink(ids, 0.0F, 0.0F);
    link.label = std::string(Link::kLongestLabel + 50, 'a');
    link.where = "https://" + std::string(Link::kLongestWhere, 'b');

    const Link put = normalized(link);

    EXPECT_EQ(put.label.size(), Link::kLongestLabel);
    EXPECT_EQ(put.where.size(), Link::kLongestWhere);
}

TEST(LinkTest, ALinkIsPutOnThePageAndFoundUnderATap) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const Link link = webLink(ids, 10.0F, 10.0F);

    ASSERT_TRUE(page.insertLink(PlacedLink{.ordinal = 0, .link = link, .layer = {}}).has_value());

    ASSERT_EQ(page.links().size(), 1U);
    ASSERT_NE(page.linkUnder(Point{.x = 20.0F, .y = 15.0F}), nullptr);
    EXPECT_EQ(page.linkUnder(Point{.x = 20.0F, .y = 15.0F})->id, link.id);
    EXPECT_EQ(page.linkUnder(Point{.x = 200.0F, .y = 200.0F}), nullptr);
}

TEST(LinkTest, TheLastLinkPutDownIsTheOneATapFinds) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const Link under = webLink(ids, 10.0F, 10.0F);
    const Link over = webLink(ids, 10.0F, 10.0F);

    ASSERT_TRUE(page.insertLink(PlacedLink{.ordinal = 0, .link = under, .layer = {}}).has_value());
    ASSERT_TRUE(page.insertLink(PlacedLink{.ordinal = 1, .link = over, .layer = {}}).has_value());

    ASSERT_NE(page.linkUnder(Point{.x = 20.0F, .y = 15.0F}), nullptr);
    EXPECT_EQ(page.linkUnder(Point{.x = 20.0F, .y = 15.0F})->id, over.id);
}

TEST(LinkTest, ALinkOnALayerThatIsShutIsNotFoundUnderATap) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const Uuid layerId = page.layers().front().id;
    const Link link = webLink(ids, 10.0F, 10.0F);
    ASSERT_TRUE(
        page.insertLink(PlacedLink{.ordinal = 0, .link = link, .layer = layerId}).has_value());
    ASSERT_NE(page.linkUnder(Point{.x = 20.0F, .y = 15.0F}), nullptr);

    std::vector<Layer> layers{page.layers().begin(), page.layers().end()};
    layers.front().locked = true;
    page.setLayers(layers);

    EXPECT_EQ(page.linkUnder(Point{.x = 20.0F, .y = 15.0F}), nullptr);

    layers.front().locked = false;
    layers.front().shown = false;
    page.setLayers(layers);

    EXPECT_EQ(page.linkUnder(Point{.x = 20.0F, .y = 15.0F}), nullptr);
}

TEST(LinkTest, ALinkIsChangedAndTakenAway) {
    Uuid7Generator ids;
    Page page{ids.next()};
    Link link = webLink(ids, 10.0F, 10.0F);
    ASSERT_TRUE(page.insertLink(PlacedLink{.ordinal = 0, .link = link, .layer = {}}).has_value());

    link.label = "Elsewhere";
    ASSERT_TRUE(page.replaceLink(link).has_value());
    ASSERT_NE(page.linkAt(link.id), nullptr);
    EXPECT_EQ(page.linkAt(link.id)->label, "Elsewhere");

    const Result<PlacedLink> gone = page.removeLink(link.id);

    ASSERT_TRUE(gone.has_value());
    EXPECT_TRUE(page.links().empty());
    EXPECT_FALSE(page.removeLink(link.id).has_value());
}

TEST(LinkTest, TheSameLinkIsNeverPutOnTwice) {
    Uuid7Generator ids;
    Page page{ids.next()};
    const Link link = webLink(ids, 10.0F, 10.0F);
    ASSERT_TRUE(page.insertLink(PlacedLink{.ordinal = 0, .link = link, .layer = {}}).has_value());

    EXPECT_FALSE(page.insertLink(PlacedLink{.ordinal = 1, .link = link, .layer = {}}).has_value());
}

TEST(LinkTest, TheTopmostOfAListIsTheOneFound) {
    Uuid7Generator ids;
    const Link lower = webLink(ids, 0.0F, 0.0F);
    const Link upper = webLink(ids, 0.0F, 0.0F);
    const std::array<PlacedLink, 2> links{
        PlacedLink{.ordinal = 5, .link = upper, .layer = {}},
        PlacedLink{.ordinal = 1, .link = lower, .layer = {}},
    };

    ASSERT_NE(linkUnder(links, Point{.x = 5.0F, .y = 5.0F}), nullptr);
    EXPECT_EQ(linkUnder(links, Point{.x = 5.0F, .y = 5.0F})->link.id, upper.id);
    EXPECT_EQ(linkUnder(links, Point{.x = 500.0F, .y = 5.0F}), nullptr);
}

}
}
