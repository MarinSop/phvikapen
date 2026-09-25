#include "core/model/Layer.hpp"

#include "core/Error.hpp"
#include "core/geometry/Distance.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/InkSample.hpp"
#include "core/ink/Stroke.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/Picture.hpp"
#include "core/model/Table.hpp"
#include "core/model/TextBox.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"
#include "core/undo/LayerCommands.hpp"
#include "core/undo/UndoStack.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

using test::TemporaryNotebook;

[[nodiscard]] Layer named(Uuid7Generator& ids, const std::string& name) {
    return Layer{.id = ids.next(), .name = name, .shown = true, .locked = false};
}

[[nodiscard]] Stroke strokeAt(Uuid7Generator& ids, float x) {
    Stroke stroke{ids.next()};
    stroke.append(InkSample{.x = x, .y = 0.0F});
    stroke.append(InkSample{.x = x + 10.0F, .y = 10.0F});
    return stroke;
}

[[nodiscard]] Uuid firstPageOf(const NotebookStore& store) {
    return store.readOutline().value().sections.front().pages.front().id;
}

TEST(LayerTest, WhereALayerStandsIsCountedFromTheBottom) {
    Uuid7Generator ids;
    const std::vector<Layer> layers{named(ids, "Paper"), named(ids, "Ink"), named(ids, "Notes")};

    EXPECT_EQ(placeOfLayer(layers, layers[0].id), 0U);
    EXPECT_EQ(placeOfLayer(layers, layers[2].id), 2U);
    EXPECT_EQ(placeOfLayer(layers, Uuid{}), layers.size());
}

TEST(LayerTest, AThingOnNoLayerAtAllStandsOnTheBottomOne) {
    Uuid7Generator ids;
    const std::vector<Layer> layers{named(ids, "Paper"), named(ids, "Ink")};

    const Layer* const found = layerOf(layers, Uuid{});

    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "Paper");
    EXPECT_EQ(layerOf({}, Uuid{}), nullptr);
}

TEST(LayerTest, ALayerSaysWhetherItMayBeDrawnOnAndWhetherItIsSeen) {
    Uuid7Generator ids;
    std::vector<Layer> layers{named(ids, "Paper"), named(ids, "Ink")};
    layers[0].shown = false;
    layers[1].locked = true;

    EXPECT_FALSE(isShownOn(layers, layers[0].id));
    EXPECT_TRUE(isShownOn(layers, layers[1].id));
    EXPECT_TRUE(isLockedOn(layers, layers[1].id));
    EXPECT_FALSE(isOpenToTheHand(layers[0]));
    EXPECT_FALSE(isOpenToTheHand(layers[1]));
    // Nothing at all is shown and open, so that a page with no layers behaves as it always did.
    EXPECT_TRUE(isShownOn({}, Uuid{}));
    EXPECT_FALSE(isLockedOn({}, Uuid{}));
}

TEST(LayerTest, ANewLayerTakesANameNoOtherCarries) {
    Uuid7Generator ids;
    const std::vector<Layer> layers{named(ids, "Layer 1"), named(ids, "Layer 1 2")};

    EXPECT_EQ(freeName(layers, "Notes"), "Notes");
    EXPECT_EQ(freeName(layers, "Layer 1"), "Layer 1 3");
}

TEST(LayerTest, ALayerCarriedToAnotherPlaceLeavesTheRestInOrder) {
    Uuid7Generator ids;
    const std::vector<Layer> layers{named(ids, "A"), named(ids, "B"), named(ids, "C")};

    const std::vector<Layer> moved = withLayerMoved(layers, 0, 2);

    ASSERT_EQ(moved.size(), 3U);
    EXPECT_EQ(moved[0].name, "B");
    EXPECT_EQ(moved[1].name, "C");
    EXPECT_EQ(moved[2].name, "A");
    EXPECT_EQ(withLayerMoved(layers, 1, 1), layers);
    EXPECT_EQ(withLayerMoved(layers, 9, 0), layers);
}

