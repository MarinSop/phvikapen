#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/Outline.hpp"
#include "core/model/PageStyle.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StrokeCodec.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>
#include <sqlite3.h>

#include <cstddef>
#include <string>
#include <vector>

namespace phvikapen::core {
namespace {

using test::makeStroke;
using test::TemporaryNotebook;

[[nodiscard]] std::vector<Uuid> pageIds(const SectionInfo& section) {
    std::vector<Uuid> ids;
    ids.reserve(section.pages.size());
    for (const PageInfo& page : section.pages) {
        ids.push_back(page.id);
    }
    return ids;
}

[[nodiscard]] std::vector<Uuid> sectionIds(const NotebookOutline& outline) {
    std::vector<Uuid> ids;
    ids.reserve(outline.sections.size());
    for (const SectionInfo& section : outline.sections) {
        ids.push_back(section.id);
    }
    return ids;
}

TEST(NotebookOutlineTest, ANewNotebookHasOneSectionWithOneBlankPage) {
    const TemporaryNotebook notebook;
    const Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;

    const Result<NotebookOutline> outline = store->readOutline();

    ASSERT_TRUE(outline.has_value()) << outline.error().message;
    EXPECT_EQ(outline->title, notebook.path().stem().string());
    ASSERT_EQ(outline->sections.size(), 1U);
    EXPECT_EQ(outline->sections.front().title, "Section 1");
    ASSERT_EQ(outline->sections.front().pages.size(), 1U);
    EXPECT_EQ(outline->sections.front().pages.front().style, PageStyle{});
    EXPECT_EQ(outline->sections.front().pages.front().id.version(), 7);
}

TEST(NotebookOutlineTest, OpeningAgainAddsNothing) {
    const TemporaryNotebook notebook;
    NotebookOutline first;
    {
        const Result<NotebookStore> store = NotebookStore::open(notebook.path());
        ASSERT_TRUE(store.has_value()) << store.error().message;
        first = store->readOutline().value();
    }

    const Result<NotebookStore> reopened = NotebookStore::open(notebook.path());
    ASSERT_TRUE(reopened.has_value()) << reopened.error().message;

    EXPECT_EQ(reopened->readOutline(), first);
}

TEST(NotebookOutlineTest, SectionsPagesTitlesAndStylesSurviveReopening) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    NotebookOutline written;
    {
        Result<NotebookStore> store = NotebookStore::open(notebook.path());
        ASSERT_TRUE(store.has_value()) << store.error().message;
        const NotebookOutline initial = store->readOutline().value();
        const Uuid firstSection = initial.sections.front().id;
        const Uuid secondSection = ids.next();
        const std::vector<Uuid> sectionOrder{firstSection, secondSection};
        const PageInfo grid{
            .id = ids.next(),
            .title = "Physics",
            .style =
                {
                    .paper = Paper::Infinite,
                    .orientation = Orientation::Landscape,
                    .background = Background::Grid,
                    .spacing = 20.0F,
                },
        };
        const std::vector<Uuid> pageOrder{grid.id};

        ASSERT_TRUE(store->setTitle("Lectures"));
        ASSERT_TRUE(store->insertSection(secondSection, "Semester 2", sectionOrder));
        ASSERT_TRUE(store->renameSection(firstSection, "Semester 1"));
        ASSERT_TRUE(store->insertPage(secondSection, grid, pageOrder));
        ASSERT_TRUE(store->renamePage(initial.sections.front().pages.front().id, "Intro"));
        written = store->readOutline().value();
    }

    const Result<NotebookStore> reopened = NotebookStore::open(notebook.path());
    ASSERT_TRUE(reopened.has_value()) << reopened.error().message;
    const Result<NotebookOutline> outline = reopened->readOutline();

    ASSERT_TRUE(outline.has_value()) << outline.error().message;
    EXPECT_EQ(*outline, written);
    EXPECT_EQ(outline->title, "Lectures");
    ASSERT_EQ(outline->sections.size(), 2U);
    EXPECT_EQ(outline->sections[0].title, "Semester 1");
    EXPECT_EQ(outline->sections[0].pages.front().title, "Intro");
    EXPECT_EQ(outline->sections[1].title, "Semester 2");
    ASSERT_EQ(outline->sections[1].pages.size(), 1U);
    EXPECT_EQ(outline->sections[1].pages.front().style.background, Background::Grid);
    EXPECT_EQ(outline->sections[1].pages.front().style.paper, Paper::Infinite);
}

TEST(NotebookOutlineTest, PagesGoWhereTheOrderSays) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const SectionInfo section = store->readOutline().value().sections.front();
    const Uuid first = section.pages.front().id;
    const PageInfo last{.id = ids.next(), .title = {}, .style = {}};
    const PageInfo middle{.id = ids.next(), .title = {}, .style = {}};

    ASSERT_TRUE(store->insertPage(section.id, last, std::vector<Uuid>{first, last.id}));
    ASSERT_TRUE(
        store->insertPage(section.id, middle, std::vector<Uuid>{first, middle.id, last.id}));

    EXPECT_EQ(pageIds(store->readOutline().value().sections.front()),
              (std::vector<Uuid>{first, middle.id, last.id}));

    ASSERT_TRUE(store->orderPages(section.id, std::vector<Uuid>{last.id, first, middle.id}));

    EXPECT_EQ(pageIds(store->readOutline().value().sections.front()),
              (std::vector<Uuid>{last.id, first, middle.id}));
}

