#include "core/Error.hpp"
#include "core/geometry/Distance.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/Outline.hpp"
#include "core/model/TextBox.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/text/InkWord.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace phvikapen::core {
namespace {

using test::TemporaryNotebook;

[[nodiscard]] TextBox boxSaying(Uuid7Generator& ids, std::string text, float top = 20.0F) {
    return TextBox{
        .id = ids.next(),
        .at = Point{.x = 10.0F, .y = top},
        .width = 200.0F,
        .height = 40.0F,
        .text = std::move(text),
        .style = TextStyle{},
    };
}

[[nodiscard]] Uuid firstPageOf(const NotebookStore& store) {
    return store.readOutline().value().sections.front().pages.front().id;
}

TEST(NotebookTextsTest, KeepsWhatWasTypedOnAPage) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    TextBox box = boxSaying(ids, "Hello there");
    box.style.font = "Georgia";
    box.style.size = 18.0F;
    box.style.color = Color{.red = 200, .green = 10, .blue = 30, .alpha = Color::kOpaque};
    box.style.align = TextAlign::Center;
    box.style.lineHeight = 1.5F;
    box.style.bold = true;
    box.style.struckOut = true;

    ASSERT_TRUE(store->insertText(page, PlacedText{.ordinal = 0, .box = box}));

    const Result<std::vector<PlacedText>> kept = store->textsOfPage(page);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    ASSERT_EQ(kept->size(), 1U);
    EXPECT_EQ(kept->front().ordinal, 0);
    EXPECT_EQ(kept->front().box, box);
}

TEST(NotebookTextsTest, ListsTextInTheOrderItWasPut) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);

    ASSERT_TRUE(store->insertText(page, {.ordinal = 1, .box = boxSaying(ids, "second")}));
    ASSERT_TRUE(store->insertText(page, {.ordinal = 0, .box = boxSaying(ids, "first")}));

    const Result<std::vector<PlacedText>> kept = store->textsOfPage(page);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    ASSERT_EQ(kept->size(), 2U);
    EXPECT_EQ(kept->front().box.text, "first");
    EXPECT_EQ(kept->back().box.text, "second");
}

TEST(NotebookTextsTest, WritesOverTextThatIsAlreadyThere) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    TextBox box = boxSaying(ids, "draft");
    ASSERT_TRUE(store->insertText(page, PlacedText{.ordinal = 0, .box = box}));

    box.text = "the finished thing";
    box.at = Point{.x = 40.0F, .y = 60.0F};
    box.style.italic = true;
    ASSERT_TRUE(store->updateText(page, box));

    const Result<std::vector<PlacedText>> kept = store->textsOfPage(page);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    ASSERT_EQ(kept->size(), 1U);
    EXPECT_EQ(kept->front().box, box);
}

TEST(NotebookTextsTest, TakesTextOffAPage) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    const TextBox box = boxSaying(ids, "gone soon");
    ASSERT_TRUE(store->insertText(page, PlacedText{.ordinal = 0, .box = box}));

    ASSERT_TRUE(store->removeText(page, box.id));

    EXPECT_TRUE(store->textsOfPage(page).value().empty());
}

TEST(NotebookTextsTest, ClearsAllTheTextOfAPage) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    ASSERT_TRUE(store->insertText(page, {.ordinal = 0, .box = boxSaying(ids, "one")}));
    ASSERT_TRUE(store->insertText(page, {.ordinal = 1, .box = boxSaying(ids, "two")}));

    const Result<std::size_t> gone = store->removeTextsOfPage(page);

    ASSERT_TRUE(gone.has_value()) << gone.error().message;
    EXPECT_EQ(*gone, 2U);
    EXPECT_TRUE(store->textsOfPage(page).value().empty());
}

TEST(NotebookTextsTest, FindsTypedTextWhateverTheAccents) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    ASSERT_TRUE(
        store->insertText(page, {.ordinal = 0, .box = boxSaying(ids, "Putška u Čehoslovačkoj")}));

    const Result<std::vector<FoundWord>> found = store->findWords("cehoslovackoj");

    ASSERT_TRUE(found.has_value()) << found.error().message;
    ASSERT_EQ(found->size(), 1U);
    EXPECT_EQ(found->front().pageId, page);
    EXPECT_EQ(found->front().word.text, "Putška u Čehoslovačkoj");
    EXPECT_TRUE(found->front().word.strokes.empty());
    EXPECT_FLOAT_EQ(found->front().word.box.left, 10.0F);
    EXPECT_FLOAT_EQ(found->front().word.box.right, 210.0F);
}

TEST(NotebookTextsTest, FindsAPhraseOnlyWhereTheWordsFollowOneAnother) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    ASSERT_TRUE(
        store->insertText(page, {.ordinal = 0, .box = boxSaying(ids, "the quick brown fox")}));

    EXPECT_EQ(store->findWords("quick brown").value().size(), 1U);
    EXPECT_TRUE(store->findWords("brown quick").value().empty());
}

TEST(NotebookTextsTest, LeavesTypedTextOutOfTheSearchOnceThePageIsInTheTrash) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    ASSERT_TRUE(store->insertText(page, {.ordinal = 0, .box = boxSaying(ids, "hidden away")}));
    ASSERT_EQ(store->findWords("hidden").value().size(), 1U);

    ASSERT_TRUE(store->trashPage(page));

    EXPECT_TRUE(store->findWords("hidden").value().empty());
}

TEST(NotebookTextsTest, EmptyingTheTrashTakesTheTextWithIt) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    ASSERT_TRUE(store->insertText(page, {.ordinal = 0, .box = boxSaying(ids, "for the bin")}));
    ASSERT_TRUE(store->trashPage(page));

    ASSERT_TRUE(store->emptyTrash());

    EXPECT_TRUE(store->textsOfPage(page).value().empty());
}

TEST(NotebookTextsTest, OpensANotebookAtTheVersionThatHoldsTypedText) {
    const TemporaryNotebook notebook;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;

    EXPECT_EQ(store->schemaVersion().value(), kNotebookSchemaVersion);
    EXPECT_EQ(kNotebookSchemaVersion, 7);
}

}
}
