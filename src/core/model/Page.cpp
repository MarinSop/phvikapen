#include "core/model/Page.hpp"

#include "core/Error.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/StrokeHitTest.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace phvikapen::core {

Page::Page(const Uuid& id) noexcept : m_id{id} {}

Page::Page(const Uuid& id, std::vector<PlacedStroke> strokes)
    : m_id{id}, m_strokes{std::move(strokes)} {
    std::ranges::stable_sort(m_strokes, {}, &PlacedStroke::ordinal);
    for (const PlacedStroke& placed : m_strokes) {
        if (const std::optional<Rect> bounds = placed.stroke.boundingBox()) {
            m_grid.insert(placed.ordinal, *bounds);
        }
    }
}

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
    if (const std::optional<Rect> bounds = placed.stroke.boundingBox()) {
        m_grid.insert(placed.ordinal, *bounds);
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
    if (const std::optional<Rect> bounds = position->stroke.boundingBox()) {
        m_grid.remove(position->ordinal, *bounds);
    }
    PlacedStroke removed = std::move(*position);
    m_strokes.erase(position);
    return removed;
}

std::vector<PlacedStroke> Page::takeAll() noexcept {
    m_grid.clear();
    return std::exchange(m_strokes, {});
}

std::vector<Uuid> Page::strokesTouchedBy(const EraserSweep& sweep) const {
    std::vector<Uuid> touched;
    for (const std::int64_t ordinal : m_grid.query(sweep.bounds())) {
        const auto position =
            std::ranges::lower_bound(m_strokes, ordinal, {}, &PlacedStroke::ordinal);
        if (position != m_strokes.end() && position->ordinal == ordinal
            && touches(position->stroke, sweep)) {
            touched.push_back(position->stroke.id());
        }
    }
    return touched;
}

}
