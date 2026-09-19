#include "core/undo/OutlineCommands.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Asset.hpp"
#include "core/model/Outline.hpp"
#include "core/model/PageStyle.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace phvikapen::core {

AddPageCommand::AddPageCommand(Outline* outline, StorageThread* storage, PagePlace place,
                               PageInfo page) noexcept
    : m_outline{outline}, m_storage{storage}, m_place{place}, m_page{std::move(page)} {}

Result<void> AddPageCommand::apply() {
    if (const Result<void> inserted = m_outline->insertPage(m_place, m_page); !inserted) {
        return inserted;
    }
    const Uuid sectionId = m_place.sectionId;
    std::vector<Uuid> order = m_outline->pageOrder(sectionId);
    if (m_stored) {
        m_storage->submit(
            [sectionId, pageId = m_page.id, order = std::move(order)](NotebookStore& store) {
                return store.restorePage(sectionId, pageId, order);
            });
    } else {
        m_storage->submit(
            [sectionId, page = m_page, order = std::move(order)](NotebookStore& store) {
                return store.insertPage(sectionId, page, order);
            });
        m_stored = true;
    }
    return {};
}

Result<void> AddPageCommand::revert() {
    if (const Result<RemovedPage> removed = m_outline->removePage(m_page.id); !removed) {
        return std::unexpected{removed.error()};
    }
    m_storage->submit(
        [pageId = m_page.id](NotebookStore& store) { return store.trashPage(pageId); });
    return {};
}

std::optional<Uuid> AddPageCommand::pageToShow() const {
    return m_page.id;
}

DuplicatePageCommand::DuplicatePageCommand(Outline* outline, StorageThread* storage,
                                           PagePlace place, PageInfo page,
                                           std::vector<PlacedStroke> strokes) noexcept
    : m_outline{outline}, m_storage{storage}, m_place{place}, m_page{std::move(page)},
      m_strokes{std::move(strokes)} {}

Result<void> DuplicatePageCommand::apply() {
    if (const Result<void> inserted = m_outline->insertPage(m_place, m_page); !inserted) {
        return inserted;
    }
    const Uuid sectionId = m_place.sectionId;
    std::vector<Uuid> order = m_outline->pageOrder(sectionId);
    m_storage->submit([sectionId, page = m_page, order = std::move(order)](NotebookStore& store) {
        return store.insertPage(sectionId, page, order);
    });
    for (const PlacedStroke& placed : m_strokes) {
        m_storage->insertStroke(m_page.id, placed);
    }
    return {};
}

Result<void> DuplicatePageCommand::revert() {
    if (const Result<RemovedPage> removed = m_outline->removePage(m_page.id); !removed) {
        return std::unexpected{removed.error()};
    }
    m_storage->removeStrokesOfPage(m_page.id);
    m_storage->submit(
        [pageId = m_page.id](NotebookStore& store) { return store.trashPage(pageId); });
    return {};
}

std::optional<Uuid> DuplicatePageCommand::pageToShow() const {
    return m_page.id;
}

DeletePageCommand::DeletePageCommand(Outline* outline, StorageThread* storage,
                                     const Uuid& pageId) noexcept
    : m_outline{outline}, m_storage{storage}, m_pageId{pageId} {}

Result<void> DeletePageCommand::apply() {
    const std::optional<PagePlace> place = m_outline->placeOf(m_pageId);
    if (!place) {
        return makeError(ErrorCode::NotFound, "the notebook has no such page");
    }
    if (m_outline->pageOrder(place->sectionId).size() < 2) {
        return makeError(ErrorCode::InvalidArgument, "a section keeps at least one page");
    }
    Result<RemovedPage> removed = m_outline->removePage(m_pageId);
    if (!removed) {
        return std::unexpected{removed.error()};
    }
    m_removed = std::move(*removed);
    m_storage->submit(
        [pageId = m_pageId](NotebookStore& store) { return store.trashPage(pageId); });
    return {};
}

