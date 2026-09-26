#include "core/Error.hpp"
#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/ContentId.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"
#include "core/model/Picture.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"
#include "core/text/InkWord.hpp"
#include "core/undo/PictureCommands.hpp"
#include "core/undo/UndoStack.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

using test::TemporaryNotebook;

[[nodiscard]] Picture pictureAt(Uuid7Generator& ids, float x, float y) {
    return Picture{
        .id = ids.next(),
        .source = ContentId{ContentId::Bytes{4, 2}},
        .at = Point{.x = x, .y = y},
        .width = 120.0F,
        .height = 90.0F,
        .turn = 12.5F,
    };
}

[[nodiscard]] Uuid firstPageOf(const NotebookStore& store) {
    return store.readOutline().value().sections.front().pages.front().id;
}

TEST(NotebookPicturesTest, KeepsAPictureThatWasPutOnAPage) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    const Picture picture = pictureAt(ids, 10.0F, 20.0F);

    ASSERT_TRUE(store->insertPicture(page, PlacedPicture{.ordinal = 0, .picture = picture}));

    const Result<std::vector<PlacedPicture>> kept = store->picturesOfPage(page);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    ASSERT_EQ(kept->size(), 1U);
    EXPECT_EQ(kept->front().ordinal, 0);
    EXPECT_EQ(kept->front().picture, picture);
}

TEST(NotebookPicturesTest, ListsPicturesInTheOrderTheyWerePutDown) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);

    ASSERT_TRUE(store->insertPicture(page, {.ordinal = 1, .picture = pictureAt(ids, 90.0F, 0.0F)}));
    ASSERT_TRUE(store->insertPicture(page, {.ordinal = 0, .picture = pictureAt(ids, 10.0F, 0.0F)}));

    const Result<std::vector<PlacedPicture>> kept = store->picturesOfPage(page);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    ASSERT_EQ(kept->size(), 2U);
    EXPECT_FLOAT_EQ(kept->front().picture.at.x, 10.0F);
    EXPECT_FLOAT_EQ(kept->back().picture.at.x, 90.0F);
}

TEST(NotebookPicturesTest, WritesOverAPictureThatIsAlreadyThere) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    Picture picture = pictureAt(ids, 10.0F, 20.0F);
    ASSERT_TRUE(store->insertPicture(page, PlacedPicture{.ordinal = 0, .picture = picture}));

    picture.at = Point{.x = 40.0F, .y = 60.0F};
    picture.width = 300.0F;
    picture.turn = -90.0F;
    ASSERT_TRUE(store->updatePicture(page, picture));

    const Result<std::vector<PlacedPicture>> kept = store->picturesOfPage(page);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    ASSERT_EQ(kept->size(), 1U);
    EXPECT_EQ(kept->front().picture, picture);
}

TEST(NotebookPicturesTest, TakesAPictureOffAPage) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    const Picture picture = pictureAt(ids, 10.0F, 20.0F);
    ASSERT_TRUE(store->insertPicture(page, PlacedPicture{.ordinal = 0, .picture = picture}));

    ASSERT_TRUE(store->removePicture(page, picture.id));

    EXPECT_TRUE(store->picturesOfPage(page).value().empty());
}

TEST(NotebookPicturesTest, ClearsAllThePicturesOfAPage) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    ASSERT_TRUE(store->insertPicture(page, {.ordinal = 0, .picture = pictureAt(ids, 0.0F, 0.0F)}));
    ASSERT_TRUE(store->insertPicture(page, {.ordinal = 1, .picture = pictureAt(ids, 5.0F, 5.0F)}));

    const Result<std::size_t> gone = store->removePicturesOfPage(page);

    ASSERT_TRUE(gone.has_value()) << gone.error().message;
    EXPECT_EQ(*gone, 2U);
    EXPECT_TRUE(store->picturesOfPage(page).value().empty());
}

TEST(NotebookPicturesTest, EmptyingTheTrashTakesThePicturesWithIt) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    ASSERT_TRUE(store->insertPicture(page, {.ordinal = 0, .picture = pictureAt(ids, 0.0F, 0.0F)}));
    ASSERT_TRUE(store->trashPage(page));

    ASSERT_TRUE(store->emptyTrash());

    EXPECT_TRUE(store->picturesOfPage(page).value().empty());
}

