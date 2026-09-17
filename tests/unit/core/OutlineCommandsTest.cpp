#include "core/undo/OutlineCommands.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/Outline.hpp"
#include "core/model/PageStyle.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace phvikapen::core {

namespace {

using test::TemporaryNotebook;

[[nodiscard]] NotebookOutline readFile(const TemporaryNotebook& file) {
    const Result<NotebookStore> store = NotebookStore::open(file.path());
    if (!store) {
        return {};
    }
    return store->readOutline().value_or(NotebookOutline{});
}

struct OpenNotebook {
    TemporaryNotebook file;
    Uuid7Generator ids;
    Outline outline{readFile(file)};
    std::vector<Error> errors;
    StorageThread storage{file.path(), [this](const Error& error) { errors.push_back(error); }};

    [[nodiscard]] Uuid firstSection() const { return outline.sections().front().id; }

    [[nodiscard]] Uuid firstPage() const { return outline.sections().front().pages.front().id; }

    [[nodiscard]] PageInfo newPage() {
        return PageInfo{.id = ids.next(), .title = {}, .style = {}};
    }

    [[nodiscard]] NotebookOutline inFile() {
        storage.waitUntilIdle();
        return readFile(file);
    }
};

TEST(OutlineCommandsTest, AddingAPageCanBeTakenBackAndDoneAgain) {
    OpenNotebook notebook;
    const PageInfo page = notebook.newPage();
    AddPageCommand command{&notebook.outline,
                           &notebook.storage,
                           {.sectionId = notebook.firstSection(), .index = 0},
                           page};

    ASSERT_TRUE(command.apply());
    EXPECT_EQ(notebook.outline.pageOrder(notebook.firstSection()),
              (std::vector<Uuid>{page.id, notebook.outline.sections().front().pages.back().id}));
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());

    ASSERT_TRUE(command.revert());
    EXPECT_EQ(notebook.outline.pageOrder(notebook.firstSection()).size(), 1U);
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());

    ASSERT_TRUE(command.apply());
    EXPECT_EQ(notebook.outline.page(notebook.firstPage())->id, page.id);
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());
    EXPECT_EQ(command.pageToShow(), page.id);
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(OutlineCommandsTest, DeletingAPageBringsItBackInItsPlace) {
    OpenNotebook notebook;
    const Uuid first = notebook.firstPage();
    AddPageCommand add{&notebook.outline,
                       &notebook.storage,
                       {.sectionId = notebook.firstSection(), .index = 1},
                       notebook.newPage()};
    ASSERT_TRUE(add.apply());
    const NotebookOutline before = notebook.outline.contents();
    DeletePageCommand command{&notebook.outline, &notebook.storage, first};

    ASSERT_TRUE(command.apply());
    EXPECT_FALSE(notebook.outline.placeOf(first).has_value());
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());

    ASSERT_TRUE(command.revert());
    EXPECT_EQ(notebook.outline.contents(), before);
    EXPECT_EQ(notebook.inFile(), before);
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(OutlineCommandsTest, TheLastPageOfASectionStays) {
    OpenNotebook notebook;
    DeletePageCommand command{&notebook.outline, &notebook.storage, notebook.firstPage()};

    const Result<void> applied = command.apply();

    ASSERT_FALSE(applied.has_value());
    EXPECT_EQ(applied.error().code, ErrorCode::InvalidArgument);
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());
}

