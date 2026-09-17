#pragma once

#include "core/geometry/Rect.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace phvikapen::core {

class StrokeGrid {
public:
    static constexpr float kDefaultCellSize = 256.0F;

    explicit StrokeGrid(float cellSize = kDefaultCellSize) noexcept;

    void insert(std::int64_t ordinal, const Rect& bounds);
    void remove(std::int64_t ordinal, const Rect& bounds);
    void clear() noexcept;

    [[nodiscard]] std::vector<std::int64_t> query(const Rect& area) const;

private:
    struct Cell {
        std::int32_t column{};
        std::int32_t row{};

        friend constexpr bool operator==(const Cell&, const Cell&) = default;
    };

    struct CellHash {
        [[nodiscard]] std::size_t operator()(const Cell& cell) const noexcept;
    };

    struct CellRange {
        Cell first;
        Cell last;
    };

    [[nodiscard]] CellRange cellsCovering(const Rect& area) const noexcept;

    float m_cellSize;
    std::unordered_map<Cell, std::vector<std::int64_t>, CellHash> m_cells;
};

}