TEST(NotebookPicturesTest, OpensANotebookAtTheVersionThatHoldsPictures) {
    const TemporaryNotebook notebook;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;

    EXPECT_EQ(store->schemaVersion().value(), kNotebookSchemaVersion);
    EXPECT_GE(kNotebookSchemaVersion, 8);
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

    [[nodiscard]] std::vector<PlacedPicture> inFile(const Uuid& page) {
        storage.waitUntilIdle();
        const Result<NotebookStore> store = NotebookStore::open(file.path());
        return store->picturesOfPage(page).value_or(std::vector<PlacedPicture>{});
    }
};

TEST(PictureCommandsTest, PutsAPictureOnThePageAndTakesItOffAgain) {
    OpenNotebook notebook;
    UndoStack history;
    Page page{notebook.firstPage()};
    const Picture picture = pictureAt(notebook.ids, 10.0F, 20.0F);

    ASSERT_TRUE(history
                    .run(std::make_unique<AddPictureCommand>(
                        &page, &notebook.storage, PlacedPicture{.ordinal = 0, .picture = picture}))
                    .has_value());

    EXPECT_EQ(page.pictures().size(), 1U);
    EXPECT_EQ(notebook.inFile(page.id()).size(), 1U);

    ASSERT_TRUE(history.undo().has_value());

    EXPECT_TRUE(page.pictures().empty());
    EXPECT_TRUE(notebook.inFile(page.id()).empty());
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(PictureCommandsTest, TakesAPictureAwayAndBringsItBack) {
    OpenNotebook notebook;
    UndoStack history;
    Page page{notebook.firstPage()};
    const Picture picture = pictureAt(notebook.ids, 10.0F, 20.0F);
    ASSERT_TRUE(history
                    .run(std::make_unique<AddPictureCommand>(
                        &page, &notebook.storage, PlacedPicture{.ordinal = 0, .picture = picture}))
                    .has_value());

    ASSERT_TRUE(
        history.run(std::make_unique<RemovePictureCommand>(&page, &notebook.storage, picture.id))
            .has_value());

    EXPECT_TRUE(page.pictures().empty());

    ASSERT_TRUE(history.undo().has_value());

    ASSERT_EQ(page.pictures().size(), 1U);
    EXPECT_EQ(page.pictures().front().picture, picture);
    EXPECT_EQ(notebook.inFile(page.id()).size(), 1U);
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(PictureCommandsTest, MovingAndTurningAPictureCanBeUndoneExactly) {
    OpenNotebook notebook;
    UndoStack history;
    Page page{notebook.firstPage()};
    const Picture picture = pictureAt(notebook.ids, 10.0F, 20.0F);
    ASSERT_TRUE(history
                    .run(std::make_unique<AddPictureCommand>(
                        &page, &notebook.storage, PlacedPicture{.ordinal = 0, .picture = picture}))
                    .has_value());

    Picture moved = picture;
    moved.at = Point{.x = 80.0F, .y = 90.0F};
    moved.turn = 90.0F;
    ASSERT_TRUE(history.run(std::make_unique<ChangePictureCommand>(&page, &notebook.storage, moved))
                    .has_value());

    EXPECT_EQ(page.pictures().front().picture, moved);
    EXPECT_EQ(notebook.inFile(page.id()).front().picture, moved);

    ASSERT_TRUE(history.undo().has_value());

    EXPECT_EQ(page.pictures().front().picture, picture);

    ASSERT_TRUE(history.redo().has_value());

    EXPECT_EQ(page.pictures().front().picture, moved);
    EXPECT_TRUE(notebook.errors.empty());
}

TEST(NotebookPicturesTest, KeepsTheWordsReadOutOfAPicture) {
    const TemporaryNotebook notebook;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const ContentId source{ContentId::Bytes{7, 7}};
    const std::vector<PictureWord> read{
        PictureWord{
            .text = "Budget",
            .box =
                Rect{
                    .left = 0.1F,
                    .top = 0.2F,
                    .right = 0.4F,
                    .bottom = 0.3F,
                },
        },
        PictureWord{
            .text = "Friday",
            .box =
                Rect{
                    .left = 0.1F,
                    .top = 0.5F,
                    .right = 0.5F,
                    .bottom = 0.6F,
                },
        },
    };

    ASSERT_TRUE(store->writePictureWords(source, read));

    const Result<std::vector<PictureWord>> kept = store->pictureWords(source);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    ASSERT_EQ(kept->size(), 2U);
    EXPECT_EQ(kept->front().text, "Budget");
    EXPECT_FLOAT_EQ(kept->front().box.left, 0.1F);
    EXPECT_FLOAT_EQ(kept->front().box.bottom, 0.3F);
    EXPECT_EQ(kept->back().text, "Friday");
}

TEST(NotebookPicturesTest, ReadingAPictureAgainPutsTheOldWordsOut) {
    const TemporaryNotebook notebook;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const ContentId source{ContentId::Bytes{7, 7}};
    const std::array<PictureWord, 1> first{PictureWord{.text = "Before", .box = Rect{}}};
    const std::array<PictureWord, 1> second{PictureWord{.text = "After", .box = Rect{}}};
    ASSERT_TRUE(store->writePictureWords(source, first));

    ASSERT_TRUE(store->writePictureWords(source, second));

    const Result<std::vector<PictureWord>> kept = store->pictureWords(source);
    ASSERT_TRUE(kept.has_value()) << kept.error().message;
    ASSERT_EQ(kept->size(), 1U);
    EXPECT_EQ(kept->front().text, "After");

    ASSERT_TRUE(store->forgetPictureWords(source));
    EXPECT_TRUE(store->pictureWords(source).value().empty());
}

TEST(NotebookPicturesTest, SearchingANotebookReachesTheWordsInAPicture) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Uuid page = firstPageOf(*store);
    Picture standing = pictureAt(ids, 100.0F, 200.0F);
    standing.turn = 0.0F;
    ASSERT_TRUE(store->insertPicture(page, PlacedPicture{.ordinal = 0, .picture = standing}));
    const std::array<PictureWord, 1> read{
        PictureWord{
            .text = "Reykjavik",
            .box = Rect{.left = 0.5F, .top = 0.5F, .right = 1.0F, .bottom = 1.0F},
        },
    };
    ASSERT_TRUE(store->writePictureWords(standing.source, read));

    const Result<std::vector<FoundWord>> hits = store->findWords("reykjavik");

    ASSERT_TRUE(hits.has_value()) << hits.error().message;
    ASSERT_EQ(hits->size(), 1U);
    EXPECT_EQ(hits->front().pageId, page);
    EXPECT_EQ(hits->front().word.text, "Reykjavik");
    // Half way across a picture 120 wide standing 100 from the left edge of the page.
    EXPECT_FLOAT_EQ(hits->front().word.box.left, 160.0F);
    EXPECT_FLOAT_EQ(hits->front().word.box.top, 245.0F);
    EXPECT_FLOAT_EQ(hits->front().word.box.right, 220.0F);
    EXPECT_FLOAT_EQ(hits->front().word.box.bottom, 290.0F);
}

TEST(NotebookPicturesTest, TheSamePictureOnTwoPagesIsFoundOnBoth) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    const Result<NotebookOutline> outline = store->readOutline();
    ASSERT_TRUE(outline.has_value()) << outline.error().message;
    const Uuid section = outline->sections.front().id;
    const Uuid first = firstPageOf(*store);
    const PageInfo another{
        .id = ids.next(),
        .title = "Another",
        .style = PageStyle{},
        .media = std::nullopt,
    };
    const std::array order{first, another.id};
    ASSERT_TRUE(store->insertPage(section, another, order));
    const Picture standing = pictureAt(ids, 0.0F, 0.0F);
    Picture again = standing;
    again.id = ids.next();
    ASSERT_TRUE(store->insertPicture(first, PlacedPicture{.ordinal = 0, .picture = standing}));
    ASSERT_TRUE(store->insertPicture(another.id, PlacedPicture{.ordinal = 0, .picture = again}));
    const std::array<PictureWord, 1> read{PictureWord{.text = "Twice", .box = Rect{}}};
    ASSERT_TRUE(store->writePictureWords(standing.source, read));

    const Result<std::vector<FoundWord>> hits = store->findWords("twice");

    ASSERT_TRUE(hits.has_value()) << hits.error().message;
    EXPECT_EQ(hits->size(), 2U);
}

}
}