Result<void> DeletePageCommand::revert() {
    if (!m_removed) {
        return makeError(ErrorCode::NotFound, "the page was never deleted");
    }
    const PagePlace place = m_removed->place;
    if (const Result<void> inserted = m_outline->insertPage(place, m_removed->page); !inserted) {
        return inserted;
    }
    m_removed.reset();
    m_storage->submit([place, pageId = m_pageId,
                       order = m_outline->pageOrder(place.sectionId)](NotebookStore& store) {
        return store.restorePage(place.sectionId, pageId, order);
    });
    return {};
}

std::optional<Uuid> DeletePageCommand::pageToShow() const {
    return m_pageId;
}

MovePageCommand::MovePageCommand(Outline* outline, StorageThread* storage, const Uuid& pageId,
                                 PagePlace place) noexcept
    : m_outline{outline}, m_storage{storage}, m_pageId{pageId}, m_place{place} {}

Result<void> MovePageCommand::moveTo(PagePlace place) {
    const Result<PagePlace> previous = m_outline->movePage(m_pageId, place);
    if (!previous) {
        return std::unexpected{previous.error()};
    }
    m_previous = *previous;
    m_storage->submit([sectionId = place.sectionId, order = m_outline->pageOrder(place.sectionId)](
                          NotebookStore& store) { return store.orderPages(sectionId, order); });
    if (previous->sectionId != place.sectionId) {
        m_storage->submit(
            [sectionId = previous->sectionId, order = m_outline->pageOrder(previous->sectionId)](
                NotebookStore& store) { return store.orderPages(sectionId, order); });
    }
    return {};
}

Result<void> MovePageCommand::apply() {
    return moveTo(m_place);
}

Result<void> MovePageCommand::revert() {
    return moveTo(m_previous);
}

std::optional<Uuid> MovePageCommand::pageToShow() const {
    return m_pageId;
}

RenamePageCommand::RenamePageCommand(Outline* outline, StorageThread* storage, const Uuid& pageId,
                                     std::string title) noexcept
    : m_outline{outline}, m_storage{storage}, m_pageId{pageId}, m_title{std::move(title)} {}

Result<void> RenamePageCommand::apply() {
    Result<std::string> previous = m_outline->renamePage(m_pageId, m_title);
    if (!previous) {
        return std::unexpected{previous.error()};
    }
    m_storage->submit([pageId = m_pageId, title = m_title](NotebookStore& store) {
        return store.renamePage(pageId, title);
    });
    m_title = std::move(*previous);
    return {};
}

Result<void> RenamePageCommand::revert() {
    return apply();
}

std::optional<Uuid> RenamePageCommand::pageToShow() const {
    return m_pageId;
}

SetPageStyleCommand::SetPageStyleCommand(Outline* outline, StorageThread* storage,
                                         const Uuid& pageId, const PageStyle& style) noexcept
    : m_outline{outline}, m_storage{storage}, m_pageId{pageId}, m_style{style} {}

Result<void> SetPageStyleCommand::apply() {
    const Result<PageStyle> previous = m_outline->setPageStyle(m_pageId, m_style);
    if (!previous) {
        return std::unexpected{previous.error()};
    }
    m_storage->submit([pageId = m_pageId, style = m_outline->page(m_pageId)->style](
                          NotebookStore& store) { return store.setPageStyle(pageId, style); });
    m_style = *previous;
    return {};
}

Result<void> SetPageStyleCommand::revert() {
    return apply();
}

std::optional<Uuid> SetPageStyleCommand::pageToShow() const {
    return m_pageId;
}

SetSectionStyleCommand::SetSectionStyleCommand(Outline* outline, StorageThread* storage,
                                               std::vector<Uuid> pageIds, const PageStyle& style)
    : m_outline{outline}, m_storage{storage}, m_pageIds{std::move(pageIds)}, m_style{style} {}

Result<void> SetSectionStyleCommand::apply() {
    std::vector<PageStyle> before;
    before.reserve(m_pageIds.size());
    for (const Uuid& pageId : m_pageIds) {
        const Result<PageStyle> previous = m_outline->setPageStyle(pageId, m_style);
        if (!previous) {
            return std::unexpected{previous.error()};
        }
        before.push_back(*previous);
        m_storage->submit([pageId, style = m_outline->page(pageId)->style](NotebookStore& store) {
            return store.setPageStyle(pageId, style);
        });
    }
    m_wasStyle = std::move(before);
    return {};
}

