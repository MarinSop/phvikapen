#include "core/model/Page.hpp"

#include "core/Error.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/StrokeHitTest.hpp"
#include "core/model/TextBox.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

namespace phvikapen::core {

Page::Page(const Uuid& id) noexcept : m_id{id} {}

Page::Page(const Uuid& id, std::vector<PlacedStroke> strokes, std::vector<PlacedText> texts)
    : m_id{id}, m_strokes{std::move(strokes)}, m_texts{std::move(texts)} {
    std::ranges::stable_sort(m_strokes, {}, &PlacedStroke::ordinal);
    std::ranges::stable_sort(m_texts, {}, &PlacedText::ordinal);
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

std::vector<PlacedText> Page::takeAllTexts() noexcept {
    return std::exchange(m_texts, {});
}

std::int64_t Page::nextTextOrdinal() const noexcept {
    return m_texts.empty() ? 0 : m_texts.back().ordinal + 1;
}

Result<void> Page::insertText(PlacedText placed) {
    const bool idTaken = std::ranges::any_of(
        m_texts, [&](const PlacedText& existing) { return existing.box.id == placed.box.id; });
    if (idTaken) {
        return makeError(ErrorCode::InvalidArgument, "the page already holds that text");
    }

    const auto position =
        std::ranges::lower_bound(m_texts, placed.ordinal, {}, &PlacedText::ordinal);
    if (position != m_texts.end() && position->ordinal == placed.ordinal) {
        return makeError(ErrorCode::InvalidArgument, "another text already has that place");
    }
    m_texts.insert(position, std::move(placed));
    return {};
}

Result<PlacedText> Page::removeText(const Uuid& textId) {
    const auto position = std::ranges::find_if(
        m_texts, [&](const PlacedText& placed) { return placed.box.id == textId; });
    if (position == m_texts.end()) {
        return makeError(ErrorCode::NotFound, "the page does not hold that text");
    }
    PlacedText removed = std::move(*position);
    m_texts.erase(position);
    return removed;
}

Result<void> Page::replaceText(TextBox box) {
    const auto position = std::ranges::find_if(
        m_texts, [&](const PlacedText& placed) { return placed.box.id == box.id; });
    if (position == m_texts.end()) {
        return makeError(ErrorCode::NotFound, "the page does not hold that text");
    }
    position->box = std::move(box);
    return {};
}

const TextBox* Page::textAt(const Uuid& textId) const noexcept {
    const auto position = std::ranges::find_if(
        m_texts, [&](const PlacedText& placed) { return placed.box.id == textId; });
    return position == m_texts.end() ? nullptr : &position->box;
}

const TextBox* Page::textUnder(Point at) const noexcept {
    for (const PlacedText& placed : std::ranges::reverse_view{m_texts}) {
        const Rect area = areaOf(placed.box);
        if (at.x >= area.left && at.x <= area.right && at.y >= area.top && at.y <= area.bottom) {
            return &placed.box;
        }
    }
    return nullptr;
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
