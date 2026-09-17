#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Page.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

struct sqlite3;

namespace phvikapen::core {

inline constexpr int kNotebookSchemaVersion = 2;

class NotebookStore {
public:
    [[nodiscard]] static Result<NotebookStore> open(const std::filesystem::path& path);

    ~NotebookStore();

    NotebookStore(NotebookStore&& other) noexcept;
    NotebookStore& operator=(NotebookStore&& other) noexcept;

    NotebookStore(const NotebookStore&) = delete;
    NotebookStore& operator=(const NotebookStore&) = delete;

    [[nodiscard]] Result<void> insertStroke(const Uuid& pageId, const PlacedStroke& placed);

    [[nodiscard]] Result<void> removeStroke(const Uuid& pageId, const Uuid& strokeId);

    [[nodiscard]] Result<std::size_t> removeStrokesOfPage(const Uuid& pageId);

    [[nodiscard]] Result<std::vector<PlacedStroke>> strokesOfPage(const Uuid& pageId) const;

    [[nodiscard]] Result<int> schemaVersion() const;

private:
    explicit NotebookStore(sqlite3* database) noexcept;

    void close() noexcept;

    sqlite3* m_database{nullptr};
};

}
