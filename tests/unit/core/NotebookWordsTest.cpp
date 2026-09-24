#include "core/Error.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/text/InkWord.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace phvikapen::core {
namespace {

using test::makeStroke;
using test::TemporaryNotebook;

[[nodiscard]] InkWord wordAt(std::string text, float left, const std::vector<Uuid>& strokes = {}) {
    return InkWord{
        .text = std::move(text),
        .box = Rect{.left = left, .top = 10.0F, .right = left + 30.0F, .bottom = 30.0F},
        .strokes = strokes,
    };
}

TEST(NotebookWordsTest, APageWithoutInkIsNotWaitingToBeRead) {
    const TemporaryNotebook notebook;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline outline = store->readOutline().value();
    const Uuid page = outline.sections.front().pages.front().id;

    const Result<std::vector<Uuid>> waiting = store->pagesWaitingToBeRead();

    ASSERT_TRUE(waiting.has_value()) << waiting.error().message;
    EXPECT_TRUE(waiting->empty());
    EXPECT_EQ(store->inkRevisionOfPage(page), 0);
}

TEST(NotebookWordsTest, WritingOnAPagePutsItInLineToBeRead) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline outline = store->readOutline().value();
    const Uuid page = outline.sections.front().pages.front().id;

    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 0, .stroke = makeStroke(ids, 0.0F)}));

    const Result<std::vector<Uuid>> waiting = store->pagesWaitingToBeRead();
    ASSERT_TRUE(waiting.has_value()) << waiting.error().message;
    ASSERT_EQ(waiting->size(), 1U);
    EXPECT_EQ(waiting->front(), page);
    EXPECT_EQ(store->inkRevisionOfPage(page), 1);
}

TEST(NotebookWordsTest, APageReadAtItsCurrentInkWaitsNoLonger) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline outline = store->readOutline().value();
    const Uuid page = outline.sections.front().pages.front().id;
    const Stroke stroke = makeStroke(ids, 0.0F);
    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 0, .stroke = stroke}));
    const std::int64_t revision = store->inkRevisionOfPage(page).value();

    ASSERT_TRUE(store->setWordsOfPage(
        page, revision, std::vector<InkWord>{wordAt("Ekotoksikologija", 20.0F, {stroke.id()})}));

    EXPECT_TRUE(store->pagesWaitingToBeRead().value().empty());
    const Result<std::vector<InkWord>> words = store->wordsOfPage(page);
    ASSERT_TRUE(words.has_value()) << words.error().message;
    ASSERT_EQ(words->size(), 1U);
    EXPECT_EQ(words->front().text, "Ekotoksikologija");
    EXPECT_FLOAT_EQ(words->front().box.left, 20.0F);
    ASSERT_EQ(words->front().strokes.size(), 1U);
    EXPECT_EQ(words->front().strokes.front(), stroke.id());
}

TEST(NotebookWordsTest, WritingMoreOnAPagePutsItInLineAgain) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline outline = store->readOutline().value();
    const Uuid page = outline.sections.front().pages.front().id;
    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 0, .stroke = makeStroke(ids, 0.0F)}));
    ASSERT_TRUE(store->setWordsOfPage(page, store->inkRevisionOfPage(page).value(),
                                      std::vector<InkWord>{wordAt("first", 0.0F)}));

    ASSERT_TRUE(store->insertStroke(page, {.ordinal = 1, .stroke = makeStroke(ids, 40.0F)}));

    const Result<std::vector<Uuid>> waiting = store->pagesWaitingToBeRead();
    ASSERT_TRUE(waiting.has_value()) << waiting.error().message;
    ASSERT_EQ(waiting->size(), 1U);
    EXPECT_EQ(waiting->front(), page);
}

TEST(NotebookWordsTest, FindsAWordHoweverItWasTyped) {
    const TemporaryNotebook notebook;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline outline = store->readOutline().value();
    const Uuid page = outline.sections.front().pages.front().id;
    ASSERT_TRUE(store->setWordsOfPage(page, 0,
                                      std::vector<InkWord>{
                                          wordAt("Čehoslovačka", 10.0F),
                                          wordAt("je", 60.0F),
                                          wordAt("povijest", 90.0F),
                                      }));

    for (const std::string& typed : {"cehoslovacka", "ČEHOSLOVAČKA", "slova", "  cehoslovacka "}) {
        const Result<std::vector<FoundWord>> found = store->findWords(typed);
        ASSERT_TRUE(found.has_value()) << found.error().message;
        ASSERT_EQ(found->size(), 1U) << typed;
        EXPECT_EQ(found->front().pageId, page);
        EXPECT_EQ(found->front().word.text, "Čehoslovačka");
    }
    EXPECT_TRUE(store->findWords("biologija").value().empty());
    EXPECT_TRUE(store->findWords("   ").value().empty());
}

TEST(NotebookWordsTest, FindsSeveralWordsOnlyWhereTheyFollowEachOther) {
    const TemporaryNotebook notebook;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline outline = store->readOutline().value();
    const Uuid page = outline.sections.front().pages.front().id;
    ASSERT_TRUE(store->setWordsOfPage(page, 0,
                                      std::vector<InkWord>{
                                          wordAt("što", 10.0F),
                                          wordAt("je", 40.0F),
                                          wordAt("ekotoksikologija", 70.0F),
                                          wordAt("je", 200.0F),
                                          wordAt("znanost", 230.0F),
                                      }));

    const Result<std::vector<FoundWord>> together = store->findWords("je ekotoksikologija");
    ASSERT_TRUE(together.has_value()) << together.error().message;
    ASSERT_EQ(together->size(), 1U);
    EXPECT_FLOAT_EQ(together->front().word.box.left, 40.0F);

    EXPECT_TRUE(store->findWords("je povijest").value().empty());
    EXPECT_EQ(store->findWords("je").value().size(), 2U);
}

TEST(NotebookWordsTest, WordsComeBackInTheOrderThePagesAreRead) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline outline = store->readOutline().value();
    const Uuid section = outline.sections.front().id;
    const Uuid first = outline.sections.front().pages.front().id;
    PageInfo second;
    second.id = ids.next();
    second.title = "Second";
    ASSERT_TRUE(store->insertPage(section, second, std::vector<Uuid>{first, second.id}));
    ASSERT_TRUE(
        store->setWordsOfPage(second.id, 0, std::vector<InkWord>{wordAt("bilješka", 0.0F)}));
    ASSERT_TRUE(store->setWordsOfPage(first, 0, std::vector<InkWord>{wordAt("bilješka", 0.0F)}));

    const Result<std::vector<FoundWord>> found = store->findWords("biljeska");

    ASSERT_TRUE(found.has_value()) << found.error().message;
    ASSERT_EQ(found->size(), 2U);
    EXPECT_EQ(found->front().pageId, first);
    EXPECT_EQ(found->back().pageId, second.id);
}

TEST(NotebookWordsTest, WordsOfAPageInTheTrashAreNotFound) {
    const TemporaryNotebook notebook;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const NotebookOutline outline = store->readOutline().value();
    const Uuid page = outline.sections.front().pages.front().id;
    ASSERT_TRUE(store->setWordsOfPage(page, 0, std::vector<InkWord>{wordAt("bilješka", 0.0F)}));

    ASSERT_TRUE(store->trashPage(page));

    EXPECT_TRUE(store->findWords("biljeska").value().empty());
    ASSERT_TRUE(store->emptyTrash());
    EXPECT_TRUE(store->wordsOfPage(page).value().empty());
}

}
}
