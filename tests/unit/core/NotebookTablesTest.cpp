#include "core/Error.hpp"
#include "core/geometry/Distance.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/Table.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"
#include "core/text/InkWord.hpp"
#include "core/undo/TableCommands.hpp"
#include "core/undo/UndoStack.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

using test::TemporaryNotebook;

[[nodiscard]] Table tableAt(Uuid7Generator& ids, float x, float y) {
    Table table = gridOf(2, 2);
    table.id = ids.next();
    table.at = Point{.x = x, .y = y};
    table.columns = {40.0F, 60.0F};
    table.rows = {20.0F, 30.0F};
    return withCellWritten(std::move(table), CellAt{.row = 0, .column = 1}, "Monday").value();
}

[[nodiscard]] Uuid firstPageOf(const NotebookStore& store) {
    return store.readOutline().value().sections.front().pages.front().id;
}

TEST(NotebookTablesTest, KeepsATableThatWasPutOnAPage) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    const Table table = tableAt(ids, 10.0F, 20.0F);

    ASSERT_TRUE(store->insertTable(page, PlacedTable{.ordinal = 0, .table = table}));

    const Result<std::vector<PlacedTable>> kept = store->tablesOfPage(page);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    ASSERT_EQ(kept->size(), 1U);
    EXPECT_EQ(kept->front().ordinal, 0);
    EXPECT_EQ(kept->front().table, table);
}

TEST(NotebookTablesTest, ListsTablesInTheOrderTheyWerePutDown) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);

    ASSERT_TRUE(store->insertTable(page, {.ordinal = 1, .table = tableAt(ids, 90.0F, 0.0F)}));
    ASSERT_TRUE(store->insertTable(page, {.ordinal = 0, .table = tableAt(ids, 10.0F, 0.0F)}));

    const Result<std::vector<PlacedTable>> kept = store->tablesOfPage(page);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    ASSERT_EQ(kept->size(), 2U);
    EXPECT_FLOAT_EQ(kept->front().table.at.x, 10.0F);
    EXPECT_FLOAT_EQ(kept->back().table.at.x, 90.0F);
}

TEST(NotebookTablesTest, WritesOverATableThatIsAlreadyThere) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    Table table = tableAt(ids, 10.0F, 20.0F);
    ASSERT_TRUE(store->insertTable(page, PlacedTable{.ordinal = 0, .table = table}));

    table.at = Point{.x = 40.0F, .y = 60.0F};
    table = withColumnAdded(std::move(table), 2).value();
    table = withCellWritten(std::move(table), CellAt{.row = 1, .column = 2}, "Tuesday").value();
    ASSERT_TRUE(store->updateTable(page, table));

    const Result<std::vector<PlacedTable>> kept = store->tablesOfPage(page);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    ASSERT_EQ(kept->size(), 1U);
    EXPECT_EQ(kept->front().table, table);
}

TEST(NotebookTablesTest, ForgetsWhatWasRubbedOutOfABox) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    Table table = tableAt(ids, 10.0F, 20.0F);
    ASSERT_TRUE(store->insertTable(page, PlacedTable{.ordinal = 0, .table = table}));

    table = withCellWritten(std::move(table), CellAt{.row = 0, .column = 1}, "").value();
    ASSERT_TRUE(store->updateTable(page, table));

    const Result<std::vector<PlacedTable>> kept = store->tablesOfPage(page);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    EXPECT_TRUE(kept->front().table.cells.at(1).text.empty());
}

TEST(NotebookTablesTest, TakesATableOffAPage) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    const Table table = tableAt(ids, 10.0F, 20.0F);
    ASSERT_TRUE(store->insertTable(page, PlacedTable{.ordinal = 0, .table = table}));

    ASSERT_TRUE(store->removeTable(page, table.id));

    EXPECT_TRUE(store->tablesOfPage(page).value().empty());
    EXPECT_TRUE(store->findWords("Monday").value().empty());
}

TEST(NotebookTablesTest, ClearsAllTheTablesOfAPage) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    ASSERT_TRUE(store->insertTable(page, {.ordinal = 0, .table = tableAt(ids, 0.0F, 0.0F)}));
    ASSERT_TRUE(store->insertTable(page, {.ordinal = 1, .table = tableAt(ids, 5.0F, 5.0F)}));

    const Result<std::size_t> gone = store->removeTablesOfPage(page);

    ASSERT_TRUE(gone.has_value()) << gone.error().message;
    EXPECT_EQ(*gone, 2U);
    EXPECT_TRUE(store->tablesOfPage(page).value().empty());
    EXPECT_TRUE(store->findWords("Monday").value().empty());
}

TEST(NotebookTablesTest, FindsWordsTypedIntoABoxWhereThatBoxStands) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    const Table table = tableAt(ids, 10.0F, 20.0F);
    ASSERT_TRUE(store->insertTable(page, PlacedTable{.ordinal = 0, .table = table}));

    const Result<std::vector<FoundWord>> found = store->findWords("monday");

    ASSERT_TRUE(found.has_value()) << found.error().message;
    ASSERT_EQ(found->size(), 1U);
    EXPECT_EQ(found->front().pageId, page);
    EXPECT_EQ(found->front().word.text, "Monday");
    EXPECT_EQ(found->front().word.box, areaOfCell(table, CellAt{.row = 0, .column = 1}));
}

