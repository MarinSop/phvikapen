#include "core/Error.hpp"
#include "core/id/ContentId.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/Asset.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/storage/NotebookStore.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace phvikapen::core {
namespace {

using test::makeStroke;
using test::TemporaryNotebook;

[[nodiscard]] PageInfo blankPage(const Uuid& id, std::string title) {
    PageInfo page;
    page.id = id;
    page.title = std::move(title);
    return page;
}

TEST(TrashTest, ListsWhatWasDeleted) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline initial = store->readOutline().value();
    const Uuid section = initial.sections.front().id;
    const Uuid first = initial.sections.front().pages.front().id;
    const PageInfo second = blankPage(ids.next(), "Notes");
    const std::array order{first, second.id};
    ASSERT_TRUE(store->insertPage(section, second, order).has_value());

    ASSERT_TRUE(store->trashPage(second.id).has_value());

    const Result<std::vector<TrashedItem>> items = store->trashedItems();
    ASSERT_TRUE(items.has_value()) << items.error().message;
    ASSERT_EQ(items->size(), 1U);
    EXPECT_EQ(items->front().id, second.id);
    EXPECT_EQ(items->front().sectionId, section);
    EXPECT_EQ(items->front().title, "Notes");
    EXPECT_FALSE(items->front().wholeSection);
    EXPECT_FALSE(items->front().sectionTrashed);
}

TEST(TrashTest, APageOfATrashedSectionKnowsItsSectionIsGone) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline initial = store->readOutline().value();
    const Uuid first = initial.sections.front().id;
    const Uuid second = ids.next();
    const std::array sections{first, second};
    ASSERT_TRUE(store->insertSection(second, "Physics", sections).has_value());
    const PageInfo page = blankPage(ids.next(), "Waves");
    const std::array pages{page.id};
    ASSERT_TRUE(store->insertPage(second, page, pages).has_value());

    ASSERT_TRUE(store->trashPage(page.id).has_value());
    ASSERT_TRUE(store->trashSection(second).has_value());

    const Result<std::vector<TrashedItem>> items = store->trashedItems();
    ASSERT_TRUE(items.has_value()) << items.error().message;
    ASSERT_EQ(items->size(), 2U);
    EXPECT_TRUE(items->front().wholeSection);
    EXPECT_FALSE(items->back().wholeSection);
    EXPECT_TRUE(items->back().sectionTrashed);
}

TEST(TrashTest, EmptyingTheTrashTakesThePagesAndTheirStrokes) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline initial = store->readOutline().value();
    const Uuid section = initial.sections.front().id;
    const Uuid kept = initial.sections.front().pages.front().id;
    const PageInfo gone = blankPage(ids.next(), "Scratch");
    const std::array order{kept, gone.id};
    ASSERT_TRUE(store->insertPage(section, gone, order).has_value());
    ASSERT_TRUE(store->insertStroke(kept, {.ordinal = 0, .stroke = makeStroke(ids, 0.0F)}).has_value());
    ASSERT_TRUE(store->insertStroke(gone.id, {.ordinal = 0, .stroke = makeStroke(ids, 0.0F)}).has_value());
    ASSERT_TRUE(store->trashPage(gone.id).has_value());

    ASSERT_TRUE(store->emptyTrash().has_value());

    EXPECT_TRUE(store->trashedItems()->empty());
    EXPECT_TRUE(store->strokesOfPage(gone.id)->empty());
    EXPECT_EQ(store->strokesOfPage(kept)->size(), 1U);
    EXPECT_EQ(store->readOutline()->sections.front().pages.size(), 1U);
}

TEST(TrashTest, EmptyingTheTrashTakesFilesNoPageShowsAnyMore) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline initial = store->readOutline().value();
    const Uuid section = initial.sections.front().id;
    const Uuid kept = initial.sections.front().pages.front().id;
    const Asset asset{
        .id = ContentId{ContentId::Bytes{7, 7, 7}},
        .kind = AssetKind::Pdf,
        .name = "lecture.pdf",
        .data = std::vector<std::byte>(1024, std::byte{1}),
    };
    ASSERT_TRUE(store->insertAsset(asset).has_value());
    PageInfo imported = blankPage(ids.next(), "Slide");
    imported.media = PageMedia{.asset = asset.id, .index = 0};
    const std::array order{kept, imported.id};
    ASSERT_TRUE(store->insertPage(section, imported, order).has_value());
    ASSERT_TRUE(store->trashPage(imported.id).has_value());

    ASSERT_TRUE(store->emptyTrash().has_value());

    EXPECT_FALSE(store->asset(asset.id).has_value());
}

TEST(TrashTest, AFileStillOnAPageStays) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline initial = store->readOutline().value();
    const Uuid section = initial.sections.front().id;
    const Uuid kept = initial.sections.front().pages.front().id;
    const Asset asset{
        .id = ContentId{ContentId::Bytes{9}},
        .kind = AssetKind::Image,
        .name = "photo.png",
        .data = std::vector<std::byte>(16, std::byte{2}),
    };
    ASSERT_TRUE(store->insertAsset(asset).has_value());
    ASSERT_TRUE(store->setPageMedia(kept, PageMedia{.asset = asset.id, .index = 0}).has_value());
    PageInfo other = blankPage(ids.next(), "Copy");
    other.media = PageMedia{.asset = asset.id, .index = 0};
    const std::array order{kept, other.id};
    ASSERT_TRUE(store->insertPage(section, other, order).has_value());
    ASSERT_TRUE(store->trashPage(other.id).has_value());

    ASSERT_TRUE(store->emptyTrash().has_value());

    EXPECT_TRUE(store->asset(asset.id).has_value());
}

TEST(TrashTest, APageThatWasPutBackIsInTheOutlineAgain) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline initial = store->readOutline().value();
    const Uuid section = initial.sections.front().id;
    const Uuid kept = initial.sections.front().pages.front().id;
    const PageInfo page = blankPage(ids.next(), "Back");
    const std::array order{kept, page.id};
    ASSERT_TRUE(store->insertPage(section, page, order).has_value());
    ASSERT_TRUE(store->trashPage(page.id).has_value());

    ASSERT_TRUE(store->restorePage(section, page.id, order).has_value());

    EXPECT_TRUE(store->trashedItems()->empty());
    EXPECT_EQ(store->readOutline()->sections.front().pages.size(), 2U);
}

}
}
