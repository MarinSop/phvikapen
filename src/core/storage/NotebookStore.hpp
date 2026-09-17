#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Outline.hpp"
#include "core/model/Page.hpp"
#include "core/model/PageStyle.hpp"

#include <cstddef>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

struct sqlite3;

namespace phvikapen::core {

inline constexpr int kNotebookSchemaVersion = 3;

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

    [[nodiscard]] Result<NotebookOutline> readOutline() const;

    [[nodiscard]] Result<void> setTitle(std::string_view title);

    [[nodiscard]] Result<void> insertSection(const Uuid& sectionId, std::string_view title,
                                             std::span<const Uuid> sectionOrder);
    [[nodiscard]] Result<void> renameSection(const Uuid& sectionId, std::string_view title);
    [[nodiscard]] Result<void> trashSection(const Uuid& sectionId);
    [[nodiscard]] Result<void> restoreSection(const Uuid& sectionId,
                                              std::span<const Uuid> sectionOrder);
    [[nodiscard]] Result<void> orderSections(std::span<const Uuid> sectionOrder);

    [[nodiscard]] Result<void> insertPage(const Uuid& sectionId, const PageInfo& page,
                                          std::span<const Uuid> pageOrder);
    [[nodiscard]] Result<void> renamePage(const Uuid& pageId, std::string_view title);
    [[nodiscard]] Result<void> setPageStyle(const Uuid& pageId, const PageStyle& style);
    [[nodiscard]] Result<void> trashPage(const Uuid& pageId);
    [[nodiscard]] Result<void> restorePage(const Uuid& sectionId, const Uuid& pageId,
                                           std::span<const Uuid> pageOrder);
    [[nodiscard]] Result<void> orderPages(const Uuid& sectionId, std::span<const Uuid> pageOrder);

    [[nodiscard]] Result<int> schemaVersion() const;

private:
    explicit NotebookStore(sqlite3* database) noexcept;

    void close() noexcept;

    [[nodiscard]] Result<void> ensureOutline(std::string_view defaultTitle);
    [[nodiscard]] Result<void> writeSectionOrder(std::span<const Uuid> sectionOrder);
    [[nodiscard]] Result<void> writePageOrder(const Uuid& sectionId,
                                              std::span<const Uuid> pageOrder);

    sqlite3* m_database{nullptr};
};

}
