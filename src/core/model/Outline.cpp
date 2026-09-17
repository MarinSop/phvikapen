#include "core/model/Outline.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/PageStyle.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] std::ptrdiff_t offset(std::size_t index) noexcept {
    return static_cast<std::ptrdiff_t>(index);
}

}

Outline::Outline(NotebookOutline contents) : m_contents{std::move(contents)} {}

void Outline::setTitle(std::string title) {
    m_contents.title = std::move(title);
}

std::optional<std::size_t> Outline::sectionIndex(const Uuid& sectionId) const noexcept {
    const auto found = std::ranges::find(m_contents.sections, sectionId, &SectionInfo::id);
    if (found == m_contents.sections.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(m_contents.sections.begin(), found));
}

std::optional<PagePlace> Outline::placeOf(const Uuid& pageId) const noexcept {
    for (const SectionInfo& section : m_contents.sections) {
        const auto found = std::ranges::find(section.pages, pageId, &PageInfo::id);
        if (found != section.pages.end()) {
            return PagePlace{
                .sectionId = section.id,
                .index = static_cast<std::size_t>(std::distance(section.pages.begin(), found)),
            };
        }
    }
    return std::nullopt;
}

const PageInfo* Outline::page(const Uuid& pageId) const noexcept {
    for (const SectionInfo& section : m_contents.sections) {
        const auto found = std::ranges::find(section.pages, pageId, &PageInfo::id);
        if (found != section.pages.end()) {
            return &*found;
        }
    }
    return nullptr;
}

SectionInfo* Outline::findSection(const Uuid& sectionId) noexcept {
    const auto found = std::ranges::find(m_contents.sections, sectionId, &SectionInfo::id);
    return found == m_contents.sections.end() ? nullptr : &*found;
}

PageInfo* Outline::findPage(const Uuid& pageId) noexcept {
    for (SectionInfo& section : m_contents.sections) {
        const auto found = std::ranges::find(section.pages, pageId, &PageInfo::id);
        if (found != section.pages.end()) {
            return &*found;
        }
    }
    return nullptr;
}

std::vector<Uuid> Outline::sectionOrder() const {
    std::vector<Uuid> order;
    order.reserve(m_contents.sections.size());
    for (const SectionInfo& section : m_contents.sections) {
        order.push_back(section.id);
    }
    return order;
}

std::vector<Uuid> Outline::pageOrder(const Uuid& sectionId) const {
    std::vector<Uuid> order;
    const auto found = std::ranges::find(m_contents.sections, sectionId, &SectionInfo::id);
    if (found == m_contents.sections.end()) {
        return order;
    }
    order.reserve(found->pages.size());
    for (const PageInfo& page : found->pages) {
        order.push_back(page.id);
    }
    return order;
}

Result<void> Outline::insertSection(std::size_t index, SectionInfo section) {
    if (index > m_contents.sections.size()) {
        return makeError(ErrorCode::InvalidArgument, "the section would land outside the notebook");
    }
    if (findSection(section.id) != nullptr) {
        return makeError(ErrorCode::InvalidArgument, "the notebook already has that section");
    }
    for (const PageInfo& page : section.pages) {
        if (findPage(page.id) != nullptr) {
            return makeError(ErrorCode::InvalidArgument, "the notebook already has that page");
        }
    }
    m_contents.sections.insert(std::next(m_contents.sections.begin(), offset(index)),
                               std::move(section));
    return {};
}

Result<RemovedSection> Outline::removeSection(const Uuid& sectionId) {
    const std::optional<std::size_t> index = sectionIndex(sectionId);
    if (!index) {
        return makeError(ErrorCode::NotFound, "the notebook has no such section");
    }
    const auto position = std::next(m_contents.sections.begin(), offset(*index));
    RemovedSection removed{.section = std::move(*position), .index = *index};
    m_contents.sections.erase(position);
    return removed;
}

Result<std::string> Outline::renameSection(const Uuid& sectionId, std::string title) {
    SectionInfo* const section = findSection(sectionId);
    if (section == nullptr) {
        return makeError(ErrorCode::NotFound, "the notebook has no such section");
    }
    return std::exchange(section->title, std::move(title));
}

Result<std::size_t> Outline::moveSection(const Uuid& sectionId, std::size_t index) {
    if (index >= m_contents.sections.size()) {
        return makeError(ErrorCode::InvalidArgument, "the section would land outside the notebook");
    }
    Result<RemovedSection> removed = removeSection(sectionId);
    if (!removed) {
        return std::unexpected{removed.error()};
    }
    m_contents.sections.insert(std::next(m_contents.sections.begin(), offset(index)),
                               std::move(removed->section));
    return removed->index;
}

Result<void> Outline::insertPage(const PagePlace& place, PageInfo page) {
    SectionInfo* const section = findSection(place.sectionId);
    if (section == nullptr) {
        return makeError(ErrorCode::NotFound, "the notebook has no such section");
    }
    if (place.index > section->pages.size()) {
        return makeError(ErrorCode::InvalidArgument, "the page would land outside its section");
    }
    if (findPage(page.id) != nullptr) {
        return makeError(ErrorCode::InvalidArgument, "the notebook already has that page");
    }
    section->pages.insert(std::next(section->pages.begin(), offset(place.index)), std::move(page));
    return {};
}

Result<RemovedPage> Outline::removePage(const Uuid& pageId) {
    const std::optional<PagePlace> place = placeOf(pageId);
    if (!place) {
        return makeError(ErrorCode::NotFound, "the notebook has no such page");
    }
    SectionInfo* const section = findSection(place->sectionId);
    const auto position = std::next(section->pages.begin(), offset(place->index));
    RemovedPage removed{.page = std::move(*position), .place = *place};
    section->pages.erase(position);
    return removed;
}

Result<std::string> Outline::renamePage(const Uuid& pageId, std::string title) {
    PageInfo* const page = findPage(pageId);
    if (page == nullptr) {
        return makeError(ErrorCode::NotFound, "the notebook has no such page");
    }
    return std::exchange(page->title, std::move(title));
}

Result<PageStyle> Outline::setPageStyle(const Uuid& pageId, const PageStyle& style) {
    PageInfo* const page = findPage(pageId);
    if (page == nullptr) {
        return makeError(ErrorCode::NotFound, "the notebook has no such page");
    }
    return std::exchange(page->style, normalized(style));
}

Result<PagePlace> Outline::movePage(const Uuid& pageId, const PagePlace& place) {
    const std::optional<PagePlace> from = placeOf(pageId);
    if (!from) {
        return makeError(ErrorCode::NotFound, "the notebook has no such page");
    }
    const SectionInfo* const target = findSection(place.sectionId);
    if (target == nullptr) {
        return makeError(ErrorCode::NotFound, "the notebook has no such section");
    }
    const std::size_t sizeAfterRemoval =
        target->pages.size() - (from->sectionId == place.sectionId ? 1 : 0);
    if (place.index > sizeAfterRemoval) {
        return makeError(ErrorCode::InvalidArgument, "the page would land outside its section");
    }
    Result<RemovedPage> removed = removePage(pageId);
    if (!removed) {
        return std::unexpected{removed.error()};
    }
    SectionInfo* const section = findSection(place.sectionId);
    section->pages.insert(std::next(section->pages.begin(), offset(place.index)),
                          std::move(removed->page));
    return *from;
}

}
