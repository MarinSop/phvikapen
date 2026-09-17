#include "core/model/Outline.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/PageStyle.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] PageInfo blankPage(const Uuid& id) {
    const PageStyle style;
    return PageInfo{.id = id, .title = {}, .style = style, .media = std::nullopt};
}

struct Fixture {
    Uuid7Generator ids;
    Uuid firstSection = ids.next();
    Uuid secondSection = ids.next();
    Uuid a = ids.next();
    Uuid b = ids.next();
    Uuid c = ids.next();
    Outline outline{NotebookOutline{
        .title = "Notebook",
        .sections =
            {
                SectionInfo{
                    .id = firstSection,
                    .title = "First",
                    .pages =
                        {
                            blankPage(a),
                            blankPage(b),
                        },
                },
                SectionInfo{
                    .id = secondSection,
                    .title = "Second",
                    .pages = {blankPage(c)},
                },
            },
    }};
};

TEST(OutlineTest, FindsWherePagesAndSectionsAre) {
    const Fixture fixture;
    const Outline& outline = fixture.outline;

    EXPECT_EQ(outline.sectionIndex(fixture.secondSection), 1U);
    EXPECT_EQ(outline.placeOf(fixture.b),
              (PagePlace{.sectionId = fixture.firstSection, .index = 1}));
    EXPECT_EQ(outline.placeOf(fixture.c),
              (PagePlace{.sectionId = fixture.secondSection, .index = 0}));
    ASSERT_NE(outline.page(fixture.c), nullptr);
    EXPECT_EQ(outline.page(fixture.c)->id, fixture.c);
    EXPECT_EQ(outline.pageOrder(fixture.firstSection), (std::vector<Uuid>{fixture.a, fixture.b}));
    EXPECT_EQ(outline.sectionOrder(),
              (std::vector<Uuid>{fixture.firstSection, fixture.secondSection}));
    EXPECT_FALSE(outline.placeOf(Uuid{}).has_value());
    EXPECT_TRUE(outline.pageOrder(Uuid{}).empty());
}

TEST(OutlineTest, InsertsAndRemovesPagesInPlace) {
    Fixture fixture;
    const Uuid inserted = fixture.ids.next();

    ASSERT_TRUE(fixture.outline.insertPage({.sectionId = fixture.firstSection, .index = 1},
                                           blankPage(inserted)));
    EXPECT_EQ(fixture.outline.pageOrder(fixture.firstSection),
              (std::vector<Uuid>{fixture.a, inserted, fixture.b}));

    const Result<RemovedPage> removed = fixture.outline.removePage(inserted);
    ASSERT_TRUE(removed.has_value()) << removed.error().message;
    EXPECT_EQ(removed->place, (PagePlace{.sectionId = fixture.firstSection, .index = 1}));
    EXPECT_EQ(fixture.outline.pageOrder(fixture.firstSection),
              (std::vector<Uuid>{fixture.a, fixture.b}));
}

TEST(OutlineTest, RefusesPagesThatCannotBeInserted) {
    Fixture fixture;

    const Result<void> duplicate = fixture.outline.insertPage(
        {.sectionId = fixture.secondSection, .index = 0}, blankPage(fixture.a));
    const Result<void> outside = fixture.outline.insertPage(
        {.sectionId = fixture.secondSection, .index = 5}, blankPage(fixture.ids.next()));
    const Result<void> nowhere = fixture.outline.insertPage({.sectionId = Uuid{}, .index = 0},
                                                            blankPage(fixture.ids.next()));

    EXPECT_EQ(duplicate.error().code, ErrorCode::InvalidArgument);
    EXPECT_EQ(outside.error().code, ErrorCode::InvalidArgument);
    EXPECT_EQ(nowhere.error().code, ErrorCode::NotFound);
}