Result<void> SetSectionStyleCommand::revert() {
    for (std::size_t index = 0; index < m_pageIds.size() && index < m_wasStyle.size(); ++index) {
        const Uuid& pageId = m_pageIds[index];
        const Result<PageStyle> previous = m_outline->setPageStyle(pageId, m_wasStyle[index]);
        if (!previous) {
            return std::unexpected{previous.error()};
        }
        m_storage->submit([pageId, style = m_outline->page(pageId)->style](NotebookStore& store) {
            return store.setPageStyle(pageId, style);
        });
    }
    return {};
}

std::optional<Uuid> SetSectionStyleCommand::pageToShow() const {
    return m_pageIds.empty() ? std::nullopt : std::optional<Uuid>{m_pageIds.front()};
}

SetPageMediaCommand::SetPageMediaCommand(Outline* outline, StorageThread* storage,
                                         const Uuid& pageId,
                                         std::optional<PageMedia> media) noexcept
    : m_outline{outline}, m_storage{storage}, m_pageId{pageId}, m_media{media} {}

Result<void> SetPageMediaCommand::apply() {
    Result<std::optional<PageMedia>> previous = m_outline->setPageMedia(m_pageId, m_media);
    if (!previous) {
        return std::unexpected{previous.error()};
    }
    m_storage->submit([pageId = m_pageId, media = m_media](NotebookStore& store) {
        return store.setPageMedia(pageId, media);
    });
    m_media = *previous;
    return {};
}

Result<void> SetPageMediaCommand::revert() {
    return apply();
}

std::optional<Uuid> SetPageMediaCommand::pageToShow() const {
    return m_pageId;
}

ImportPagesCommand::ImportPagesCommand(Outline* outline, StorageThread* storage, PagePlace place,
                                       Asset asset, std::vector<PageInfo> pages) noexcept
    : m_outline{outline}, m_storage{storage}, m_place{place}, m_asset{std::move(asset)},
      m_pages{std::move(pages)} {}

Result<void> ImportPagesCommand::apply() {
    if (m_pages.empty()) {
        return makeError(ErrorCode::InvalidArgument, "there are no pages to import");
    }
    std::size_t index = m_place.index;
    for (const PageInfo& page : m_pages) {
        const PagePlace place{.sectionId = m_place.sectionId, .index = index++};
        if (const Result<void> inserted = m_outline->insertPage(place, page); !inserted) {
            return inserted;
        }
    }

    const Uuid sectionId = m_place.sectionId;
    std::vector<Uuid> order = m_outline->pageOrder(sectionId);
    if (m_stored) {
        std::vector<Uuid> restored;
        restored.reserve(m_pages.size());
        for (const PageInfo& page : m_pages) {
            restored.push_back(page.id);
        }
        m_storage->submit([sectionId, restored = std::move(restored),
                           order = std::move(order)](NotebookStore& store) {
            return store.restorePages(sectionId, restored, order);
        });
        return {};
    }

    m_storage->submit([asset = m_asset](NotebookStore& store) { return store.insertAsset(asset); });
    m_storage->submit([sectionId, pages = m_pages, order = std::move(order)](NotebookStore& store) {
        return store.insertPages(sectionId, pages, order);
    });
    m_stored = true;
    return {};
}

Result<void> ImportPagesCommand::revert() {
    for (const PageInfo& page : m_pages) {
        if (const Result<RemovedPage> removed = m_outline->removePage(page.id); !removed) {
            return std::unexpected{removed.error()};
        }
        m_storage->submit(
            [pageId = page.id](NotebookStore& store) { return store.trashPage(pageId); });
    }
    return {};
}

std::optional<Uuid> ImportPagesCommand::pageToShow() const {
    return m_pages.empty() ? std::nullopt : std::optional<Uuid>{m_pages.front().id};
}

AddSectionCommand::AddSectionCommand(Outline* outline, StorageThread* storage, std::size_t index,
                                     SectionInfo section) noexcept
    : m_outline{outline}, m_storage{storage}, m_index{index}, m_section{std::move(section)} {}

