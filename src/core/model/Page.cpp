#include "core/model/Page.hpp"

#include "core/Error.hpp"
#include "core/id/Uuid.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace phvikapen::core {

Page::Page(const Uuid& id) noexcept : m_id{id} {}

std::int64_t Page::nextOrdinal() const noexcept {
    return m_strokes.empty() ? 0 : m_strokes.back().ordinal + 1;
}

Result<void> Page::insert(PlacedStroke placed) {
    const bool idTaken = std::ranges::any_of(m_strokes, [&](const PlacedStroke& existing) {
        return existing.stroke.id() == placed.stroke.id();
    });
    if (idTaken) {
        return makeError(ErrorCode::InvalidArgument, "the page already holds that stroke");
    }

    const auto position =
        std::ranges::lower_bound(m_strokes, placed.ordinal, {}, &PlacedStroke::ordinal);
    if (position != m_strokes.end() && position->ordinal == placed.ordinal) {
        return makeError(ErrorCode::InvalidArgument, "another stroke already has that place");
    }
    m_strokes.insert(position, std::move(placed));
    return {};
}

Result<PlacedStroke> Page::remove(const Uuid& strokeId) {
    const auto position = std::ranges::find_if(
        m_strokes, [&](const PlacedStroke& placed) { return placed.stroke.id() == strokeId; });
    if (position == m_strokes.end()) {
        return makeError(ErrorCode::NotFound, "the page does not hold that stroke");
    }
    PlacedStroke removed = std::move(*position);
    m_strokes.erase(position);
    return removed;
}

std::vector<PlacedStroke> Page::takeAll() noexcept {
    return std::exchange(m_strokes, {});
}

}
