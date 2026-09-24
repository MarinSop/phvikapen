#include "core/undo/StrokeCommands.hpp"

#include "core/Error.hpp"
#include "core/geometry/Distance.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/Stroke.hpp"
#include "core/ink/StrokeTransform.hpp"
#include "core/model/Page.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"
#include "core/undo/UndoStack.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

using test::makeStroke;
using test::TemporaryNotebook;

struct OpenNotebook {
    TemporaryNotebook file;
    Uuid7Generator ids;
    Page page{ids.next()};
    std::vector<Error> errors;
    StorageThread storage{file.path(), [this](const Error& error) { errors.push_back(error); }};
};

[[nodiscard]] std::vector<Uuid> idsOn(const Page& page) {
    std::vector<Uuid> ids;
    for (const PlacedStroke& placed : page.strokes()) {
        ids.push_back(placed.stroke.id());
    }
    return ids;
}

[[nodiscard]] std::vector<Uuid> idsInFile(OpenNotebook& notebook, const Uuid& page) {
    notebook.storage.waitUntilIdle();
    std::vector<Uuid> ids;
    const Result<NotebookStore> store = NotebookStore::open(notebook.file.path());
    if (!store) {
        return ids;
    }
    const Result<std::vector<PlacedStroke>> strokes = store->strokesOfPage(page);
    if (!strokes) {
        return ids;
    }
    for (const PlacedStroke& placed : *strokes) {
        ids.push_back(placed.stroke.id());
    }
    return ids;
}

