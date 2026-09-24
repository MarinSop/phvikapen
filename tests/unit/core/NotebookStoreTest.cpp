#include "core/storage/NotebookStore.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/Stroke.hpp"
#include "core/model/Color.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/storage/StrokeCodec.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>
#include <sqlite3.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace phvikapen::core {
namespace {

using test::makeStroke;
using test::TemporaryNotebook;

TEST(NotebookStoreTest, CreatesAFileWithTheCurrentSchema) {
    const TemporaryNotebook notebook;

    const Result<NotebookStore> store = NotebookStore::open(notebook.path());

    ASSERT_TRUE(store.has_value()) << store.error().message;
    EXPECT_TRUE(std::filesystem::exists(notebook.path()));
    const Result<int> version = store->schemaVersion();
    ASSERT_TRUE(version.has_value()) << version.error().message;
    EXPECT_EQ(*version, kNotebookSchemaVersion);
}

TEST(NotebookStoreTest, StrokesSurviveClosingAndReopening) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    const Stroke first = makeStroke(ids, 10.0F);
    const Stroke second = makeStroke(ids, 100.0F);

    {
        Result<NotebookStore> store = NotebookStore::open(notebook.path());
        ASSERT_TRUE(store.has_value()) << store.error().message;
        ASSERT_TRUE(store->insertStroke(page, {.ordinal = 0, .stroke = first}).has_value());
        ASSERT_TRUE(store->insertStroke(page, {.ordinal = 1, .stroke = second}).has_value());
    }

    const Result<NotebookStore> reopened = NotebookStore::open(notebook.path());
    ASSERT_TRUE(reopened.has_value()) << reopened.error().message;
    const Result<std::vector<PlacedStroke>> strokes = reopened->strokesOfPage(page);
    ASSERT_TRUE(strokes.has_value()) << strokes.error().message;

    ASSERT_EQ(strokes->size(), 2U);
    EXPECT_EQ(strokes->front().stroke.id(), first.id());
    EXPECT_EQ(strokes->back().stroke.id(), second.id());
    EXPECT_EQ(strokes->front().stroke.samples().size(), first.samples().size());
}

TEST(NotebookStoreTest, KeepsPagesApart) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid firstPage = ids.next();
    const Uuid secondPage = ids.next();

    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    ASSERT_TRUE(store->insertStroke(firstPage, {.ordinal = 0, .stroke = makeStroke(ids, 0.0F)})
                    .has_value());
    ASSERT_TRUE(store->insertStroke(secondPage, {.ordinal = 1, .stroke = makeStroke(ids, 0.0F)})
                    .has_value());
    ASSERT_TRUE(store->insertStroke(secondPage, {.ordinal = 2, .stroke = makeStroke(ids, 0.0F)})
                    .has_value());

    EXPECT_EQ(store->strokesOfPage(firstPage)->size(), 1U);
    EXPECT_EQ(store->strokesOfPage(secondPage)->size(), 2U);
    EXPECT_TRUE(store->strokesOfPage(ids.next())->empty());
}

TEST(NotebookStoreTest, RemovesOneStrokeAndLeavesTheRest) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Stroke first = makeStroke(ids, 10.0F);
    const Stroke second = makeStroke(ids, 100.0F);
    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 0, .stroke = first}).has_value());
    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 1, .stroke = second}).has_value());

    EXPECT_TRUE(store->removeStroke(page, first.id()).has_value());

    const Result<std::vector<PlacedStroke>> strokes = store->strokesOfPage(page);
    ASSERT_TRUE(strokes.has_value()) << strokes.error().message;
    ASSERT_EQ(strokes->size(), 1U);
    EXPECT_EQ(strokes->front().stroke.id(), second.id());
}

TEST(NotebookStoreTest, ReportsAStrokeThePageDoesNotHold) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    const Uuid otherPage = ids.next();
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Stroke stroke = makeStroke(ids, 10.0F);
    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 0, .stroke = stroke}).has_value());

    const Result<void> unknownStroke = store->removeStroke(page, ids.next());
    ASSERT_FALSE(unknownStroke.has_value());
    EXPECT_EQ(unknownStroke.error().code, ErrorCode::NotFound);

    const Result<void> wrongPage = store->removeStroke(otherPage, stroke.id());
    ASSERT_FALSE(wrongPage.has_value());
    EXPECT_EQ(wrongPage.error().code, ErrorCode::NotFound);
    EXPECT_EQ(store->strokesOfPage(page)->size(), 1U);
}

