#include "core/storage/NotebookStore.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid7Generator.hpp"
#include "core/ink/Stroke.hpp"

#include <gtest/gtest.h>
#include <sqlite3.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

namespace phvikapen::core {
namespace {

using std::chrono::microseconds;

/// A notebook file in the temporary directory, removed again when the test ends.
class TemporaryNotebook {
public:
    TemporaryNotebook() {
        Uuid7Generator ids;
        m_path = std::filesystem::temp_directory_path()
                 / ("phvikapen-" + ids.next().toString() + ".phvika");
        // The write-ahead log leaves two files of its own next to the notebook. Their names are
        // built here, where allocating may throw, so that the destructor cannot.
        m_walPath = m_path.string() + "-wal";
        m_shmPath = m_path.string() + "-shm";
    }

    ~TemporaryNotebook() {
        std::error_code ignored;
        std::filesystem::remove(m_path, ignored);
        std::filesystem::remove(m_walPath, ignored);
        std::filesystem::remove(m_shmPath, ignored);
    }

    TemporaryNotebook(const TemporaryNotebook&) = delete;
    TemporaryNotebook& operator=(const TemporaryNotebook&) = delete;
    TemporaryNotebook(TemporaryNotebook&&) = delete;
    TemporaryNotebook& operator=(TemporaryNotebook&&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_path;
    std::filesystem::path m_walPath;
    std::filesystem::path m_shmPath;
};

[[nodiscard]] Stroke makeStroke(Uuid7Generator& ids, float originX) {
    Stroke stroke{ids.next(), StrokeStyle{.width = 3.0F}};
    for (int i = 0; i < 20; ++i) {
        const auto step = static_cast<float>(i);
        stroke.append(InkSample{
            .x = originX + step,
            .y = 50.0F + step,
            .pressure = 0.7F,
            .timestamp = microseconds{8'000 * i},
        });
    }
    return stroke;
}

TEST(NotebookStoreTest, CreatesAFileWithTheCurrentSchema) {
    const TemporaryNotebook notebook;

    const Result<NotebookStore> store = NotebookStore::open(notebook.path());

    ASSERT_TRUE(store.has_value()) << store.error().message;
    EXPECT_TRUE(std::filesystem::exists(notebook.path()));
    const Result<int> version = store->schemaVersion();
    ASSERT_TRUE(version.has_value()) << version.error().message;
    EXPECT_EQ(*version, kNotebookSchemaVersion);
}

TEST(NotebookStoreTest, StrokesSurviveClosingAndReopening) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid page = ids.next();
    const Stroke first = makeStroke(ids, 10.0F);
    const Stroke second = makeStroke(ids, 100.0F);

    {
        Result<NotebookStore> store = NotebookStore::open(notebook.path());
        ASSERT_TRUE(store.has_value()) << store.error().message;
        ASSERT_TRUE(store->appendStroke(page, first).has_value());
        ASSERT_TRUE(store->appendStroke(page, second).has_value());
    }

    const Result<NotebookStore> reopened = NotebookStore::open(notebook.path());
    ASSERT_TRUE(reopened.has_value()) << reopened.error().message;
    const Result<std::vector<Stroke>> strokes = reopened->strokesOfPage(page);
    ASSERT_TRUE(strokes.has_value()) << strokes.error().message;

    ASSERT_EQ(strokes->size(), 2U);
    // The order the strokes were drawn in is the order they come back in.
    EXPECT_EQ(strokes->front().id(), first.id());
    EXPECT_EQ(strokes->back().id(), second.id());
    EXPECT_EQ(strokes->front().samples().size(), first.samples().size());
}

TEST(NotebookStoreTest, KeepsPagesApart) {
    const TemporaryNotebook notebook;
    Uuid7Generator ids;
    const Uuid firstPage = ids.next();
    const Uuid secondPage = ids.next();

    Result<NotebookStore> store = NotebookStore::open(notebook.path());
    ASSERT_TRUE(store.has_value()) << store.error().message;
    ASSERT_TRUE(store->appendStroke(firstPage, makeStroke(ids, 0.0F)).has_value());
    ASSERT_TRUE(store->appendStroke(secondPage, makeStroke(ids, 0.0F)).has_value());
    ASSERT_TRUE(store->appendStroke(secondPage, makeStroke(ids, 0.0F)).has_value());

    EXPECT_EQ(store->strokesOfPage(firstPage)->size(), 1U);
    EXPECT_EQ(store->strokesOfPage(secondPage)->size(), 2U);
    EXPECT_TRUE(store->strokesOfPage(ids.next())->empty());
}

TEST(NotebookStoreTest, RefusesANotebookFromANewerVersion) {
    const TemporaryNotebook notebook;
    {
        const Result<NotebookStore> store = NotebookStore::open(notebook.path());
        ASSERT_TRUE(store.has_value()) << store.error().message;
    }

    // Pretend a later version of the application wrote this file.
    const std::filesystem::path& path = notebook.path();
    {
        sqlite3* raw = nullptr;
        ASSERT_EQ(sqlite3_open(path.string().c_str(), &raw), SQLITE_OK);
        ASSERT_EQ(sqlite3_exec(
                      raw,
                      ("PRAGMA user_version = " + std::to_string(kNotebookSchemaVersion + 1) + ";")
                          .c_str(),
                      nullptr, nullptr, nullptr),
                  SQLITE_OK);
        sqlite3_close(raw);
    }

    const Result<NotebookStore> reopened = NotebookStore::open(path);

    ASSERT_FALSE(reopened.has_value());
    EXPECT_EQ(reopened.error().code, ErrorCode::Unsupported);
}

TEST(NotebookStoreTest, ReportsAPathItCannotOpen) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "phvikapen-missing-directory" / "notebook.phvika";

    const Result<NotebookStore> store = NotebookStore::open(path);

    ASSERT_FALSE(store.has_value());
    EXPECT_EQ(store.error().code, ErrorCode::IoFailure);
}

} // namespace
} // namespace phvikapen::core