TEST(NotebookTablesTest, EmptyingTheTrashTakesTheTablesWithIt) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    ASSERT_TRUE(store->insertTable(page, {.ordinal = 0, .table = tableAt(ids, 0.0F, 0.0F)}));
    ASSERT_TRUE(store->trashPage(page));

    ASSERT_TRUE(store->emptyTrash());

    EXPECT_TRUE(store->tablesOfPage(page).value().empty());
    EXPECT_TRUE(store->findWords("Monday").value().empty());
}

TEST(NotebookTablesTest, OpensANotebookAtTheVersionThatHoldsTables) {
    const TemporaryNotebook notebook;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;

    EXPECT_EQ(store->schemaVersion().value(), kNotebookSchemaVersion);
    EXPECT_GE(kNotebookSchemaVersion, 9);
}

struct OpenNotebook {
    TemporaryNotebook file;
    Uuid7Generator ids;
    std::vector<Error> errors;
    StorageThread storage{file.path(), [this](const Error& error) { errors.push_back(error); }};

    [[nodiscard]] Uuid firstPage() {
        storage.waitUntilIdle();
        const Result<NotebookStore> store = NotebookStore::open(file.path());
        return store->readOutline().value().sections.front().pages.front().id;
    }

    [[nodiscard]] std::vector<PlacedTable> inFile(const Uuid& page) {
        storage.waitUntilIdle();
        const Result<NotebookStore> store = NotebookStore::open(file.path());
        return store->tablesOfPage(page).value_or(std::vector<PlacedTable>{});
    }
};

TEST(TableCommandsTest, PutsATableOnThePageAndTakesItOffAgain) {
    OpenNotebook notebook;
    UndoStack history;
    Page page{notebook.firstPage()};
    const Table table = tableAt(notebook.ids, 10.0F, 20.0F);

    ASSERT_TRUE(history
                    .run(std::make_unique<AddTableCommand>(
                        &page, &notebook.storage, PlacedTable{.ordinal = 0, .table = table}))
                    .has_value());

    EXPECT_EQ(page.tables().size(), 1U);
    EXPECT_EQ(notebook.inFile(page.id()).size(), 1U);

    ASSERT_TRUE(history.undo().has_value());

    EXPECT_TRUE(page.tables().empty());
    EXPECT_TRUE(notebook.inFile(page.id()).empty());
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(TableCommandsTest, TakesATableAwayAndBringsItBack) {
    OpenNotebook notebook;
    UndoStack history;
    Page page{notebook.firstPage()};
    const Table table = tableAt(notebook.ids, 10.0F, 20.0F);
    ASSERT_TRUE(history
                    .run(std::make_unique<AddTableCommand>(
                        &page, &notebook.storage, PlacedTable{.ordinal = 0, .table = table}))
                    .has_value());

    ASSERT_TRUE(
        history.run(std::make_unique<RemoveTableCommand>(&page, &notebook.storage, table.id))
            .has_value());

    EXPECT_TRUE(page.tables().empty());

    ASSERT_TRUE(history.undo().has_value());

    ASSERT_EQ(page.tables().size(), 1U);
    EXPECT_EQ(page.tables().front().table, table);
    EXPECT_EQ(notebook.inFile(page.id()).size(), 1U);
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(TableCommandsTest, TypingIntoABoxCanBeUndoneExactly) {
    OpenNotebook notebook;
    UndoStack history;
    Page page{notebook.firstPage()};
    const Table table = tableAt(notebook.ids, 10.0F, 20.0F);
    ASSERT_TRUE(history
                    .run(std::make_unique<AddTableCommand>(
                        &page, &notebook.storage, PlacedTable{.ordinal = 0, .table = table}))
                    .has_value());

    const Table written = withCellWritten(table, CellAt{.row = 1, .column = 0}, "Tuesday").value();
    ASSERT_TRUE(history.run(std::make_unique<ChangeTableCommand>(&page, &notebook.storage, written))
                    .has_value());

    EXPECT_EQ(page.tables().front().table, written);
    EXPECT_EQ(notebook.inFile(page.id()).front().table, written);

    ASSERT_TRUE(history.undo().has_value());

    EXPECT_EQ(page.tables().front().table, table);
    EXPECT_EQ(notebook.inFile(page.id()).front().table, table);

    ASSERT_TRUE(history.redo().has_value());

    EXPECT_EQ(page.tables().front().table, written);
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(TableCommandsTest, GrowingATableCanBeUndoneExactly) {
    OpenNotebook notebook;
    UndoStack history;
    Page page{notebook.firstPage()};
    const Table table = tableAt(notebook.ids, 10.0F, 20.0F);
    ASSERT_TRUE(history
                    .run(std::make_unique<AddTableCommand>(
                        &page, &notebook.storage, PlacedTable{.ordinal = 0, .table = table}))
                    .has_value());

    const Table grown = withRowAdded(table, 0).value();
    ASSERT_TRUE(history.run(std::make_unique<ChangeTableCommand>(&page, &notebook.storage, grown))
                    .has_value());

    EXPECT_EQ(rowsOf(notebook.inFile(page.id()).front().table), 3);
    EXPECT_EQ(cellAt(notebook.inFile(page.id()).front().table, CellAt{.row = 1, .column = 1})->text,
              "Monday");

    ASSERT_TRUE(history.undo().has_value());

    EXPECT_EQ(notebook.inFile(page.id()).front().table, table);
    EXPECT_TRUE(notebook.errors.empty());
}

}
}
