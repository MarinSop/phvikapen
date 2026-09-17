#include "core/storage/StorageThread.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/Stroke.hpp"
#include "core/model/Page.hpp"
#include "core/storage/NotebookStore.hpp"
#include "support/TemporaryNotebook.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

using test::makeStroke;
using test::TemporaryNotebook;

[[nodiscard]] std::vector<Uuid> idsInFile(const std::filesystem::path& path, const Uuid& page) {
    std::vector<Uuid> ids;
    const Result<NotebookStore> store = NotebookStore::open(path);
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

class ErrorLog {
public:
    [[nodiscard]] StorageThread::ErrorHandler handler() {
        return [this](const Error& error) { m_errors.push_back(error); };
    }

    [[nodiscard]] const std::vector<Error>& errors() const { return m_errors; }

private:
    std::vector<Error> m_errors;
};

TEST(StorageThreadTest, WritesReachTheNotebookFile) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    const Stroke first = makeStroke(ids, 0.0F);
    const Stroke second = makeStroke(ids, 10.0F);
    ErrorLog log;
    StorageThread storage{notebook.path(), log.handler()};

    storage.insertStroke(page, {.ordinal = 0, .stroke = first});
    storage.insertStroke(page, {.ordinal = 1, .stroke = second});
    storage.waitUntilIdle();

    EXPECT_TRUE(log.errors().empty());
    EXPECT_EQ(idsInFile(notebook.path(), page), (std::vector<Uuid>{first.id(), second.id()}));
}

TEST(StorageThreadTest, CarriesOutRequestsInTheOrderTheyWereMade) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    const Stroke removed = makeStroke(ids, 0.0F);
    const Stroke cleared = makeStroke(ids, 10.0F);
    const Stroke kept = makeStroke(ids, 20.0F);
    ErrorLog log;
    StorageThread storage{notebook.path(), log.handler()};

    storage.insertStroke(page, {.ordinal = 0, .stroke = removed});
    storage.removeStroke(page, removed.id());
    storage.insertStroke(page, {.ordinal = 0, .stroke = cleared});
    storage.removeStrokesOfPage(page);
    storage.insertStroke(page, {.ordinal = 0, .stroke = kept});
    storage.waitUntilIdle();

    EXPECT_TRUE(log.errors().empty());
    EXPECT_EQ(idsInFile(notebook.path(), page), std::vector<Uuid>{kept.id()});
}

TEST(StorageThreadTest, LoadsAPageInDrawingOrder) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    const Stroke early = makeStroke(ids, 0.0F);
    const Stroke late = makeStroke(ids, 10.0F);
    {
        Result<NotebookStore> store = NotebookStore::open(notebook.path());
        ASSERT_TRUE(store.has_value()) << store.error().message;
        ASSERT_TRUE(store->insertStroke(page, {.ordinal = 4, .stroke = late}));
        ASSERT_TRUE(store->insertStroke(page, {.ordinal = 1, .stroke = early}));
    }
    ErrorLog log;
    StorageThread storage{notebook.path(), log.handler()};
    std::optional<Result<std::vector<PlacedStroke>>> loaded;

    storage.loadPage(
        page, [&](Result<std::vector<PlacedStroke>> strokes) { loaded = std::move(strokes); });
    storage.waitUntilIdle();

    ASSERT_TRUE(loaded.has_value());
    ASSERT_TRUE(loaded->has_value()) << loaded->error().message;
    ASSERT_EQ((*loaded)->size(), 2U);
    EXPECT_EQ((*loaded)->front().ordinal, 1);
    EXPECT_EQ((*loaded)->front().stroke.id(), early.id());
    EXPECT_EQ((*loaded)->back().ordinal, 4);
    EXPECT_EQ((*loaded)->back().stroke.id(), late.id());
}

TEST(StorageThreadTest, FinishesEveryQueuedWriteBeforeItStops) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    std::vector<Uuid> written;
    ErrorLog log;
    {
        StorageThread storage{notebook.path(), log.handler()};
        for (int ordinal = 0; ordinal < 50; ++ordinal) {
            Stroke stroke = makeStroke(ids, static_cast<float>(ordinal));
            written.push_back(stroke.id());
            storage.insertStroke(page, {.ordinal = ordinal, .stroke = std::move(stroke)});
        }
    }

    EXPECT_TRUE(log.errors().empty());
    EXPECT_EQ(idsInFile(notebook.path(), page), written);
}

TEST(StorageThreadTest, ReportsANotebookItCannotOpen) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "phvikapen-missing-directory" / "notebook.phvika";
    Uuid7Generator ids;
    const Uuid page = ids.next();
    ErrorLog log;
    StorageThread storage{path, log.handler()};
    std::optional<Result<std::vector<PlacedStroke>>> loaded;

    storage.insertStroke(page, {.ordinal = 0, .stroke = makeStroke(ids, 0.0F)});
    storage.loadPage(
        page, [&](Result<std::vector<PlacedStroke>> strokes) { loaded = std::move(strokes); });
    storage.waitUntilIdle();

    ASSERT_EQ(log.errors().size(), 1U);
    EXPECT_EQ(log.errors().front().code, ErrorCode::IoFailure);
    ASSERT_TRUE(loaded.has_value());
    ASSERT_FALSE(loaded->has_value());
    EXPECT_EQ(loaded->error().code, ErrorCode::IoFailure);
}

TEST(StorageThreadTest, ReportsAWriteThatFails) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    ErrorLog log;
    StorageThread storage{notebook.path(), log.handler()};

    storage.removeStroke(page, ids.next());
    storage.waitUntilIdle();

    ASSERT_EQ(log.errors().size(), 1U);
    EXPECT_EQ(log.errors().front().code, ErrorCode::NotFound);
}

}
}
