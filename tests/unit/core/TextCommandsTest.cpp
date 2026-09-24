#include "core/undo/TextCommands.hpp"

#include "core/Error.hpp"
#include "core/geometry/Distance.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/TextBox.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"
#include "core/undo/BundleCommand.hpp"
#include "core/undo/UndoStack.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

using test::TemporaryNotebook;

// The page text is put on has to be one the notebook holds, since a text box belongs to a page.
[[nodiscard]] Uuid firstPageIn(const std::filesystem::path& path) {
    const Result<NotebookStore> store = NotebookStore::open(path);
    if (!store) {
        return Uuid{};
    }
    const Result<NotebookOutline> outline = store->readOutline();
    if (!outline || outline->sections.empty() || outline->sections.front().pages.empty()) {
        return Uuid{};
    }
    return outline->sections.front().pages.front().id;
}

struct OpenNotebook {
    TemporaryNotebook file;
    Uuid7Generator ids;
    Page page{firstPageIn(file.path())};
    std::vector<Error> errors;
    StorageThread storage{file.path(), [this](const Error& error) { errors.push_back(error); }};
};

[[nodiscard]] TextBox boxSaying(Uuid7Generator& ids, std::string text) {
    return TextBox{
        .id = ids.next(),
        .at = Point{.x = 12.0F, .y = 24.0F},
        .width = 180.0F,
        .height = 36.0F,
        .text = std::move(text),
        .style = TextStyle{},
    };
}

[[nodiscard]] std::vector<PlacedText> textsInFile(OpenNotebook& notebook, const Uuid& page) {
    notebook.storage.waitUntilIdle();
    const Result<NotebookStore> store = NotebookStore::open(notebook.file.path());
    if (!store) {
        return {};
    }
    const Result<std::vector<PlacedText>> texts = store->textsOfPage(page);
    return texts ? *texts : std::vector<PlacedText>{};
}

