#include "core/undo/StrokeCommands.hpp"

#include "core/Error.hpp"
#include "core/model/Page.hpp"
#include "core/storage/StorageThread.hpp"

#include <utility>
#include <vector>

namespace phvikapen::core {

AddStrokeCommand::AddStrokeCommand(Page* page, StorageThread* storage, PlacedStroke placed) noexcept
    : m_page{page}, m_storage{storage}, m_placed{std::move(placed)} {}

Result<void> AddStrokeCommand::apply() {
    if (const Result<void> inserted = m_page->insert(m_placed); !inserted) {
        return inserted;
    }
    m_storage->insertStroke(m_page->id(), m_placed);
    return {};
}

Result<void> AddStrokeCommand::revert() {
    if (const Result<PlacedStroke> removed = m_page->remove(m_placed.stroke.id()); !removed) {
        return std::unexpected{removed.error()};
    }
    m_storage->removeStroke(m_page->id(), m_placed.stroke.id());
    return {};
}

ClearPageCommand::ClearPageCommand(Page* page, StorageThread* storage) noexcept
    : m_page{page}, m_storage{storage} {}

Result<void> ClearPageCommand::apply() {
    m_removed = m_page->takeAll();
    m_storage->removeStrokesOfPage(m_page->id());
    return {};
}

Result<void> ClearPageCommand::revert() {
    std::vector<PlacedStroke> removed = std::exchange(m_removed, {});
    for (PlacedStroke& placed : removed) {
        if (const Result<void> inserted = m_page->insert(placed); !inserted) {
            return inserted;
        }
        m_storage->insertStroke(m_page->id(), std::move(placed));
    }
    return {};
}

}
