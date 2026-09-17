#include "core/undo/StrokeCommands.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/Stroke.hpp"
#include "core/storage/NotebookStore.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace phvikapen::core {

AddStrokeCommand::AddStrokeCommand(NotebookStore* store, const Uuid& pageId,
                                   PlacedStroke placed) noexcept
    : m_store{store}, m_pageId{pageId}, m_placed{std::move(placed)} {}

Result<void> AddStrokeCommand::apply() {
    return m_store->insertStroke(m_pageId, m_placed);
}

Result<void> AddStrokeCommand::revert() {
    return m_store->removeStroke(m_pageId, m_placed.stroke.id());
}

ClearPageCommand::ClearPageCommand(NotebookStore* store, const Uuid& pageId) noexcept
    : m_store{store}, m_pageId{pageId} {}

Result<void> ClearPageCommand::apply() {
    Result<std::vector<PlacedStroke>> strokes = m_store->strokesOfPage(m_pageId);
    if (!strokes) {
        return std::unexpected{strokes.error()};
    }
    const Result<std::size_t> removed = m_store->removeStrokesOfPage(m_pageId);
    if (!removed) {
        return std::unexpected{removed.error()};
    }
    m_removed = std::move(*strokes);
    return {};
}

Result<void> ClearPageCommand::revert() {
    for (const PlacedStroke& placed : m_removed) {
        if (const Result<void> restored = m_store->insertStroke(m_pageId, placed); !restored) {
            return restored;
        }
    }
    m_removed.clear();
    return {};
}

}