TEST(OutlineCommandsTest, AMovedPageGoesBackToItsSection) {
    OpenNotebook notebook;
    const Uuid moved = notebook.firstPage();
    const SectionInfo target{
        .id = notebook.ids.next(),
        .title = "Target",
        .pages = {notebook.newPage()},
    };
    AddSectionCommand addSection{&notebook.outline, &notebook.storage, 1, target};
    AddPageCommand addPage{&notebook.outline,
                           &notebook.storage,
                           {.sectionId = notebook.firstSection(), .index = 1},
                           notebook.newPage()};
    ASSERT_TRUE(addSection.apply());
    ASSERT_TRUE(addPage.apply());
    const NotebookOutline before = notebook.outline.contents();
    MovePageCommand command{
        &notebook.outline, &notebook.storage, moved, {.sectionId = target.id, .index = 1}};

    ASSERT_TRUE(command.apply());
    EXPECT_EQ(notebook.outline.placeOf(moved), (PagePlace{.sectionId = target.id, .index = 1}));
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());

    ASSERT_TRUE(command.revert());
    EXPECT_EQ(notebook.outline.contents(), before);
    EXPECT_EQ(notebook.inFile(), before);
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(OutlineCommandsTest, RenamingAndRestylingAPageSwapBackAndForth) {
    OpenNotebook notebook;
    const Uuid page = notebook.firstPage();
    const PageStyle grid{.paper = Paper::Infinite, .background = Background::Grid};
    RenamePageCommand rename{&notebook.outline, &notebook.storage, page, "Chemistry"};
    SetPageStyleCommand restyle{&notebook.outline, &notebook.storage, page, grid};

    ASSERT_TRUE(rename.apply());
    ASSERT_TRUE(restyle.apply());
    EXPECT_EQ(notebook.outline.page(page)->title, "Chemistry");
    EXPECT_EQ(notebook.outline.page(page)->style, grid);
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());

    ASSERT_TRUE(restyle.revert());
    ASSERT_TRUE(rename.revert());
    EXPECT_EQ(notebook.outline.page(page)->title, "");
    EXPECT_EQ(notebook.outline.page(page)->style, PageStyle{});
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());

    ASSERT_TRUE(rename.apply());
    ASSERT_TRUE(restyle.apply());
    EXPECT_EQ(notebook.outline.page(page)->style, grid);
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(OutlineCommandsTest, SectionsComeAndGoWithTheirPages) {
    OpenNotebook notebook;
    const SectionInfo added{
        .id = notebook.ids.next(),
        .title = "Added",
        .pages = {notebook.newPage(), notebook.newPage()},
    };
    AddSectionCommand add{&notebook.outline, &notebook.storage, 0, added};

    ASSERT_TRUE(add.apply());
    EXPECT_EQ(notebook.outline.sections().front(), added);
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());

    ASSERT_TRUE(add.revert());
    EXPECT_EQ(notebook.outline.sections().size(), 1U);
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());

    ASSERT_TRUE(add.apply());
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());

    const NotebookOutline before = notebook.outline.contents();
    DeleteSectionCommand remove{&notebook.outline, &notebook.storage, added.id};
    ASSERT_TRUE(remove.apply());
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());
    ASSERT_TRUE(remove.revert());
    EXPECT_EQ(notebook.outline.contents(), before);
    EXPECT_EQ(notebook.inFile(), before);
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(OutlineCommandsTest, TheLastSectionStays) {
    OpenNotebook notebook;
    DeleteSectionCommand command{&notebook.outline, &notebook.storage, notebook.firstSection()};

    const Result<void> applied = command.apply();

    ASSERT_FALSE(applied.has_value());
    EXPECT_EQ(applied.error().code, ErrorCode::InvalidArgument);
}

TEST(OutlineCommandsTest, SectionsCanBeRenamedAndReordered) {
    OpenNotebook notebook;
    const Uuid first = notebook.firstSection();
    AddSectionCommand add{
        &notebook.outline,
        &notebook.storage,
        1,
        {.id = notebook.ids.next(), .title = "Second", .pages = {notebook.newPage()}}};
    ASSERT_TRUE(add.apply());
    RenameSectionCommand rename{&notebook.outline, &notebook.storage, first, "Renamed"};
    MoveSectionCommand move{&notebook.outline, &notebook.storage, first, 1};

    ASSERT_TRUE(rename.apply());
    ASSERT_TRUE(move.apply());
    EXPECT_EQ(notebook.outline.sections().back().title, "Renamed");
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());

    ASSERT_TRUE(move.revert());
    ASSERT_TRUE(rename.revert());
    EXPECT_EQ(notebook.outline.sections().front().id, first);
    EXPECT_EQ(notebook.outline.sections().front().title, "Section 1");
    EXPECT_EQ(notebook.inFile(), notebook.outline.contents());
    EXPECT_TRUE(notebook.errors.empty());
}

}
}
