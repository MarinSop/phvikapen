#include "core/undo/LinkCommands.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Link.hpp"
#include "core/model/Page.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"

#include <optional>
#include <utility>

namespace phvikapen::core {

AddLinkCommand::AddLinkCommand(Page* page, StorageThread* storage, PlacedLink placed) noexcept
    : m_page{page}, m_storage{storage}, m_placed{std::move(placed)} {}

Result<void> AddLinkCommand::apply() {
    if (const Result<void> inserted = m_page->insertLink(m_placed); !inserted) {
        return inserted;
    }
    m_storage->submit([pageId = m_page->id(), placed = m_placed](NotebookStore& store) {
        return store.insertLink(pageId, placed);
    });
    return {};
}

Result<void> AddLinkCommand::revert() {
    if (const Result<PlacedLink> removed = m_page->removeLink(m_placed.link.id); !removed) {
        return std::unexpected{removed.error()};
    }
    m_storage->submit([pageId = m_page->id(), linkId = m_placed.link.id](NotebookStore& store) {
        return store.removeLink(pageId, linkId);
    });
    return {};
}

std::optional<Uuid> AddLinkCommand::pageToShow() const {
    return m_page->id();
}

RemoveLinkCommand::RemoveLinkCommand(Page* page, StorageThread* storage,
                                     const Uuid& linkId) noexcept
    : m_page{page}, m_storage{storage}, m_linkId{linkId} {}

Result<void> RemoveLinkCommand::apply() {
    Result<PlacedLink> removed = m_page->removeLink(m_linkId);
    if (!removed) {
        return std::unexpected{removed.error()};
    }
    m_removed = *removed;
    m_storage->submit([pageId = m_page->id(), linkId = m_linkId](NotebookStore& store) {
        return store.removeLink(pageId, linkId);
    });
    return {};
}

Result<void> RemoveLinkCommand::revert() {
    if (!m_removed) {
        return makeError(ErrorCode::InvalidArgument, "there is no link to put back");
    }
    if (const Result<void> inserted = m_page->insertLink(*m_removed); !inserted) {
        return inserted;
    }
    m_storage->submit([pageId = m_page->id(), placed = *m_removed](NotebookStore& store) {
        return store.insertLink(pageId, placed);
    });
    return {};
}

std::optional<Uuid> RemoveLinkCommand::pageToShow() const {
    return m_page->id();
}

ChangeLinkCommand::ChangeLinkCommand(Page* page, StorageThread* storage, Link link) noexcept
    : m_page{page}, m_storage{storage}, m_link{std::move(link)} {}

Result<void> ChangeLinkCommand::apply() {
    const Link* const standing = m_page->linkAt(m_link.id);
    if (standing == nullptr) {
        return makeError(ErrorCode::NotFound, "there is no such link on this page");
    }
    m_was = *standing;
    if (const Result<void> changed = m_page->replaceLink(m_link); !changed) {
        return changed;
    }
    m_storage->submit([pageId = m_page->id(), link = m_link](NotebookStore& store) {
        return store.updateLink(pageId, link);
    });
    return {};
}

Result<void> ChangeLinkCommand::revert() {
    if (!m_was) {
        return makeError(ErrorCode::InvalidArgument, "there is nothing to put the link back to");
    }
    if (const Result<void> changed = m_page->replaceLink(*m_was); !changed) {
        return changed;
    }
    m_storage->submit([pageId = m_page->id(), link = *m_was](NotebookStore& store) {
        return store.updateLink(pageId, link);
    });
    return {};
}

std::optional<Uuid> ChangeLinkCommand::pageToShow() const {
    return m_page->id();
}

}