TEST(AddTextCommandTest, PutsTheTextOnThePageAndTakesItOffAgain) {
    OpenNotebook notebook;
    const TextBox box = boxSaying(notebook.ids, "Hello");
    AddTextCommand command{&notebook.page, &notebook.storage, PlacedText{.ordinal = 0, .box = box}};

    ASSERT_TRUE(command.apply());
    ASSERT_EQ(notebook.page.texts().size(), 1U);
    EXPECT_EQ(textsInFile(notebook, notebook.page.id()).size(), 1U);

    ASSERT_TRUE(command.revert());
    EXPECT_TRUE(notebook.page.texts().empty());
    EXPECT_TRUE(textsInFile(notebook, notebook.page.id()).empty());
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(RemoveTextCommandTest, TakesTheTextOffAndPutsItBackWhereItWas) {
    OpenNotebook notebook;
    const TextBox box = boxSaying(notebook.ids, "Goodbye");
    AddTextCommand added{&notebook.page, &notebook.storage, PlacedText{.ordinal = 3, .box = box}};
    ASSERT_TRUE(added.apply());

    RemoveTextCommand command{&notebook.page, &notebook.storage, box.id};
    ASSERT_TRUE(command.apply());
    EXPECT_TRUE(notebook.page.texts().empty());

    ASSERT_TRUE(command.revert());
    ASSERT_EQ(notebook.page.texts().size(), 1U);
    EXPECT_EQ(notebook.page.texts().front().ordinal, 3);
    const std::vector<PlacedText> kept = textsInFile(notebook, notebook.page.id());
    ASSERT_EQ(kept.size(), 1U);
    EXPECT_EQ(kept.front().box, box);
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(RemoveTextCommandTest, SaysSoWhenThereIsNoSuchText) {
    OpenNotebook notebook;
    RemoveTextCommand command{&notebook.page, &notebook.storage, notebook.ids.next()};

    const Result<void> applied = command.apply();

    ASSERT_FALSE(applied);
    EXPECT_EQ(applied.error().code, ErrorCode::NotFound);
}

TEST(ChangeTextCommandTest, WritesTheNewBoxAndGoesBackToTheOldOne) {
    OpenNotebook notebook;
    const TextBox before = boxSaying(notebook.ids, "draft");
    AddTextCommand added{&notebook.page, &notebook.storage,
                         PlacedText{.ordinal = 0, .box = before}};
    ASSERT_TRUE(added.apply());

    TextBox after = before;
    after.text = "the finished thing";
    after.at = Point{.x = 50.0F, .y = 80.0F};
    after.style.bold = true;
    ChangeTextCommand command{&notebook.page, &notebook.storage, after};

    ASSERT_TRUE(command.apply());
    ASSERT_EQ(notebook.page.texts().size(), 1U);
    EXPECT_EQ(notebook.page.texts().front().box, after);
    EXPECT_EQ(textsInFile(notebook, notebook.page.id()).front().box, after);

    ASSERT_TRUE(command.revert());
    EXPECT_EQ(notebook.page.texts().front().box, before);
    EXPECT_EQ(textsInFile(notebook, notebook.page.id()).front().box, before);
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(ChangeTextCommandTest, KeepsTheFirstStateEvenWhenItIsAppliedAgain) {
    OpenNotebook notebook;
    const TextBox before = boxSaying(notebook.ids, "first");
    AddTextCommand added{&notebook.page, &notebook.storage,
                         PlacedText{.ordinal = 0, .box = before}};
    ASSERT_TRUE(added.apply());
    TextBox after = before;
    after.text = "second";
    ChangeTextCommand command{&notebook.page, &notebook.storage, after};

    ASSERT_TRUE(command.apply());
    ASSERT_TRUE(command.revert());
    ASSERT_TRUE(command.apply());
    ASSERT_TRUE(command.revert());

    EXPECT_EQ(notebook.page.texts().front().box.text, "first");
}

TEST(BundleCommandTest, PutsSeveralChangesInAsOneStep) {
    OpenNotebook notebook;
    const TextBox first = boxSaying(notebook.ids, "one");
    const TextBox second = boxSaying(notebook.ids, "two");
    std::vector<std::unique_ptr<ICommand>> steps;
    steps.push_back(std::make_unique<AddTextCommand>(&notebook.page, &notebook.storage,
                                                     PlacedText{.ordinal = 0, .box = first}));
    steps.push_back(std::make_unique<AddTextCommand>(&notebook.page, &notebook.storage,
                                                     PlacedText{.ordinal = 1, .box = second}));
    BundleCommand command{std::move(steps)};

    ASSERT_TRUE(command.apply());
    EXPECT_EQ(notebook.page.texts().size(), 2U);

    ASSERT_TRUE(command.revert());
    EXPECT_TRUE(notebook.page.texts().empty());
    EXPECT_EQ(command.pageToShow(), notebook.page.id());
}

TEST(BundleCommandTest, LeavesNothingBehindWhenAStepWillNotGoIn) {
    OpenNotebook notebook;
    const TextBox box = boxSaying(notebook.ids, "one");
    std::vector<std::unique_ptr<ICommand>> steps;
    steps.push_back(std::make_unique<AddTextCommand>(&notebook.page, &notebook.storage,
                                                     PlacedText{.ordinal = 0, .box = box}));
    steps.push_back(std::make_unique<RemoveTextCommand>(&notebook.page, &notebook.storage,
                                                        notebook.ids.next()));
    BundleCommand command{std::move(steps)};

    const Result<void> applied = command.apply();

    ASSERT_FALSE(applied);
    EXPECT_EQ(applied.error().code, ErrorCode::NotFound);
    EXPECT_TRUE(notebook.page.texts().empty());
}

TEST(BundleCommandTest, GoesThroughTheUndoStackAsOneStep) {
    OpenNotebook notebook;
    UndoStack history;
    const TextBox box = boxSaying(notebook.ids, "bundled");
    std::vector<std::unique_ptr<ICommand>> steps;
    steps.push_back(std::make_unique<AddTextCommand>(&notebook.page, &notebook.storage,
                                                     PlacedText{.ordinal = 0, .box = box}));
    TextBox after = box;
    after.text = "bundled and changed";
    steps.push_back(std::make_unique<ChangeTextCommand>(&notebook.page, &notebook.storage, after));

    ASSERT_TRUE(history.run(std::make_unique<BundleCommand>(std::move(steps))));
    EXPECT_EQ(notebook.page.texts().front().box.text, "bundled and changed");
    EXPECT_EQ(history.undoCount(), 1U);

    ASSERT_TRUE(history.undo());
    EXPECT_TRUE(notebook.page.texts().empty());

    ASSERT_TRUE(history.redo());
    EXPECT_EQ(notebook.page.texts().front().box.text, "bundled and changed");
}

}
}