TEST(PageLayersTest, APageAlwaysHasALayerToStandThingsOn) {
    Uuid7Generator ids;
    const Page page{ids.next(), {}};

    ASSERT_EQ(page.layers().size(), 1U);
    EXPECT_FALSE(page.layers().front().name.empty());
    EXPECT_TRUE(page.layers().front().shown);
}

TEST(PageLayersTest, EverythingOnAPageSaysWhichLayerItBelongsTo) {
    Uuid7Generator ids;
    const Layer paper = named(ids, "Paper");
    const Layer notes = named(ids, "Notes");
    Page page{ids.next(), {}, {}, {}, {}, {paper, notes}};
    const Stroke stroke = strokeAt(ids, 0.0F);
    const Uuid strokeId = stroke.id();
    ASSERT_TRUE(page.insert(PlacedStroke{.ordinal = 0, .stroke = stroke, .layer = paper.id}));

    EXPECT_EQ(page.countOnLayer(paper.id), 1);
    EXPECT_EQ(page.countOnLayer(notes.id), 0);

    const Result<Uuid> stood = page.moveToLayer(strokeId, notes.id);

    ASSERT_TRUE(stood.has_value());
    EXPECT_EQ(*stood, paper.id);
    EXPECT_EQ(page.countOnLayer(notes.id), 1);
    EXPECT_FALSE(page.moveToLayer(Uuid{}, notes.id).has_value());
}

TEST(PageLayersTest, ATableAPictureAndABoxOfTypeAllCarryALayer) {
    Uuid7Generator ids;
    const Layer paper = named(ids, "Paper");
    const Layer notes = named(ids, "Notes");
    Page page{ids.next(), {}, {}, {}, {}, {paper, notes}};
    TextBox box;
    box.id = ids.next();
    Picture picture;
    picture.id = ids.next();
    Table table = gridOf(2, 2);
    table.id = ids.next();
    ASSERT_TRUE(page.insertText(PlacedText{.ordinal = 0, .box = box, .layer = paper.id}));
    ASSERT_TRUE(
        page.insertPicture(PlacedPicture{.ordinal = 0, .picture = picture, .layer = paper.id}));
    ASSERT_TRUE(page.insertTable(PlacedTable{.ordinal = 0, .table = table, .layer = notes.id}));

    EXPECT_EQ(page.countOnLayer(paper.id), 2);
    EXPECT_EQ(page.countOnLayer(notes.id), 1);
    ASSERT_TRUE(page.moveToLayer(picture.id, notes.id).has_value());
    EXPECT_EQ(page.countOnLayer(notes.id), 2);
}

TEST(NotebookLayersTest, KeepsTheLayersOfAPageAndTheOrderTheyStandIn) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    std::vector<Layer> layers{named(ids, "Paper"), named(ids, "Notes")};
    layers[0].locked = true;
    layers[1].shown = false;

    ASSERT_TRUE(store->writeLayers(page, layers));

    const Result<std::vector<Layer>> kept = store->layersOfPage(page);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    EXPECT_EQ(*kept, layers);
}

TEST(NotebookLayersTest, WritingTheLayersAgainLeavesNoneOfTheOldOnes) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    ASSERT_TRUE(store->writeLayers(page, std::vector<Layer>{named(ids, "A"), named(ids, "B")}));

    ASSERT_TRUE(store->writeLayers(page, std::vector<Layer>{named(ids, "C")}));

    const Result<std::vector<Layer>> kept = store->layersOfPage(page);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    ASSERT_EQ(kept->size(), 1U);
    EXPECT_EQ(kept->front().name, "C");
}

TEST(NotebookLayersTest, KeepsWhichLayerEverythingOnAPageStandsOn) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    const Layer notes = named(ids, "Notes");
    ASSERT_TRUE(store->writeLayers(page, std::vector<Layer>{notes}));
    const Stroke stroke = strokeAt(ids, 5.0F);
    ASSERT_TRUE(
        store->insertStroke(page, PlacedStroke{.ordinal = 0, .stroke = stroke, .layer = notes.id}));
    Table table = gridOf(2, 2);
    table.id = ids.next();
    ASSERT_TRUE(
        store->insertTable(page, PlacedTable{.ordinal = 0, .table = table, .layer = notes.id}));

    const Result<std::vector<PlacedStroke>> strokes = store->strokesOfPage(page);
    const Result<std::vector<PlacedTable>> tables = store->tablesOfPage(page);

    ASSERT_TRUE(strokes.has_value()) << strokes.error().message;
    ASSERT_EQ(strokes->size(), 1U);
    EXPECT_EQ(strokes->front().layer, notes.id);
    ASSERT_TRUE(tables.has_value()) << tables.error().message;
    ASSERT_EQ(tables->size(), 1U);
    EXPECT_EQ(tables->front().layer, notes.id);
}

