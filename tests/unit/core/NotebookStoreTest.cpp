#include "core/storage/NotebookStore.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/Stroke.hpp"
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
        ASSERT_TRUE(store->appendStroke(page, first).has_value());
        ASSERT_TRUE(store->appendStroke(page, second).has_value());
    }

    const Result<NotebookStore> reopened = NotebookStore::open(notebook.path());
    ASSERT_TRUE(reopened.has_value()) << reopened.error().message;
    const Result<std::vector<Stroke>> strokes = reopened->strokesOfPage(page);
    ASSERT_TRUE(strokes.has_value()) << strokes.error().message;

    ASSERT_EQ(strokes->size(), 2U);
    // The order the strokes were drawn in is the order they come back in.
    EXPECT_EQ(strokes->front().id(), first.id());
    EXPECT_EQ(strokes->back().id(), second.id());
    EXPECT_EQ(strokes->front().samples().size(), first.samples().size());
}

TEST(NotebookStoreTest, KeepsPagesApart) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid firstPage = ids.next();
    const Uuid secondPage = ids.next();

    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    ASSERT_TRUE(store->appendStroke(firstPage, makeStroke(ids, 0.0F)).has_value());
    ASSERT_TRUE(store->appendStroke(secondPage, makeStroke(ids, 0.0F)).has_value());
    ASSERT_TRUE(store->appendStroke(secondPage, makeStroke(ids, 0.0F)).has_value());

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
    ASSERT_TRUE(store->appendStroke(page, first).has_value());
    ASSERT_TRUE(store->appendStroke(page, second).has_value());

    EXPECT_TRUE(store->removeStroke(page, first.id()).has_value());

    const Result<std::vector<Stroke>> strokes = store->strokesOfPage(page);
    ASSERT_TRUE(strokes.has_value()) << strokes.error().message;
    ASSERT_EQ(strokes->size(), 1U);
    EXPECT_EQ(strokes->front().id(), second.id());
}

TEST(NotebookStoreTest, ReportsAStrokeThePageDoesNotHold) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    const Uuid otherPage = ids.next();
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Stroke stroke = makeStroke(ids, 10.0F);
    ASSERT_TRUE(store->appendStroke(page, stroke).has_value());

    const Result<void> unknownStroke = store->removeStroke(page, ids.next());
    ASSERT_FALSE(unknownStroke.has_value());
    EXPECT_EQ(unknownStroke.error().code, ErrorCode::NotFound);

    // The stroke exists, but not on the page it is asked for.
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
    ASSERT_TRUE(store->appendStroke(emptied, makeStroke(ids, 0.0F)).has_value());
    ASSERT_TRUE(store->appendStroke(emptied, makeStroke(ids, 50.0F)).has_value());
    ASSERT_TRUE(store->appendStroke(kept, makeStroke(ids, 0.0F)).has_value());

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

TEST(NotebookStoreTest, RefusesANotebookFromANewerVersion) {
    const TemporaryNotebook notebook;
    {
        const Result<NotebookStore> store = NotebookStore::open(notebook.path());
        ASSERT_TRUE(store.has_value()) << store.error().message;
    }

    // Pretend a later version of the application wrote this file.
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

} // namespace
} // namespace phvikapen::core