TEST(AddStrokeCommandTest, PutsTheStrokeOnThePageAndTakesItOffAgain) {
    OpenNotebook notebook;
    const Stroke stroke = makeStroke(notebook.ids, 10.0F);
    AddStrokeCommand command{&notebook.page, &notebook.storage, {.ordinal = 0, .stroke = stroke}};

    ASSERT_TRUE(command.apply());
    EXPECT_EQ(idsOn(notebook.page), std::vector<Uuid>{stroke.id()});
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()), std::vector<Uuid>{stroke.id()});

    ASSERT_TRUE(command.revert());
    EXPECT_TRUE(idsOn(notebook.page).empty());
    EXPECT_TRUE(idsInFile(notebook, notebook.page.id()).empty());
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(AddStrokeCommandTest, PutsTheStrokeBackWhereItWas) {
    OpenNotebook notebook;
    const Stroke first = makeStroke(notebook.ids, 10.0F);
    const Stroke second = makeStroke(notebook.ids, 100.0F);
    const Stroke third = makeStroke(notebook.ids, 200.0F);
    AddStrokeCommand addFirst{&notebook.page, &notebook.storage, {.ordinal = 0, .stroke = first}};
    AddStrokeCommand addSecond{&notebook.page, &notebook.storage, {.ordinal = 1, .stroke = second}};
    AddStrokeCommand addThird{&notebook.page, &notebook.storage, {.ordinal = 2, .stroke = third}};
    ASSERT_TRUE(addFirst.apply());
    ASSERT_TRUE(addSecond.apply());
    ASSERT_TRUE(addThird.apply());

    ASSERT_TRUE(addSecond.revert());
    ASSERT_TRUE(addSecond.apply());

    const std::vector<Uuid> expected{first.id(), second.id(), third.id()};
    EXPECT_EQ(idsOn(notebook.page), expected);
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()), expected);
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(AddStrokeCommandTest, WritesNothingWhenThePlaceIsTaken) {
    OpenNotebook notebook;
    const Stroke kept = makeStroke(notebook.ids, 10.0F);
    AddStrokeCommand addKept{&notebook.page, &notebook.storage, {.ordinal = 0, .stroke = kept}};
    AddStrokeCommand addClash{&notebook.page,
                              &notebook.storage,
                              {.ordinal = 0, .stroke = makeStroke(notebook.ids, 20.0F)}};
    ASSERT_TRUE(addKept.apply());

    const Result<void> applied = addClash.apply();

    ASSERT_FALSE(applied.has_value());
    EXPECT_EQ(applied.error().code, ErrorCode::InvalidArgument);
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()), std::vector<Uuid>{kept.id()});
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(ClearPageCommandTest, EmptiesThePageAndBringsEverythingBackInOrder) {
    OpenNotebook notebook;
    const Stroke first = makeStroke(notebook.ids, 10.0F);
    const Stroke second = makeStroke(notebook.ids, 100.0F);
    AddStrokeCommand addFirst{&notebook.page, &notebook.storage, {.ordinal = 0, .stroke = first}};
    AddStrokeCommand addSecond{&notebook.page, &notebook.storage, {.ordinal = 1, .stroke = second}};
    ASSERT_TRUE(addFirst.apply());
    ASSERT_TRUE(addSecond.apply());
    ClearPageCommand command{&notebook.page, &notebook.storage};

    ASSERT_TRUE(command.apply());
    EXPECT_TRUE(idsOn(notebook.page).empty());
    EXPECT_TRUE(idsInFile(notebook, notebook.page.id()).empty());

    ASSERT_TRUE(command.revert());
    const std::vector<Uuid> expected{first.id(), second.id()};
    EXPECT_EQ(idsOn(notebook.page), expected);
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()), expected);
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(ClearPageCommandTest, LeavesTheOtherPagesInTheFileAlone) {
    OpenNotebook notebook;
    const Uuid otherPage = notebook.ids.next();
    const Stroke elsewhere = makeStroke(notebook.ids, 100.0F);
    notebook.storage.insertStroke(otherPage, {.ordinal = 0, .stroke = elsewhere});
    AddStrokeCommand add{&notebook.page,
                         &notebook.storage,
                         {.ordinal = 0, .stroke = makeStroke(notebook.ids, 10.0F)}};
    ASSERT_TRUE(add.apply());
    ClearPageCommand command{&notebook.page, &notebook.storage};

    ASSERT_TRUE(command.apply());

    EXPECT_TRUE(idsInFile(notebook, notebook.page.id()).empty());
    EXPECT_EQ(idsInFile(notebook, otherPage), std::vector<Uuid>{elsewhere.id()});
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(ClearPageCommandTest, ClearingAnEmptyPageCanStillBeUndone) {
    OpenNotebook notebook;
    ClearPageCommand command{&notebook.page, &notebook.storage};

    EXPECT_TRUE(command.apply());
    EXPECT_TRUE(command.revert());
    EXPECT_TRUE(idsOn(notebook.page).empty());
    EXPECT_TRUE(idsInFile(notebook, notebook.page.id()).empty());
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(EraseStrokesCommandTest, ErasesSeveralStrokesAndPutsThemBackInTheirPlaces) {
    OpenNotebook notebook;
    const Stroke first = makeStroke(notebook.ids, 10.0F);
    const Stroke second = makeStroke(notebook.ids, 100.0F);
    const Stroke third = makeStroke(notebook.ids, 200.0F);
    for (const auto& [ordinal, stroke] :
         {std::pair{0, first}, std::pair{1, second}, std::pair{2, third}}) {
        AddStrokeCommand add{
            &notebook.page, &notebook.storage, {.ordinal = ordinal, .stroke = stroke}};
        ASSERT_TRUE(add.apply());
    }
    EraseStrokesCommand command{&notebook.page, &notebook.storage, {third.id(), first.id()}};

    ASSERT_TRUE(command.apply());
    EXPECT_EQ(idsOn(notebook.page), std::vector<Uuid>{second.id()});
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()), std::vector<Uuid>{second.id()});

    ASSERT_TRUE(command.revert());
    const std::vector<Uuid> all{first.id(), second.id(), third.id()};
    EXPECT_EQ(idsOn(notebook.page), all);
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()), all);

    ASSERT_TRUE(command.apply());
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()), std::vector<Uuid>{second.id()});
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(EraseStrokesCommandTest, ErasesNothingWhenOneStrokeIsMissing) {
    OpenNotebook notebook;
    const Stroke kept = makeStroke(notebook.ids, 10.0F);
    AddStrokeCommand add{&notebook.page, &notebook.storage, {.ordinal = 0, .stroke = kept}};
    ASSERT_TRUE(add.apply());
    EraseStrokesCommand command{
        &notebook.page, &notebook.storage, {kept.id(), notebook.ids.next()}};

    const Result<void> applied = command.apply();

    ASSERT_FALSE(applied.has_value());
    EXPECT_EQ(applied.error().code, ErrorCode::NotFound);
    EXPECT_EQ(idsOn(notebook.page), std::vector<Uuid>{kept.id()});
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()), std::vector<Uuid>{kept.id()});
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(StrokeCommandsTest, DrawingAndClearingWalkBackAndForwardThroughTheHistory) {
    OpenNotebook notebook;
    const Stroke first = makeStroke(notebook.ids, 10.0F);
    const Stroke second = makeStroke(notebook.ids, 100.0F);
    const std::vector<Uuid> both{first.id(), second.id()};
    UndoStack history;

    ASSERT_TRUE(history.run(std::make_unique<AddStrokeCommand>(
        &notebook.page, &notebook.storage, PlacedStroke{.ordinal = 0, .stroke = first})));
    ASSERT_TRUE(history.run(std::make_unique<AddStrokeCommand>(
        &notebook.page, &notebook.storage, PlacedStroke{.ordinal = 1, .stroke = second})));
    ASSERT_TRUE(history.run(std::make_unique<ClearPageCommand>(&notebook.page, &notebook.storage)));
    EXPECT_TRUE(idsOn(notebook.page).empty());

    ASSERT_TRUE(history.undo());
    EXPECT_EQ(idsOn(notebook.page), both);

    ASSERT_TRUE(history.undo());
    EXPECT_EQ(idsOn(notebook.page), std::vector<Uuid>{first.id()});
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()), std::vector<Uuid>{first.id()});

    ASSERT_TRUE(history.redo());
    EXPECT_EQ(idsOn(notebook.page), both);
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()), both);

    ASSERT_TRUE(history.redo());
    EXPECT_TRUE(idsOn(notebook.page).empty());
    EXPECT_TRUE(idsInFile(notebook, notebook.page.id()).empty());
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(StrokeCommandsTest, MovingStrokesShiftsThemAndPutsThemBackWhereTheyWere) {
    OpenNotebook notebook;
    UndoStack history;
    const Stroke first = makeStroke(notebook.ids, 10.0F);
    const Stroke second = makeStroke(notebook.ids, 100.0F);
    ASSERT_TRUE(notebook.page.insert({.ordinal = 0, .stroke = first}).has_value());
    ASSERT_TRUE(notebook.page.insert({.ordinal = 1, .stroke = second}).has_value());
    notebook.storage.insertStroke(notebook.page.id(), {.ordinal = 0, .stroke = first});
    notebook.storage.insertStroke(notebook.page.id(), {.ordinal = 1, .stroke = second});
    const float startX = first.samples().front().x;
    const std::vector<Uuid> picked{first.id()};

    ASSERT_TRUE(history
                    .run(std::make_unique<MoveStrokesCommand>(&notebook.page, &notebook.storage,
                                                              picked, 30.0F, -12.0F))
                    .has_value());

    EXPECT_EQ(idsOn(notebook.page).size(), 2U);
    const PlacedStroke& shifted = notebook.page.strokes().front();
    EXPECT_EQ(shifted.stroke.id(), first.id());
    EXPECT_FLOAT_EQ(shifted.stroke.samples().front().x, startX + 30.0F);
    EXPECT_FLOAT_EQ(shifted.stroke.samples().front().y, first.samples().front().y - 12.0F);
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()).size(), 2U);

    ASSERT_TRUE(history.undo().has_value());

    EXPECT_FLOAT_EQ(notebook.page.strokes().front().stroke.samples().front().x, startX);
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(StrokeCommandsTest, AddedStrokesGoInTogetherAndComeOutTogether) {
    OpenNotebook notebook;
    UndoStack history;
    std::vector<PlacedStroke> pasted{
        PlacedStroke{.ordinal = 0, .stroke = makeStroke(notebook.ids, 10.0F)},
        PlacedStroke{.ordinal = 1, .stroke = makeStroke(notebook.ids, 50.0F)},
    };

    ASSERT_TRUE(
        history.run(std::make_unique<AddStrokesCommand>(&notebook.page, &notebook.storage, pasted))
            .has_value());

    EXPECT_EQ(idsOn(notebook.page).size(), 2U);
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()).size(), 2U);

    ASSERT_TRUE(history.undo().has_value());

    EXPECT_TRUE(idsOn(notebook.page).empty());
    EXPECT_TRUE(idsInFile(notebook, notebook.page.id()).empty());
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(StrokeCommandsTest, AStrokeKeepsItsSamplesWhenItIsGivenAnotherColour) {
    OpenNotebook notebook;
    UndoStack history;
    const Stroke stroke = makeStroke(notebook.ids, 10.0F);
    ASSERT_TRUE(notebook.page.insert({.ordinal = 0, .stroke = stroke}).has_value());
    notebook.storage.insertStroke(notebook.page.id(), {.ordinal = 0, .stroke = stroke});
    const StrokeStyle wanted{.color = Color{.red = 200, .green = 30, .blue = 30}, .width = 6.0F};
    const std::vector<Uuid> picked{stroke.id()};

    ASSERT_TRUE(history
                    .run(std::make_unique<RestyleStrokesCommand>(&notebook.page, &notebook.storage,
                                                                 picked, wanted))
                    .has_value());

    const PlacedStroke& restyled = notebook.page.strokes().front();
    EXPECT_EQ(restyled.stroke.style(), wanted);
    EXPECT_EQ(restyled.stroke.samples().size(), stroke.samples().size());
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()).size(), 1U);

    ASSERT_TRUE(history.undo().has_value());

    EXPECT_EQ(notebook.page.strokes().front().stroke.style(), stroke.style());
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(StrokeCommandsTest, TurningWhatIsPickedCanBeUndoneExactly) {
    OpenNotebook notebook;
    UndoStack history;
    const Stroke stroke = makeStroke(notebook.ids, 10.0F);
    ASSERT_TRUE(notebook.page.insert({.ordinal = 0, .stroke = stroke}).has_value());
    notebook.storage.insertStroke(notebook.page.id(), {.ordinal = 0, .stroke = stroke});
    const Transform quarter{
        .pivot = Point{.x = 10.0F, .y = 50.0F},
        .turn = 90.0F,
    };

    ASSERT_TRUE(history
                    .run(std::make_unique<TransformStrokesCommand>(
                        &notebook.page, &notebook.storage, std::vector<Uuid>{stroke.id()}, quarter))
                    .has_value());

    const Stroke& turned = notebook.page.strokes().front().stroke;
    EXPECT_EQ(turned.id(), stroke.id());
    EXPECT_NE(turned.samples().back().x, stroke.samples().back().x);
    EXPECT_EQ(idsInFile(notebook, notebook.page.id()).size(), 1U);

    ASSERT_TRUE(history.undo().has_value());

    const Stroke& back = notebook.page.strokes().front().stroke;
    ASSERT_EQ(back.samples().size(), stroke.samples().size());
    EXPECT_FLOAT_EQ(back.samples().back().x, stroke.samples().back().x);
    EXPECT_FLOAT_EQ(back.samples().back().y, stroke.samples().back().y);
    EXPECT_EQ(back.style(), stroke.style());
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(StrokeCommandsTest, SizingWhatIsPickedIsOneChangeThatCanBeDoneAgain) {
    OpenNotebook notebook;
    UndoStack history;
    const Stroke stroke = makeStroke(notebook.ids, 10.0F);
    ASSERT_TRUE(notebook.page.insert({.ordinal = 0, .stroke = stroke}).has_value());
    notebook.storage.insertStroke(notebook.page.id(), {.ordinal = 0, .stroke = stroke});
    const Transform twice{.wide = 2.0F, .tall = 2.0F};

    ASSERT_TRUE(history
                    .run(std::make_unique<TransformStrokesCommand>(
                        &notebook.page, &notebook.storage, std::vector<Uuid>{stroke.id()}, twice))
                    .has_value());
    ASSERT_TRUE(history.undo().has_value());
    ASSERT_TRUE(history.redo().has_value());

    EXPECT_FLOAT_EQ(notebook.page.strokes().front().stroke.style().width,
                    stroke.style().width * 2.0F);
    EXPECT_TRUE(notebook.errors.empty());
}

}
}
