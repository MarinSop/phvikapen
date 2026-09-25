#include "core/model/Table.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <utility>

namespace phvikapen::core {
namespace {

[[nodiscard]] bool holds(const Table& table, CellAt cell) noexcept {
    return cell.row >= 0 && cell.column >= 0 && cell.row < rowsOf(table)
           && cell.column < columnsOf(table);
}

[[nodiscard]] std::size_t indexOf(const Table& table, CellAt cell) noexcept {
    return (static_cast<std::size_t>(cell.row) * table.columns.size())
           + static_cast<std::size_t>(cell.column);
}

[[nodiscard]] float sumOf(const std::vector<float>& measures) noexcept {
    return std::accumulate(measures.begin(), measures.end(), 0.0F);
}

[[nodiscard]] float edgeBefore(const std::vector<float>& measures, int index) noexcept {
    const auto reached = std::min(static_cast<std::size_t>(std::max(index, 0)), measures.size());
    return std::accumulate(measures.begin(),
                           std::next(measures.begin(), static_cast<std::ptrdiff_t>(reached)), 0.0F);
}

[[nodiscard]] std::optional<int> lineUnder(const std::vector<float>& measures, float from,
                                           float at) noexcept {
    float edge = from;
    for (std::size_t step = 0; step < measures.size(); ++step) {
        const float next = edge + measures[step];
        if (at >= edge && at <= next) {
            return static_cast<int>(step);
        }
        edge = next;
    }
    return std::nullopt;
}

void clampMeasures(std::vector<float>& measures, float plain, float smallest) noexcept {
    for (float& measure : measures) {
        if (!std::isfinite(measure)) {
            measure = plain;
        }
        measure = std::max(measure, smallest);
    }
}

void spreadOver(std::vector<float>& measures, float want, float smallest) noexcept {
    const float have = sumOf(measures);
    if (have <= 0.0F || !std::isfinite(want) || want <= 0.0F) {
        return;
    }
    const float share = want / have;
    for (float& measure : measures) {
        measure = std::max(measure * share, smallest);
    }
}

}

int columnsOf(const Table& table) noexcept {
    return static_cast<int>(table.columns.size());
}

int rowsOf(const Table& table) noexcept {
    return static_cast<int>(table.rows.size());
}

Table gridOf(int rows, int columns) {
    const auto wide = static_cast<std::size_t>(std::clamp(columns, 1, Table::kMostColumns));
    const auto tall = static_cast<std::size_t>(std::clamp(rows, 1, Table::kMostRows));
    Table table;
    table.columns.assign(wide, Table::kDefaultColumnWidth);
    table.rows.assign(tall, Table::kDefaultRowHeight);
    table.cells.resize(wide * tall);
    return table;
}

float widthOf(const Table& table) noexcept {
    return sumOf(table.columns);
}

float heightOf(const Table& table) noexcept {
    return sumOf(table.rows);
}

Rect areaOf(const Table& table) noexcept {
    return Rect{
        .left = table.at.x,
        .top = table.at.y,
        .right = table.at.x + widthOf(table),
        .bottom = table.at.y + heightOf(table),
    };
}

Rect areaOfCell(const Table& table, CellAt cell) noexcept {
    if (!holds(table, cell)) {
        return Rect{};
    }
    const float left = table.at.x + edgeBefore(table.columns, cell.column);
    const float top = table.at.y + edgeBefore(table.rows, cell.row);
    return Rect{
        .left = left,
        .top = top,
        .right = left + table.columns[static_cast<std::size_t>(cell.column)],
        .bottom = top + table.rows[static_cast<std::size_t>(cell.row)],
    };
}

std::optional<CellAt> cellUnder(const Table& table, Point at) noexcept {
    const std::optional<int> column = lineUnder(table.columns, table.at.x, at.x);
    const std::optional<int> row = lineUnder(table.rows, table.at.y, at.y);
    if (!column || !row) {
        return std::nullopt;
    }
    return CellAt{.row = *row, .column = *column};
}

const TableCell* cellAt(const Table& table, CellAt cell) noexcept {
    if (!holds(table, cell)) {
        return nullptr;
    }
    return &table.cells[indexOf(table, cell)];
}

Table normalized(Table table) {
    if (!std::isfinite(table.at.x) || !std::isfinite(table.at.y)) {
        table.at = Point{};
    }
    if (table.columns.empty()) {
        table.columns.assign(1, Table::kDefaultColumnWidth);
    }
    if (table.rows.empty()) {
        table.rows.assign(1, Table::kDefaultRowHeight);
    }
    table.columns.resize(
        std::min(table.columns.size(), static_cast<std::size_t>(Table::kMostColumns)),
        Table::kDefaultColumnWidth);
    table.rows.resize(std::min(table.rows.size(), static_cast<std::size_t>(Table::kMostRows)),
                      Table::kDefaultRowHeight);
    clampMeasures(table.columns, Table::kDefaultColumnWidth, Table::kNarrowestColumn);
    clampMeasures(table.rows, Table::kDefaultRowHeight, Table::kShortestRow);
    table.cells.resize(table.columns.size() * table.rows.size());
    table.style = normalized(std::move(table.style));
    if (!std::isfinite(table.ruleWidth)) {
        table.ruleWidth = Table::kDefaultRuleWidth;
    }
    table.ruleWidth = std::clamp(table.ruleWidth, Table::kThinnestRule, Table::kThickestRule);
    return table;
}

Result<Table> withRowAdded(Table table, int at) {
    const int rows = rowsOf(table);
    if (at < 0 || at > rows) {
        return makeError(ErrorCode::InvalidArgument, "the table has no such row");
    }
    if (rows >= Table::kMostRows) {
        return makeError(ErrorCode::InvalidArgument, "the table cannot hold another row");
    }
    const std::size_t columns = table.columns.size();
    const float height = rows == 0 ? Table::kDefaultRowHeight
                                   : table.rows[static_cast<std::size_t>(std::min(at, rows - 1))];
    table.rows.insert(std::next(table.rows.begin(), static_cast<std::ptrdiff_t>(at)), height);
    table.cells.insert(std::next(table.cells.begin(), static_cast<std::ptrdiff_t>(
                                                          static_cast<std::size_t>(at) * columns)),
                       columns, TableCell{});
    return table;
}

Result<Table> withColumnAdded(Table table, int at) {
    const int columns = columnsOf(table);
    if (at < 0 || at > columns) {
        return makeError(ErrorCode::InvalidArgument, "the table has no such column");
    }
    if (columns >= Table::kMostColumns) {
        return makeError(ErrorCode::InvalidArgument, "the table cannot hold another column");
    }
    const float width = columns == 0
                            ? Table::kDefaultColumnWidth
                            : table.columns[static_cast<std::size_t>(std::min(at, columns - 1))];
    for (int row = rowsOf(table) - 1; row >= 0; --row) {
        const auto where = (static_cast<std::ptrdiff_t>(row) * columns) + at;
        table.cells.insert(std::next(table.cells.begin(), where), TableCell{});
    }
    table.columns.insert(std::next(table.columns.begin(), static_cast<std::ptrdiff_t>(at)), width);
    return table;
}

Result<Table> withRowRemoved(Table table, int at) {
    const int rows = rowsOf(table);
    if (at < 0 || at >= rows) {
        return makeError(ErrorCode::InvalidArgument, "the table has no such row");
    }
    if (rows <= 1) {
        return makeError(ErrorCode::InvalidArgument, "the last row of a table cannot go");
    }
    const auto columns = static_cast<std::ptrdiff_t>(table.columns.size());
    const auto first = std::next(table.cells.begin(), static_cast<std::ptrdiff_t>(at) * columns);
    table.cells.erase(first, std::next(first, columns));
    table.rows.erase(std::next(table.rows.begin(), static_cast<std::ptrdiff_t>(at)));
    return table;
}

Result<Table> withColumnRemoved(Table table, int at) {
    const int columns = columnsOf(table);
    if (at < 0 || at >= columns) {
        return makeError(ErrorCode::InvalidArgument, "the table has no such column");
    }
    if (columns <= 1) {
        return makeError(ErrorCode::InvalidArgument, "the last column of a table cannot go");
    }
    for (int row = rowsOf(table) - 1; row >= 0; --row) {
        const auto where = (static_cast<std::ptrdiff_t>(row) * columns) + at;
        table.cells.erase(std::next(table.cells.begin(), where));
    }
    table.columns.erase(std::next(table.columns.begin(), static_cast<std::ptrdiff_t>(at)));
    return table;
}

Result<Table> withCellWritten(Table table, CellAt cell, std::string words) {
    if (!holds(table, cell)) {
        return makeError(ErrorCode::InvalidArgument, "the table has no such box");
    }
    table.cells[indexOf(table, cell)].text = std::move(words);
    return table;
}

Table sizedTo(Table table, float width, float height) noexcept {
    spreadOver(table.columns, width, Table::kNarrowestColumn);
    spreadOver(table.rows, height, Table::kShortestRow);
    return table;
}

}