TEST(NotebookOutlineTest, APageCanMoveToAnotherSection) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const SectionInfo source = store->readOutline().value().sections.front();
    const Uuid moved = source.pages.front().id;
    const Uuid target = ids.next();
    ASSERT_TRUE(store->insertSection(target, "Target", std::vector<Uuid>{source.id, target}));
    const PageInfo staying{.id = ids.next(), .title = {}, .style = {}};
    ASSERT_TRUE(store->insertPage(target, staying, std::vector<Uuid>{staying.id}));

    ASSERT_TRUE(store->orderPages(target, std::vector<Uuid>{moved, staying.id}));

    const NotebookOutline outline = store->readOutline().value();
    EXPECT_TRUE(outline.sections[0].pages.empty());
    EXPECT_EQ(pageIds(outline.sections[1]), (std::vector<Uuid>{moved, staying.id}));
}

TEST(NotebookOutlineTest, ATrashedPageIsHiddenUntilItIsRestoredToItsPlace) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const SectionInfo section = store->readOutline().value().sections.front();
    const Uuid first = section.pages.front().id;
    const PageInfo second{.id = ids.next(), .title = {}, .style = {}};
    const std::vector<Uuid> both{first, second.id};
    ASSERT_TRUE(store->insertPage(section.id, second, both));
    const Stroke kept = makeStroke(ids, 0.0F);
    ASSERT_TRUE(store->insertStroke(first, {.ordinal = 0, .stroke = kept}));

    ASSERT_TRUE(store->trashPage(first));
    EXPECT_EQ(pageIds(store->readOutline().value().sections.front()), std::vector<Uuid>{second.id});
    EXPECT_EQ(store->strokesOfPage(first)->size(), 1U);

    ASSERT_TRUE(store->restorePage(section.id, first, both));
    EXPECT_EQ(pageIds(store->readOutline().value().sections.front()), both);
}

TEST(NotebookOutlineTest, ATrashedSectionTakesItsPagesOutOfSight) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid first = store->readOutline().value().sections.front().id;
    const Uuid second = ids.next();
    const std::vector<Uuid> both{first, second};
    ASSERT_TRUE(store->insertSection(second, "Second", both));

    ASSERT_TRUE(store->trashSection(first));
    EXPECT_EQ(sectionIds(store->readOutline().value()), std::vector<Uuid>{second});

    ASSERT_TRUE(store->restoreSection(first, both));
    EXPECT_EQ(sectionIds(store->readOutline().value()), both);
}

TEST(NotebookOutlineTest, ChangingSomethingThatDoesNotExistIsReported) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid unknown = ids.next();

    for (const Result<void>& changed : {
             store->renamePage(unknown, "x"),
             store->setPageStyle(unknown, PageStyle{}),
             store->trashPage(unknown),
             store->renameSection(unknown, "x"),
             store->trashSection(unknown),
         }) {
        ASSERT_FALSE(changed.has_value());
        EXPECT_EQ(changed.error().code, ErrorCode::NotFound);
    }
}

TEST(NotebookOutlineTest, AFailedChangeLeavesTheOutlineAsItWas) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline before = store->readOutline().value();
    const Uuid section = before.sections.front().id;
    const PageInfo page{.id = ids.next(), .title = {}, .style = {}};

    const Result<void> inserted =
        store->insertPage(section, page, std::vector<Uuid>{page.id, ids.next()});

    ASSERT_FALSE(inserted.has_value());
    EXPECT_EQ(store->readOutline(), before);
}

TEST(NotebookOutlineTest, StrokesFromSchemaVersion2GetAPageWithTheDefaultStyle) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    const Stroke stroke = makeStroke(ids, 10.0F);
    {
        sqlite3* raw = nullptr;
        ASSERT_EQ(sqlite3_open(notebook.path().string().c_str(), &raw), SQLITE_OK);
        ASSERT_EQ(sqlite3_exec(raw, R"sql(
            CREATE TABLE strokes (
                id        BLOB PRIMARY KEY NOT NULL,
                page_id   BLOB NOT NULL,
                ordinal   INTEGER NOT NULL,
                data      BLOB NOT NULL
            );
            CREATE UNIQUE INDEX strokes_by_page ON strokes (page_id, ordinal);
            PRAGMA user_version = 2;
        )sql",
                               nullptr, nullptr, nullptr),
                  SQLITE_OK);
        sqlite3_stmt* insert = nullptr;
        ASSERT_EQ(sqlite3_prepare_v2(raw, "INSERT INTO strokes VALUES (?, ?, 0, ?);", -1, &insert,
                                     nullptr),
                  SQLITE_OK);
        const std::vector<std::byte> data = encodeStroke(stroke);
        sqlite3_bind_blob(insert, 1, stroke.id().bytes().data(), 16, SQLITE_TRANSIENT);
        sqlite3_bind_blob(insert, 2, page.bytes().data(), 16, SQLITE_TRANSIENT);
        sqlite3_bind_blob(insert, 3, data.data(), static_cast<int>(data.size()), SQLITE_TRANSIENT);
        EXPECT_EQ(sqlite3_step(insert), SQLITE_DONE);
        sqlite3_finalize(insert);
        sqlite3_close(raw);
    }

    const Result<NotebookStore> store = NotebookStore::open(notebook.path());

    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Result<NotebookOutline> outline = store->readOutline();
    ASSERT_TRUE(outline.has_value()) << outline.error().message;
    ASSERT_EQ(outline->sections.size(), 1U);
    ASSERT_EQ(outline->sections.front().pages.size(), 1U);
    EXPECT_EQ(outline->sections.front().pages.front().id, page);
    EXPECT_EQ(outline->sections.front().pages.front().style, PageStyle{});
    EXPECT_EQ(store->strokesOfPage(page)->size(), 1U);
}

}
}