TEST(NotebookStoreTest, EmptiesOnePageAndCountsWhatItRemoved) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid emptied = ids.next();
    const Uuid kept = ids.next();
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    ASSERT_TRUE(
        store->insertStroke(emptied, {.ordinal = 0, .stroke = makeStroke(ids, 0.0F)}).has_value());
    ASSERT_TRUE(
        store->insertStroke(emptied, {.ordinal = 1, .stroke = makeStroke(ids, 50.0F)}).has_value());
    ASSERT_TRUE(
        store->insertStroke(kept, {.ordinal = 2, .stroke = makeStroke(ids, 0.0F)}).has_value());

    const Result<std::size_t> removed = store->removeStrokesOfPage(emptied);

    ASSERT_TRUE(removed.has_value()) << removed.error().message;
    EXPECT_EQ(*removed, 2U);
    EXPECT_TRUE(store->strokesOfPage(emptied)->empty());
    EXPECT_EQ(store->strokesOfPage(kept)->size(), 1U);
}

TEST(NotebookStoreTest, EmptyingAPageWithoutStrokesRemovesNothing) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;

    const Result<std::size_t> removed = store->removeStrokesOfPage(ids.next());

    ASSERT_TRUE(removed.has_value()) << removed.error().message;
    EXPECT_EQ(*removed, 0U);
}

TEST(NotebookStoreTest, KeepsThePlaceOfEveryStroke) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Stroke late = makeStroke(ids, 0.0F);
    const Stroke early = makeStroke(ids, 10.0F);
    const Stroke middle = makeStroke(ids, 20.0F);
    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 7, .stroke = late}));
    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 2, .stroke = early}));
    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 5, .stroke = middle}));

    const Result<std::vector<PlacedStroke>> strokes = store->strokesOfPage(page);

    ASSERT_TRUE(strokes.has_value()) << strokes.error().message;
    ASSERT_EQ(strokes->size(), 3U);
    EXPECT_EQ((*strokes)[0].ordinal, 2);
    EXPECT_EQ((*strokes)[0].stroke.id(), early.id());
    EXPECT_EQ((*strokes)[1].ordinal, 5);
    EXPECT_EQ((*strokes)[1].stroke.id(), middle.id());
    EXPECT_EQ((*strokes)[2].ordinal, 7);
    EXPECT_EQ((*strokes)[2].stroke.id(), late.id());
}

TEST(NotebookStoreTest, RefusesTwoStrokesInOnePlaceOfAPage) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    const Uuid otherPage = ids.next();
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 0, .stroke = makeStroke(ids, 0.0F)}));

    const Result<void> samePlace =
        store->insertStroke(page, {.ordinal = 0, .stroke = makeStroke(ids, 10.0F)});
    const Result<void> otherPagePlace =
        store->insertStroke(otherPage, {.ordinal = 0, .stroke = makeStroke(ids, 10.0F)});

    ASSERT_FALSE(samePlace.has_value());
    EXPECT_EQ(samePlace.error().code, ErrorCode::IoFailure);
    EXPECT_TRUE(otherPagePlace.has_value()) << otherPagePlace.error().message;
    EXPECT_EQ(store->strokesOfPage(page)->size(), 1U);
}

TEST(NotebookStoreTest, UpgradesANotebookFromSchemaVersion1) {
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
            CREATE INDEX strokes_by_page ON strokes (page_id, ordinal);
            PRAGMA user_version = 1;
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

    Result<NotebookStore> store = NotebookStore::open(notebook.path());

    ASSERT_TRUE(store.has_value()) << store.error().message;
    EXPECT_EQ(store->schemaVersion(), kNotebookSchemaVersion);
    const Result<std::vector<PlacedStroke>> strokes = store->strokesOfPage(page);
    ASSERT_TRUE(strokes.has_value()) << strokes.error().message;
    ASSERT_EQ(strokes->size(), 1U);
    EXPECT_EQ(strokes->front().stroke.id(), stroke.id());
    EXPECT_FALSE(store->insertStroke(page, {.ordinal = 0, .stroke = makeStroke(ids, 0.0F)}));
}

