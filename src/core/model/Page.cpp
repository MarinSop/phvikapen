#include "core/model/Page.hpp"

#include "core/Error.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/ink/StrokeHitTest.hpp"
#include "core/model/Layer.hpp"
#include "core/model/Table.hpp"
#include "core/model/TextBox.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace phvikapen::core {

Page::Page(const Uuid& id) noexcept : m_id{id} {}

Page::Page(const Uuid& id, std::vector<PlacedStroke> strokes, std::vector<PlacedText> texts,
           std::vector<PlacedPicture> pictures, std::vector<PlacedTable> tables,
           std::vector<Layer> layers)
    : m_id{id}, m_strokes{std::move(strokes)}, m_texts{std::move(texts)},
      m_pictures{std::move(pictures)}, m_tables{std::move(tables)}, m_layers{std::move(layers)} {
    setLayers(std::move(m_layers));
    std::ranges::stable_sort(m_strokes, {}, &PlacedStroke::ordinal);
    std::ranges::stable_sort(m_texts, {}, &PlacedText::ordinal);
    std::ranges::stable_sort(m_pictures, {}, &PlacedPicture::ordinal);
    std::ranges::stable_sort(m_tables, {}, &PlacedTable::ordinal);
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

namespace {

// Nothing on a layer that is hidden or locked can be taken hold of: it is either not there to be
// seen, or it is being kept still on purpose.
[[nodiscard]] bool canBeTakenHold(std::span<const Layer> layers, const Uuid& stands) noexcept {
    const Layer* const found = layerOf(layers, stands);
    return found == nullptr || isOpenToTheHand(*found);
}

}

const TextBox* Page::textUnder(Point at) const noexcept {
    for (const PlacedText& placed : std::ranges::reverse_view{m_texts}) {
        const Rect area = areaOf(placed.box);
        if (at.x >= area.left && at.x <= area.right && at.y >= area.top && at.y <= area.bottom
            && canBeTakenHold(m_layers, placed.layer)) {
            return &placed.box;
        }
    }
    return nullptr;
}

std::vector<PlacedPicture> Page::takeAllPictures() noexcept {
    return std::exchange(m_pictures, {});
}

std::int64_t Page::nextTableOrdinal() const noexcept {
    return m_tables.empty() ? 0 : m_tables.back().ordinal + 1;
}

Result<void> Page::insertTable(PlacedTable placed) {
    const bool idTaken = std::ranges::any_of(m_tables, [&](const PlacedTable& existing) {
        return existing.table.id == placed.table.id;
    });
    if (idTaken) {
        return makeError(ErrorCode::InvalidArgument, "the page already holds that table");
    }

    const auto position =
        std::ranges::lower_bound(m_tables, placed.ordinal, {}, &PlacedTable::ordinal);
    if (position != m_tables.end() && position->ordinal == placed.ordinal) {
        return makeError(ErrorCode::InvalidArgument, "another table already has that place");
    }
    m_tables.insert(position, std::move(placed));
    return {};
}

Result<PlacedTable> Page::removeTable(const Uuid& tableId) {
    const auto position = std::ranges::find_if(
        m_tables, [&](const PlacedTable& placed) { return placed.table.id == tableId; });
    if (position == m_tables.end()) {
        return makeError(ErrorCode::NotFound, "the page does not hold that table");
    }
    PlacedTable removed = std::move(*position);
    m_tables.erase(position);
    return removed;
}

Result<void> Page::replaceTable(Table table) {
    const auto position = std::ranges::find_if(
        m_tables, [&](const PlacedTable& placed) { return placed.table.id == table.id; });
    if (position == m_tables.end()) {
        return makeError(ErrorCode::NotFound, "the page does not hold that table");
    }
    position->table = std::move(table);
    return {};
}

const Table* Page::tableAt(const Uuid& tableId) const noexcept {
    const auto position = std::ranges::find_if(
        m_tables, [&](const PlacedTable& placed) { return placed.table.id == tableId; });
    return position == m_tables.end() ? nullptr : &position->table;
}

const Table* Page::tableUnder(Point at) const noexcept {
    for (const PlacedTable& placed : std::ranges::reverse_view{m_tables}) {
        const Rect area = areaOf(placed.table);
        if (at.x >= area.left && at.x <= area.right && at.y >= area.top && at.y <= area.bottom
            && canBeTakenHold(m_layers, placed.layer)) {
            return &placed.table;
        }
    }
    return nullptr;
}

std::vector<PlacedTable> Page::takeAllTables() noexcept {
    return std::exchange(m_tables, {});
}

std::int64_t Page::nextPictureOrdinal() const noexcept {
    return m_pictures.empty() ? 0 : m_pictures.back().ordinal + 1;
}

Result<void> Page::insertPicture(PlacedPicture placed) {
    const bool idTaken = std::ranges::any_of(m_pictures, [&](const PlacedPicture& existing) {
        return existing.picture.id == placed.picture.id;
    });
    if (idTaken) {
        return makeError(ErrorCode::InvalidArgument, "the page already holds that picture");
    }

    const auto position =
        std::ranges::lower_bound(m_pictures, placed.ordinal, {}, &PlacedPicture::ordinal);
    if (position != m_pictures.end() && position->ordinal == placed.ordinal) {
        return makeError(ErrorCode::InvalidArgument, "another picture already has that place");
    }
    m_pictures.insert(position, placed);
    return {};
}

Result<PlacedPicture> Page::removePicture(const Uuid& pictureId) {
    const auto position = std::ranges::find_if(
        m_pictures, [&](const PlacedPicture& placed) { return placed.picture.id == pictureId; });
    if (position == m_pictures.end()) {
        return makeError(ErrorCode::NotFound, "the page does not hold that picture");
    }
    const PlacedPicture removed = *position;
    m_pictures.erase(position);
    return removed;
}

Result<void> Page::replacePicture(Picture picture) {
    const auto position = std::ranges::find_if(
        m_pictures, [&](const PlacedPicture& placed) { return placed.picture.id == picture.id; });
    if (position == m_pictures.end()) {
        return makeError(ErrorCode::NotFound, "the page does not hold that picture");
    }
    position->picture = picture;
    return {};
}

const Picture* Page::pictureAt(const Uuid& pictureId) const noexcept {
    const auto position = std::ranges::find_if(
        m_pictures, [&](const PlacedPicture& placed) { return placed.picture.id == pictureId; });
    return position == m_pictures.end() ? nullptr : &position->picture;
}

const Picture* Page::pictureUnder(Point at) const noexcept {
    for (const PlacedPicture& placed : std::ranges::reverse_view{m_pictures}) {
        const Rect area = areaOf(placed.picture);
        if (at.x >= area.left && at.x <= area.right && at.y >= area.top && at.y <= area.bottom
            && canBeTakenHold(m_layers, placed.layer)) {
            return &placed.picture;
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

void Page::setLayers(std::vector<Layer> layers) {
    m_layers = std::move(layers);
    if (m_layers.size() > Layer::kMostLayers) {
        m_layers.resize(Layer::kMostLayers);
    }
    // A page always has a layer, so that everything on it has somewhere to stand, whether it was
    // written down before there were layers or after every one of them was taken away.
    if (m_layers.empty()) {
        m_layers.push_back(Layer{.id = m_id, .name = "Layer 1", .shown = true, .locked = false});
    }
}

Result<Uuid> Page::moveToLayer(const Uuid& thingId, const Uuid& layerId) {
    const auto moved = [&layerId](Uuid& layer) {
        const Uuid stood = layer;
        layer = layerId;
        return stood;
    };
    for (PlacedStroke& placed : m_strokes) {
        if (placed.stroke.id() == thingId) {
            return moved(placed.layer);
        }
    }
    for (PlacedText& placed : m_texts) {
        if (placed.box.id == thingId) {
            return moved(placed.layer);
        }
    }
    for (PlacedPicture& placed : m_pictures) {
        if (placed.picture.id == thingId) {
            return moved(placed.layer);
        }
    }
    for (PlacedTable& placed : m_tables) {
        if (placed.table.id == thingId) {
            return moved(placed.layer);
        }
    }
    return makeError(ErrorCode::NotFound, "the page has nothing of that name on it");
}

int Page::countOnLayer(const Uuid& layerId) const noexcept {
    int found = 0;
    const auto count = [&](const Uuid& layer) {
        const Layer* const stands = layerOf(m_layers, layer);
        if (stands != nullptr && stands->id == layerId) {
            ++found;
        }
    };
    for (const PlacedStroke& placed : m_strokes) {
        count(placed.layer);
    }
    for (const PlacedText& placed : m_texts) {
        count(placed.layer);
    }
    for (const PlacedPicture& placed : m_pictures) {
        count(placed.layer);
    }
    for (const PlacedTable& placed : m_tables) {
        count(placed.layer);
    }
    return found;
}

}
