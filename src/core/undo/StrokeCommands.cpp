#include "core/undo/StrokeCommands.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/Stroke.hpp"
#include "core/storage/NotebookStore.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace phvikapen::core {

AddStrokeCommand::AddStrokeCommand(NotebookStore* store, const Uuid& pageId, Stroke stroke) noexcept
    : m_store{store}, m_pageId{pageId}, m_stroke{std::move(stroke)} {}

Result<void> AddStrokeCommand::apply() {
    return m_store->appendStroke(m_pageId, m_stroke);
}

Result<void> AddStrokeCommand::revert() {
    return m_store->removeStroke(m_pageId, m_stroke.id());
}

ClearPageCommand::ClearPageCommand(NotebookStore* store, const Uuid& pageId) noexcept
    : m_store{store}, m_pageId{pageId} {}

Result<void> ClearPageCommand::apply() {
    Result<std::vector<Stroke>> strokes = m_store->strokesOfPage(m_pageId);
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
    for (const Stroke& stroke : m_removed) {
        if (const Result<void> restored = m_store->appendStroke(m_pageId, stroke); !restored) {
            return restored;
        }
    }
    m_removed.clear();
    return {};
}

}
