#include "core/model/StrokeGrid.hpp"

#include "core/geometry/Rect.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace phvikapen::core {
namespace {

constexpr float kMinimumCellSize = 1.0F;
constexpr float kFarthestCell = 1048576.0F;
constexpr unsigned kRowShift = 32U;

[[nodiscard]] std::int32_t cellIndex(float coordinate, float cellSize) noexcept {
    const float index = std::floor(coordinate / cellSize);
    if (std::isnan(index)) {
        return 0;
    }
    return static_cast<std::int32_t>(std::clamp(index, -kFarthestCell, kFarthestCell));
}

}

std::size_t StrokeGrid::CellHash::operator()(const Cell& cell) const noexcept {
    const auto packed =
        (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cell.row)) << kRowShift)
        | static_cast<std::uint32_t>(cell.column);
    return std::hash<std::uint64_t>{}(packed);
}

StrokeGrid::StrokeGrid(float cellSize) noexcept
    : m_cellSize{std::max(cellSize, kMinimumCellSize)} {}

StrokeGrid::CellRange StrokeGrid::cellsCovering(const Rect& area) const noexcept {
    return {
        .first =
            {
                .column = cellIndex(area.left, m_cellSize),
                .row = cellIndex(area.top, m_cellSize),
            },
        .last =
            {
                .column = cellIndex(area.right, m_cellSize),
                .row = cellIndex(area.bottom, m_cellSize),
            },
    };
}

void StrokeGrid::insert(std::int64_t ordinal, const Rect& bounds) {
    const CellRange range = cellsCovering(bounds);
    for (std::int32_t row = range.first.row; row <= range.last.row; ++row) {
        for (std::int32_t column = range.first.column; column <= range.last.column; ++column) {
            m_cells[Cell{.column = column, .row = row}].push_back(ordinal);
        }
    }
}

void StrokeGrid::remove(std::int64_t ordinal, const Rect& bounds) {
    const CellRange range = cellsCovering(bounds);
    for (std::int32_t row = range.first.row; row <= range.last.row; ++row) {
        for (std::int32_t column = range.first.column; column <= range.last.column; ++column) {
            const auto cell = m_cells.find(Cell{.column = column, .row = row});
            if (cell == m_cells.end()) {
                continue;
            }
            std::erase(cell->second, ordinal);
            if (cell->second.empty()) {
                m_cells.erase(cell);
            }
        }
    }
}

void StrokeGrid::clear() noexcept {
    m_cells.clear();
}

std::vector<std::int64_t> StrokeGrid::query(const Rect& area) const {
    std::vector<std::int64_t> ordinals;
    const CellRange range = cellsCovering(area);
    for (std::int32_t row = range.first.row; row <= range.last.row; ++row) {
        for (std::int32_t column = range.first.column; column <= range.last.column; ++column) {
            const auto cell = m_cells.find(Cell{.column = column, .row = row});
            if (cell != m_cells.end()) {
                ordinals.insert(ordinals.end(), cell->second.begin(), cell->second.end());
            }
        }
    }
    std::ranges::sort(ordinals);
    const auto duplicates = std::ranges::unique(ordinals);
    ordinals.erase(duplicates.begin(), duplicates.end());
    return ordinals;
}

}
