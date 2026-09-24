#include "core/undo/TextCommands.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Page.hpp"
#include "core/model/TextBox.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"

#include <optional>
#include <utility>

namespace phvikapen::core {

AddTextCommand::AddTextCommand(Page* page, StorageThread* storage, PlacedText placed) noexcept
    : m_page{page}, m_storage{storage}, m_placed{std::move(placed)} {}

Result<void> AddTextCommand::apply() {
    if (const Result<void> inserted = m_page->insertText(m_placed); !inserted) {
        return inserted;
    }
    m_storage->submit([pageId = m_page->id(), placed = m_placed](NotebookStore& store) {
        return store.insertText(pageId, placed);
    });
    return {};
}

Result<void> AddTextCommand::revert() {
    if (const Result<PlacedText> removed = m_page->removeText(m_placed.box.id); !removed) {
        return std::unexpected{removed.error()};
    }
    m_storage->submit([pageId = m_page->id(), textId = m_placed.box.id](NotebookStore& store) {
        return store.removeText(pageId, textId);
    });
    return {};
}

std::optional<Uuid> AddTextCommand::pageToShow() const {
    return m_page->id();
}

RemoveTextCommand::RemoveTextCommand(Page* page, StorageThread* storage,
                                     const Uuid& textId) noexcept
    : m_page{page}, m_storage{storage}, m_textId{textId} {}

Result<void> RemoveTextCommand::apply() {
    Result<PlacedText> removed = m_page->removeText(m_textId);
    if (!removed) {
        return std::unexpected{removed.error()};
    }
    m_removed = std::move(*removed);
    m_storage->submit([pageId = m_page->id(), textId = m_textId](NotebookStore& store) {
        return store.removeText(pageId, textId);
    });
    return {};
}

Result<void> RemoveTextCommand::revert() {
    if (!m_removed) {
        return makeError(ErrorCode::InvalidArgument, "there is no text to put back");
    }
    if (const Result<void> inserted = m_page->insertText(*m_removed); !inserted) {
        return inserted;
    }
    m_storage->submit([pageId = m_page->id(), placed = *m_removed](NotebookStore& store) {
        return store.insertText(pageId, placed);
    });
    return {};
}

std::optional<Uuid> RemoveTextCommand::pageToShow() const {
    return m_page->id();
}

ChangeTextCommand::ChangeTextCommand(Page* page, StorageThread* storage, TextBox box) noexcept
    : m_page{page}, m_storage{storage}, m_after{std::move(box)} {}

Result<void> ChangeTextCommand::write(TextBox box) {
    if (const Result<void> written = m_page->replaceText(box); !written) {
        return written;
    }
    m_storage->submit([pageId = m_page->id(), box = std::move(box)](NotebookStore& store) {
        return store.updateText(pageId, box);
    });
    return {};
}

Result<void> ChangeTextCommand::apply() {
    if (!m_before) {
        const TextBox* const kept = m_page->textAt(m_after.id);
        if (kept == nullptr) {
            return makeError(ErrorCode::NotFound, "the page does not hold that text");
        }
        m_before = *kept;
    }
    return write(m_after);
}

Result<void> ChangeTextCommand::revert() {
    if (!m_before) {
        return makeError(ErrorCode::InvalidArgument, "there is nothing to go back to");
    }
    return write(*m_before);
}

std::optional<Uuid> ChangeTextCommand::pageToShow() const {
    return m_page->id();
}

}
