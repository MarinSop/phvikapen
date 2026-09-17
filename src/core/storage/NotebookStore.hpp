#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/Stroke.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

struct sqlite3;

namespace phvikapen::core {

/// Schema version this build writes and understands.
inline constexpr int kNotebookSchemaVersion = 1;

/// A notebook file: one SQLite database holding the strokes of every page.
///
/// The file is opened in write-ahead logging mode, so that reading a page never blocks the write
/// of a finished stroke. Every stroke is written in its own transaction, which is what keeps a
/// crash from costing more than the stroke in progress.
///
/// This class lives in the core layer although it touches the file system, because nothing in it
/// is specific to an operating system: it is the storage half of the document model.
///
/// One store belongs to one thread. The application hands strokes to it from a storage thread.
class NotebookStore {
public:
    /// Opens the notebook at @p path, creating it when it does not exist yet, and brings its
    /// schema up to date.
    ///
    /// Fails with ErrorCode::IoFailure when the file cannot be opened or migrated, and with
    /// ErrorCode::Unsupported when it was written by a newer version of the schema.
    [[nodiscard]] static Result<NotebookStore> open(const std::filesystem::path& path);

    ~NotebookStore();

    NotebookStore(NotebookStore&& other) noexcept;
    NotebookStore& operator=(NotebookStore&& other) noexcept;

    NotebookStore(const NotebookStore&) = delete;
    NotebookStore& operator=(const NotebookStore&) = delete;

    /// Appends @p stroke to the page @p pageId, after the strokes already stored there.
    [[nodiscard]] Result<void> appendStroke(const Uuid& pageId, const Stroke& stroke);

    /// Removes the stroke @p strokeId from the page @p pageId.
    ///
    /// Fails with ErrorCode::NotFound when the page does not hold that stroke, so that taking back
    /// a change the notebook no longer has is reported rather than passing for success.
    [[nodiscard]] Result<void> removeStroke(const Uuid& pageId, const Uuid& strokeId);

    /// Removes every stroke of the page @p pageId and returns how many were removed.
    [[nodiscard]] Result<std::size_t> removeStrokesOfPage(const Uuid& pageId);

    /// Returns the strokes of the page @p pageId, in the order they were drawn.
    [[nodiscard]] Result<std::vector<Stroke>> strokesOfPage(const Uuid& pageId) const;

    /// Schema version of the open file.
    [[nodiscard]] Result<int> schemaVersion() const;

private:
    explicit NotebookStore(sqlite3* database) noexcept;

    void close() noexcept;

    sqlite3* m_database{nullptr};
};

} // namespace phvikapen::core
