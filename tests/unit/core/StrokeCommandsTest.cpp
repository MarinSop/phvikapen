#include "core/undo/StrokeCommands.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/Stroke.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/undo/UndoStack.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace phvikapen::core {
namespace {

using test::makeStroke;
using test::TemporaryNotebook;

[[nodiscard]] std::vector<Uuid> idsOnPage(const NotebookStore& store, const Uuid& page) {
    std::vector<Uuid> ids;
    const Result<std::vector<PlacedStroke>> strokes = store.strokesOfPage(page);
    if (!strokes) {
        return ids;
    }
    for (const PlacedStroke& placed : *strokes) {
        ids.push_back(placed.stroke.id());
    }
    return ids;
}

TEST(AddStrokeCommandTest, PutsTheStrokeOnThePageAndTakesItOffAgain) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Stroke stroke = makeStroke(ids, 10.0F);
    AddStrokeCommand command{&*store, page, {.ordinal = 0, .stroke = stroke}};

    ASSERT_TRUE(command.apply());
    EXPECT_EQ(idsOnPage(*store, page), std::vector<Uuid>{stroke.id()});

    ASSERT_TRUE(command.revert());
    EXPECT_TRUE(idsOnPage(*store, page).empty());
}

TEST(AddStrokeCommandTest, PutsTheStrokeBackWhereItWas) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Stroke first = makeStroke(ids, 10.0F);
    const Stroke second = makeStroke(ids, 100.0F);
    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 0, .stroke = first}));
    AddStrokeCommand command{&*store, page, {.ordinal = 1, .stroke = second}};
    ASSERT_TRUE(command.apply());

    ASSERT_TRUE(command.revert());
    ASSERT_TRUE(command.apply());

    EXPECT_EQ(idsOnPage(*store, page), (std::vector<Uuid>{first.id(), second.id()}));
    const Result<std::vector<PlacedStroke>> strokes = store->strokesOfPage(page);
    ASSERT_TRUE(strokes.has_value()) << strokes.error().message;
    EXPECT_EQ(strokes->back().stroke.samples().size(), second.samples().size());
}

TEST(ClearPageCommandTest, EmptiesThePageAndBringsEverythingBackInOrder) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Stroke first = makeStroke(ids, 10.0F);
    const Stroke second = makeStroke(ids, 100.0F);
    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 0, .stroke = first}));
    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 1, .stroke = second}));
    ClearPageCommand command{&*store, page};

    ASSERT_TRUE(command.apply());
    EXPECT_TRUE(idsOnPage(*store, page).empty());

    ASSERT_TRUE(command.revert());
    EXPECT_EQ(idsOnPage(*store, page), (std::vector<Uuid>{first.id(), second.id()}));
}

TEST(ClearPageCommandTest, LeavesTheOtherPagesAlone) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid cleared = ids.next();
    const Uuid kept = ids.next();
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    ASSERT_TRUE(store->insertStroke(cleared, {.ordinal = 0, .stroke = makeStroke(ids, 10.0F)}));
    const Stroke elsewhere = makeStroke(ids, 100.0F);
    ASSERT_TRUE(store->insertStroke(kept, {.ordinal = 1, .stroke = elsewhere}));
    ClearPageCommand command{&*store, cleared};

    ASSERT_TRUE(command.apply());

    EXPECT_TRUE(idsOnPage(*store, cleared).empty());
    EXPECT_EQ(idsOnPage(*store, kept), std::vector<Uuid>{elsewhere.id()});
}

TEST(ClearPageCommandTest, ClearingAnEmptyPageCanStillBeUndone) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    ClearPageCommand command{&*store, page};

    EXPECT_TRUE(command.apply());
    EXPECT_TRUE(command.revert());
    EXPECT_TRUE(idsOnPage(*store, page).empty());
}

TEST(StrokeCommandsTest, DrawingAndClearingWalkBackAndForwardThroughTheHistory) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Stroke first = makeStroke(ids, 10.0F);
    const Stroke second = makeStroke(ids, 100.0F);
    UndoStack history;

    ASSERT_TRUE(history.run(std::make_unique<AddStrokeCommand>(
        &*store, page, PlacedStroke{.ordinal = 0, .stroke = first})));
    ASSERT_TRUE(history.run(std::make_unique<AddStrokeCommand>(
        &*store, page, PlacedStroke{.ordinal = 1, .stroke = second})));
    ASSERT_TRUE(history.run(std::make_unique<ClearPageCommand>(&*store, page)));
    EXPECT_TRUE(idsOnPage(*store, page).empty());

    ASSERT_TRUE(history.undo());
    EXPECT_EQ(idsOnPage(*store, page), (std::vector<Uuid>{first.id(), second.id()}));

    ASSERT_TRUE(history.undo());
    EXPECT_EQ(idsOnPage(*store, page), std::vector<Uuid>{first.id()});

    ASSERT_TRUE(history.redo());
    EXPECT_EQ(idsOnPage(*store, page), (std::vector<Uuid>{first.id(), second.id()}));

    ASSERT_TRUE(history.redo());
    EXPECT_TRUE(idsOnPage(*store, page).empty());
}

}
}
