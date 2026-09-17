#include "core/undo/StrokeCommands.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Page.hpp"
#include "core/storage/StorageThread.hpp"

#include <tuple>
#include <utility>
#include <vector>

namespace phvikapen::core {
namespace {

[[nodiscard]] Result<void> restoreStrokes(Page& page, StorageThread& storage,
                                          std::vector<PlacedStroke> strokes) {
    for (PlacedStroke& placed : strokes) {
        if (const Result<void> inserted = page.insert(placed); !inserted) {
            return inserted;
        }
        storage.insertStroke(page.id(), std::move(placed));
    }
    return {};
}

}

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
    return restoreStrokes(*m_page, *m_storage, std::exchange(m_removed, {}));
}

EraseStrokesCommand::EraseStrokesCommand(Page* page, StorageThread* storage,
                                         std::vector<Uuid> strokeIds) noexcept
    : m_page{page}, m_storage{storage}, m_strokeIds{std::move(strokeIds)} {}

Result<void> EraseStrokesCommand::apply() {
    std::vector<PlacedStroke> erased;
    erased.reserve(m_strokeIds.size());
    for (const Uuid& strokeId : m_strokeIds) {
        Result<PlacedStroke> removed = m_page->remove(strokeId);
        if (!removed) {
            for (PlacedStroke& placed : erased) {
                std::ignore = m_page->insert(std::move(placed));
            }
            return std::unexpected{removed.error()};
        }
        erased.push_back(std::move(*removed));
    }
    for (const Uuid& strokeId : m_strokeIds) {
        m_storage->removeStroke(m_page->id(), strokeId);
    }
    m_erased = std::move(erased);
    return {};
}

Result<void> EraseStrokesCommand::revert() {
    return restoreStrokes(*m_page, *m_storage, std::exchange(m_erased, {}));
}

}