Result<void> AddSectionCommand::apply() {
    if (m_section.pages.empty()) {
        return makeError(ErrorCode::InvalidArgument, "a section needs at least one page");
    }
    if (const Result<void> inserted = m_outline->insertSection(m_index, m_section); !inserted) {
        return inserted;
    }
    std::vector<Uuid> order = m_outline->sectionOrder();
    if (m_stored) {
        m_storage->submit(
            [sectionId = m_section.id, order = std::move(order)](NotebookStore& store) {
                return store.restoreSection(sectionId, order);
            });
        return {};
    }
    m_storage->submit([section = m_section, order = std::move(order)](NotebookStore& store) {
        if (const Result<void> inserted = store.insertSection(section.id, section.title, order);
            !inserted) {
            return inserted;
        }
        std::vector<Uuid> pageOrder;
        for (const PageInfo& page : section.pages) {
            pageOrder.push_back(page.id);
            if (const Result<void> inserted = store.insertPage(section.id, page, pageOrder);
                !inserted) {
                return inserted;
            }
        }
        return Result<void>{};
    });
    m_stored = true;
    return {};
}

Result<void> AddSectionCommand::revert() {
    if (const Result<RemovedSection> removed = m_outline->removeSection(m_section.id); !removed) {
        return std::unexpected{removed.error()};
    }
    m_storage->submit(
        [sectionId = m_section.id](NotebookStore& store) { return store.trashSection(sectionId); });
    return {};
}

std::optional<Uuid> AddSectionCommand::pageToShow() const {
    return m_section.pages.front().id;
}

DeleteSectionCommand::DeleteSectionCommand(Outline* outline, StorageThread* storage,
                                           const Uuid& sectionId) noexcept
    : m_outline{outline}, m_storage{storage}, m_sectionId{sectionId} {}

Result<void> DeleteSectionCommand::apply() {
    if (m_outline->sections().size() < 2) {
        return makeError(ErrorCode::InvalidArgument, "a notebook keeps at least one section");
    }
    Result<RemovedSection> removed = m_outline->removeSection(m_sectionId);
    if (!removed) {
        return std::unexpected{removed.error()};
    }
    m_removed = std::move(*removed);
    m_storage->submit(
        [sectionId = m_sectionId](NotebookStore& store) { return store.trashSection(sectionId); });
    return {};
}

Result<void> DeleteSectionCommand::revert() {
    if (!m_removed) {
        return makeError(ErrorCode::NotFound, "the section was never deleted");
    }
    if (const Result<void> inserted =
            m_outline->insertSection(m_removed->index, m_removed->section);
        !inserted) {
        return inserted;
    }
    m_removed.reset();
    m_storage->submit([sectionId = m_sectionId, order = m_outline->sectionOrder()](
                          NotebookStore& store) { return store.restoreSection(sectionId, order); });
    return {};
}

MoveSectionCommand::MoveSectionCommand(Outline* outline, StorageThread* storage,
                                       const Uuid& sectionId, std::size_t index) noexcept
    : m_outline{outline}, m_storage{storage}, m_sectionId{sectionId}, m_index{index} {}

Result<void> MoveSectionCommand::moveTo(std::size_t index) {
    const Result<std::size_t> previous = m_outline->moveSection(m_sectionId, index);
    if (!previous) {
        return std::unexpected{previous.error()};
    }
    m_previous = *previous;
    m_storage->submit([order = m_outline->sectionOrder()](NotebookStore& store) {
        return store.orderSections(order);
    });
    return {};
}

Result<void> MoveSectionCommand::apply() {
    return moveTo(m_index);
}

Result<void> MoveSectionCommand::revert() {
    return moveTo(m_previous);
}

RenameSectionCommand::RenameSectionCommand(Outline* outline, StorageThread* storage,
                                           const Uuid& sectionId, std::string title) noexcept
    : m_outline{outline}, m_storage{storage}, m_sectionId{sectionId}, m_title{std::move(title)} {}

Result<void> RenameSectionCommand::apply() {
    Result<std::string> previous = m_outline->renameSection(m_sectionId, m_title);
    if (!previous) {
        return std::unexpected{previous.error()};
    }
    m_storage->submit([sectionId = m_sectionId, title = m_title](NotebookStore& store) {
        return store.renameSection(sectionId, title);
    });
    m_title = std::move(*previous);
    return {};
}

Result<void> RenameSectionCommand::revert() {
    return apply();
}

}