TEST(OutlineTest, MovesAPageWithinItsSectionAndAcrossSections) {
    Fixture fixture;

    const Result<PagePlace> within =
        fixture.outline.movePage(fixture.a, {.sectionId = fixture.firstSection, .index = 1});
    ASSERT_TRUE(within.has_value()) << within.error().message;
    EXPECT_EQ(*within, (PagePlace{.sectionId = fixture.firstSection, .index = 0}));
    EXPECT_EQ(fixture.outline.pageOrder(fixture.firstSection),
              (std::vector<Uuid>{fixture.b, fixture.a}));

    const Result<PagePlace> across =
        fixture.outline.movePage(fixture.b, {.sectionId = fixture.secondSection, .index = 1});
    ASSERT_TRUE(across.has_value()) << across.error().message;
    EXPECT_EQ(*across, (PagePlace{.sectionId = fixture.firstSection, .index = 0}));
    EXPECT_EQ(fixture.outline.pageOrder(fixture.firstSection), std::vector<Uuid>{fixture.a});
    EXPECT_EQ(fixture.outline.pageOrder(fixture.secondSection),
              (std::vector<Uuid>{fixture.c, fixture.b}));

    const Result<PagePlace> tooFar =
        fixture.outline.movePage(fixture.a, {.sectionId = fixture.firstSection, .index = 1});
    ASSERT_FALSE(tooFar.has_value());
    EXPECT_EQ(fixture.outline.pageOrder(fixture.firstSection), std::vector<Uuid>{fixture.a});
}

TEST(OutlineTest, ChangesTitlesAndStylesAndHandsBackThePreviousOnes) {
    Fixture fixture;
    const PageStyle dotted{.background = Background::Dotted};

    EXPECT_EQ(fixture.outline.renameSection(fixture.firstSection, "Renamed"), "First");
    EXPECT_EQ(fixture.outline.renamePage(fixture.a, "Title"), "");
    EXPECT_EQ(fixture.outline.setPageStyle(fixture.a, dotted), PageStyle{});

    EXPECT_EQ(fixture.outline.sections()[0].title, "Renamed");
    EXPECT_EQ(fixture.outline.page(fixture.a)->title, "Title");
    EXPECT_EQ(fixture.outline.page(fixture.a)->style, dotted);
}

TEST(OutlineTest, KeepsAStyleWithinBounds) {
    Fixture fixture;

    ASSERT_TRUE(fixture.outline.setPageStyle(fixture.a, PageStyle{.spacing = 0.0F}));

    EXPECT_EQ(fixture.outline.page(fixture.a)->style.spacing, PageStyle::kMinimumSpacing);
}

TEST(OutlineTest, MovesAndRemovesSectionsTogetherWithTheirPages) {
    Fixture fixture;

    EXPECT_EQ(fixture.outline.moveSection(fixture.secondSection, 0), 1U);
    EXPECT_EQ(fixture.outline.sectionOrder(),
              (std::vector<Uuid>{fixture.secondSection, fixture.firstSection}));

    const Result<RemovedSection> removed = fixture.outline.removeSection(fixture.firstSection);
    ASSERT_TRUE(removed.has_value()) << removed.error().message;
    EXPECT_EQ(removed->index, 1U);
    EXPECT_EQ(removed->section.pages.size(), 2U);
    EXPECT_FALSE(fixture.outline.placeOf(fixture.a).has_value());

    ASSERT_TRUE(fixture.outline.insertSection(removed->index, removed->section));
    EXPECT_EQ(fixture.outline.placeOf(fixture.a),
              (PagePlace{.sectionId = fixture.firstSection, .index = 0}));
}

TEST(OutlineTest, RefusesASectionWhosePagesAreAlreadyElsewhere) {
    Fixture fixture;

    const Result<void> inserted =
        fixture.outline.insertSection(0, SectionInfo{
                                             .id = fixture.ids.next(),
                                             .title = "Copy",
                                             .pages = {blankPage(fixture.c)},
                                         });

    ASSERT_FALSE(inserted.has_value());
    EXPECT_EQ(fixture.outline.sections().size(), 2U);
}

}
}
