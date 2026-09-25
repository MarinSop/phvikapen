#include "core/undo/TableCommands.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Page.hpp"
#include "core/model/Table.hpp"
#include "core/storage/NotebookStore.hpp"
#include "core/storage/StorageThread.hpp"

#include <optional>
#include <utility>

namespace phvikapen::core {

AddTableCommand::AddTableCommand(Page* page, StorageThread* storage, PlacedTable placed)
    : m_page{page}, m_storage{storage}, m_placed{std::move(placed)} {}

Result<void> AddTableCommand::apply() {
    if (const Result<void> inserted = m_page->insertTable(m_placed); !inserted) {
        return inserted;
    }
    m_storage->submit([pageId = m_page->id(), placed = m_placed](NotebookStore& store) {
        return store.insertTable(pageId, placed);
    });
    return {};
}

Result<void> AddTableCommand::revert() {
    if (const Result<PlacedTable> removed = m_page->removeTable(m_placed.table.id); !removed) {
        return std::unexpected{removed.error()};
    }
    m_storage->submit([pageId = m_page->id(), tableId = m_placed.table.id](NotebookStore& store) {
        return store.removeTable(pageId, tableId);
    });
    return {};
}

std::optional<Uuid> AddTableCommand::pageToShow() const {
    return m_page->id();
}

RemoveTableCommand::RemoveTableCommand(Page* page, StorageThread* storage,
                                       const Uuid& tableId) noexcept
    : m_page{page}, m_storage{storage}, m_tableId{tableId} {}

Result<void> RemoveTableCommand::apply() {
    Result<PlacedTable> removed = m_page->removeTable(m_tableId);
    if (!removed) {
        return std::unexpected{removed.error()};
    }
    m_removed = std::move(*removed);
    m_storage->submit([pageId = m_page->id(), tableId = m_tableId](NotebookStore& store) {
        return store.removeTable(pageId, tableId);
    });
    return {};
}

Result<void> RemoveTableCommand::revert() {
    if (!m_removed) {
        return makeError(ErrorCode::InvalidArgument, "there is no table to put back");
    }
    if (const Result<void> inserted = m_page->insertTable(*m_removed); !inserted) {
        return inserted;
    }
    m_storage->submit([pageId = m_page->id(), placed = *m_removed](NotebookStore& store) {
        return store.insertTable(pageId, placed);
    });
    return {};
}

std::optional<Uuid> RemoveTableCommand::pageToShow() const {
    return m_page->id();
}

ChangeTableCommand::ChangeTableCommand(Page* page, StorageThread* storage, Table table)
    : m_page{page}, m_storage{storage}, m_after{std::move(table)} {}

Result<void> ChangeTableCommand::write(Table table) {
    if (const Result<void> written = m_page->replaceTable(table); !written) {
        return written;
    }
    m_storage->submit([pageId = m_page->id(), table = std::move(table)](NotebookStore& store) {
        return store.updateTable(pageId, table);
    });
    return {};
}

Result<void> ChangeTableCommand::apply() {
    if (!m_before) {
        const Table* const kept = m_page->tableAt(m_after.id);
        if (kept == nullptr) {
            return makeError(ErrorCode::NotFound, "the page does not hold that table");
        }
        m_before = *kept;
    }
    return write(m_after);
}

Result<void> ChangeTableCommand::revert() {
    if (!m_before) {
        return makeError(ErrorCode::InvalidArgument, "there is nothing to go back to");
    }
    return write(*m_before);
}

std::optional<Uuid> ChangeTableCommand::pageToShow() const {
    return m_page->id();
}

}