TEST(NotebookStoreTest, AddsThePaperColumnsAnOlderNotebookNeverGot) {
    const TemporaryNotebook notebook;
    {
        const Result<NotebookStore> store = NotebookStore::open(notebook.path());
        ASSERT_TRUE(store.has_value()) << store.error().message;
    }
    // A notebook written before the paper of a page could be chosen, stamped with the version the
    // paper went out under.
    {
        sqlite3* raw = nullptr;
        ASSERT_EQ(sqlite3_open(notebook.path().string().c_str(), &raw), SQLITE_OK);
        ASSERT_EQ(sqlite3_exec(raw, R"sql(
            DROP TABLE page_words;
            DROP TABLE page_texts;
            ALTER TABLE pages DROP COLUMN ink_revision;
            ALTER TABLE pages DROP COLUMN read_revision;
            ALTER TABLE pages DROP COLUMN paper_color;
            ALTER TABLE pages DROP COLUMN line_color;
            ALTER TABLE pages DROP COLUMN margin_color;
            ALTER TABLE pages DROP COLUMN line_width;
            ALTER TABLE pages DROP COLUMN margin_at;
            ALTER TABLE pages DROP COLUMN margin;
            PRAGMA user_version = 4;
        )sql",
                               nullptr, nullptr, nullptr),
                  SQLITE_OK);
        sqlite3_close(raw);
    }

    const Result<NotebookStore> reopened = NotebookStore::open(notebook.path());

    ASSERT_TRUE(reopened.has_value()) << reopened.error().message;
    EXPECT_EQ(reopened->schemaVersion(), kNotebookSchemaVersion);
    const Result<NotebookOutline> outline = reopened->readOutline();
    ASSERT_TRUE(outline.has_value()) << outline.error().message;
    ASSERT_FALSE(outline->sections.empty());
    ASSERT_FALSE(outline->sections.front().pages.empty());
    EXPECT_EQ(outline->sections.front().pages.front().style.paperColor, PageStyle::kUnset);
}

TEST(NotebookStoreTest, LeavesTheColumnsOfANotebookThatAlreadyHasThemAlone) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    {
        Result<NotebookStore> store = NotebookStore::open(notebook.path());
        ASSERT_TRUE(store.has_value()) << store.error().message;
        const NotebookOutline outline = store->readOutline().value();
        PageInfo page;
        page.id = ids.next();
        page.title = "Paper";
        page.style.paperColor = Color{.red = 250, .green = 240, .blue = 200, .alpha = 255};
        ASSERT_TRUE(store->insertPage(
            outline.sections.front().id, page,
            std::vector<Uuid>{outline.sections.front().pages.front().id, page.id}));
    }
    {
        sqlite3* raw = nullptr;
        ASSERT_EQ(sqlite3_open(notebook.path().string().c_str(), &raw), SQLITE_OK);
        ASSERT_EQ(sqlite3_exec(raw, R"sql(
            DROP TABLE page_words;
            DROP TABLE page_texts;
            ALTER TABLE pages DROP COLUMN ink_revision;
            ALTER TABLE pages DROP COLUMN read_revision;
            PRAGMA user_version = 4;
        )sql",
                               nullptr, nullptr, nullptr),
                  SQLITE_OK);
        sqlite3_close(raw);
    }

    const Result<NotebookStore> reopened = NotebookStore::open(notebook.path());

    ASSERT_TRUE(reopened.has_value()) << reopened.error().message;
    EXPECT_EQ(reopened->schemaVersion(), kNotebookSchemaVersion);
    const Result<NotebookOutline> outline = reopened->readOutline();
    ASSERT_TRUE(outline.has_value()) << outline.error().message;
    ASSERT_EQ(outline->sections.front().pages.size(), 2U);
    EXPECT_EQ(outline->sections.front().pages.back().style.paperColor,
              (Color{.red = 250, .green = 240, .blue = 200, .alpha = 255}));
}

TEST(NotebookStoreTest, RefusesANotebookFromANewerVersion) {
    const TemporaryNotebook notebook;
    {
        const Result<NotebookStore> store = NotebookStore::open(notebook.path());
        ASSERT_TRUE(store.has_value()) << store.error().message;
    }

    const std::filesystem::path& path = notebook.path();
    {
        sqlite3* raw = nullptr;
        ASSERT_EQ(sqlite3_open(path.string().c_str(), &raw), SQLITE_OK);
        ASSERT_EQ(sqlite3_exec(
                      raw,
                      ("PRAGMA user_version = " + std::to_string(kNotebookSchemaVersion + 1) + ";")
                          .c_str(),
                      nullptr, nullptr, nullptr),
                  SQLITE_OK);
        sqlite3_close(raw);
    }

    const Result<NotebookStore> reopened = NotebookStore::open(path);

    ASSERT_FALSE(reopened.has_value());
    EXPECT_EQ(reopened.error().code, ErrorCode::Unsupported);
}

TEST(NotebookStoreTest, ReportsAPathItCannotOpen) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "phvikapen-missing-directory" / "notebook.phvika";

    const Result<NotebookStore> store = NotebookStore::open(path);

    ASSERT_FALSE(store.has_value());
    EXPECT_EQ(store.error().code, ErrorCode::IoFailure);
}

}
}
