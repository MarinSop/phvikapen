#pragma once

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/PageStyle.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace phvikapen::core {

struct PageInfo {
    Uuid id;
    std::string title;
    PageStyle style;

    friend bool operator==(const PageInfo&, const PageInfo&) = default;
};

struct SectionInfo {
    Uuid id;
    std::string title;
    std::vector<PageInfo> pages;

    friend bool operator==(const SectionInfo&, const SectionInfo&) = default;
};

struct NotebookOutline {
    std::string title;
    std::vector<SectionInfo> sections;

    friend bool operator==(const NotebookOutline&, const NotebookOutline&) = default;
};

struct PagePlace {
    Uuid sectionId;
    std::size_t index{};

    friend bool operator==(const PagePlace&, const PagePlace&) = default;
};

struct RemovedPage {
    PageInfo page;
    PagePlace place;
};

struct RemovedSection {
    SectionInfo section;
    std::size_t index{};
};

class Outline {
public:
    explicit Outline(NotebookOutline contents = {});

    [[nodiscard]] const NotebookOutline& contents() const noexcept { return m_contents; }

    [[nodiscard]] std::span<const SectionInfo> sections() const noexcept {
        return m_contents.sections;
    }

    void setTitle(std::string title);

    [[nodiscard]] std::optional<std::size_t> sectionIndex(const Uuid& sectionId) const noexcept;
    [[nodiscard]] std::optional<PagePlace> placeOf(const Uuid& pageId) const noexcept;
    [[nodiscard]] const PageInfo* page(const Uuid& pageId) const noexcept;
    [[nodiscard]] std::vector<Uuid> sectionOrder() const;
    [[nodiscard]] std::vector<Uuid> pageOrder(const Uuid& sectionId) const;

    [[nodiscard]] Result<void> insertSection(std::size_t index, SectionInfo section);
    [[nodiscard]] Result<RemovedSection> removeSection(const Uuid& sectionId);
    [[nodiscard]] Result<std::string> renameSection(const Uuid& sectionId, std::string title);
    [[nodiscard]] Result<std::size_t> moveSection(const Uuid& sectionId, std::size_t index);

    [[nodiscard]] Result<void> insertPage(const PagePlace& place, PageInfo page);
    [[nodiscard]] Result<RemovedPage> removePage(const Uuid& pageId);
    [[nodiscard]] Result<std::string> renamePage(const Uuid& pageId, std::string title);
    [[nodiscard]] Result<PageStyle> setPageStyle(const Uuid& pageId, const PageStyle& style);
    [[nodiscard]] Result<PagePlace> movePage(const Uuid& pageId, const PagePlace& place);

private:
    [[nodiscard]] SectionInfo* findSection(const Uuid& sectionId) noexcept;
    [[nodiscard]] PageInfo* findPage(const Uuid& pageId) noexcept;

    NotebookOutline m_contents;
};

}