TEST(NotebookLayersTest, CarriesOneThingFromOneLayerToAnother) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    const Layer paper = named(ids, "Paper");
    const Layer notes = named(ids, "Notes");
    ASSERT_TRUE(store->writeLayers(page, std::vector<Layer>{paper, notes}));
    const Stroke stroke = strokeAt(ids, 5.0F);
    ASSERT_TRUE(
        store->insertStroke(page, PlacedStroke{.ordinal = 0, .stroke = stroke, .layer = paper.id}));

    ASSERT_TRUE(store->moveToLayer(page, stroke.id(), notes.id));

    const Result<std::vector<PlacedStroke>> strokes = store->strokesOfPage(page);
    ASSERT_TRUE(strokes.has_value()) << strokes.error().message;
    ASSERT_EQ(strokes->size(), 1U);
    EXPECT_EQ(strokes->front().layer, notes.id);
}

TEST(NotebookLayersTest, ANotebookMadeBeforeThereWereLayersOpensWithNone) {
    const TemporaryNotebook notebook;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);

    const Result<std::vector<Layer>> kept = store->layersOfPage(page);

    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    EXPECT_TRUE(kept->empty());
    // A page given none makes itself one, so everything on it still has somewhere to stand.
    const Page made{page, {}, {}, {}, {}, *kept};
    EXPECT_EQ(made.layers().size(), 1U);
}

struct OpenNotebook {
    TemporaryNotebook file;
    Uuid7Generator ids;
    std::vector<Error> errors;
    StorageThread storage{file.path(), [this](const Error& error) { errors.push_back(error); }};
};

TEST(LayerCommandsTest, ChangingTheLayersCanBeTakenBackExactly) {
    OpenNotebook notebook;
    Uuid7Generator& ids = notebook.ids;
    StorageThread& storage = notebook.storage;
    Page page{ids.next(), {}, {}, {}, {}, {named(ids, "Paper")}};
    const std::vector<Layer> before{page.layers().begin(), page.layers().end()};
    std::vector<Layer> wanted = before;
    wanted.push_back(named(ids, "Notes"));
    UndoStack undo;

    ASSERT_TRUE(
        undo.run(std::make_unique<ChangeLayersCommand>(&page, &storage, std::move(wanted))));

    EXPECT_EQ(page.layers().size(), 2U);

    ASSERT_TRUE(undo.undo());

    ASSERT_EQ(page.layers().size(), 1U);
    EXPECT_EQ(page.layers().front(), before.front());
}

TEST(LayerCommandsTest, CarryingAThingToAnotherLayerCanBeTakenBack) {
    OpenNotebook notebook;
    Uuid7Generator& ids = notebook.ids;
    StorageThread& storage = notebook.storage;
    const Layer paper = named(ids, "Paper");
    const Layer notes = named(ids, "Notes");
    Page page{ids.next(), {}, {}, {}, {}, {paper, notes}};
    const Stroke stroke = strokeAt(ids, 0.0F);
    ASSERT_TRUE(page.insert(PlacedStroke{.ordinal = 0, .stroke = stroke, .layer = paper.id}));
    UndoStack undo;

    ASSERT_TRUE(
        undo.run(std::make_unique<MoveToLayerCommand>(&page, &storage, stroke.id(), notes.id)));

    EXPECT_EQ(page.countOnLayer(notes.id), 1);

    ASSERT_TRUE(undo.undo());

    EXPECT_EQ(page.countOnLayer(paper.id), 1);
    EXPECT_EQ(page.countOnLayer(notes.id), 0);
}

}
}
