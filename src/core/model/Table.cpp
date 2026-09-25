#include "core/model/Table.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <string>
#include <utility>

namespace phvikapen::core {
namespace {

[[nodiscard]] std::size_t indexOf(const Table& table, CellAt cell) noexcept {
    return (static_cast<std::size_t>(cell.row) * table.columns.size())
           + static_cast<std::size_t>(cell.column);
}

// A table whose boxes do not match its measures holds nothing at all, so that one half built is
// asked no questions it cannot answer.
[[nodiscard]] bool holds(const Table& table, CellAt cell) noexcept {
    return cell.row >= 0 && cell.column >= 0 && cell.row < rowsOf(table)
           && cell.column < columnsOf(table) && indexOf(table, cell) < table.cells.size();
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

void sayAsWell(std::string& said, std::string& more) {
    if (more.empty()) {
        return;
    }
    if (!said.empty()) {
        said.push_back(' ');
    }
    said.append(more);
    more.clear();
}

// Every place one box reaches over marked as covered, and whatever those places said handed to the
// box that swallowed them, so that nothing is lost to a join.
void coverUnder(Table& table, std::size_t here, CellAt at, std::vector<bool>& covered) {
    const auto wide = static_cast<int>(table.columns.size());
    const int across = table.cells[here].across;
    const int down = table.cells[here].down;
    for (int downTo = at.row; downTo < at.row + down; ++downTo) {
        for (int acrossTo = at.column; acrossTo < at.column + across; ++acrossTo) {
            const auto under = (static_cast<std::size_t>(downTo) * static_cast<std::size_t>(wide))
                               + static_cast<std::size_t>(acrossTo);
            if (under == here) {
                continue;
            }
            covered[under] = true;
            std::string taken = std::move(table.cells[under].text);
            table.cells[under].text.clear();
            sayAsWell(table.cells[here].text, taken);
        }
    }
}

// Every box put back in agreement with the rest: a box reaches no further than the table allows,
// and the boxes it reaches over are covered.
void mendSpans(Table& table) {
    const auto wide = static_cast<int>(table.columns.size());
    const auto tall = static_cast<int>(table.rows.size());
    std::vector<bool> covered(table.cells.size(), false);
    for (int row = 0; row < tall; ++row) {
        for (int column = 0; column < wide; ++column) {
            const auto here = (static_cast<std::size_t>(row) * static_cast<std::size_t>(wide))
                              + static_cast<std::size_t>(column);
            if (covered[here]) {
                table.cells[here].across = 0;
                table.cells[here].down = 0;
                continue;
            }
            table.cells[here].across = std::clamp(table.cells[here].across, 1, wide - column);
            table.cells[here].down = std::clamp(table.cells[here].down, 1, tall - row);
            coverUnder(table, here, CellAt{.row = row, .column = column}, covered);
        }
    }
}

// Every box of the rectangle two boxes stand at the corners of, changed one at a time.
template <typename Change>
[[nodiscard]] Result<Table> overEachOf(Table table, CellAt from, CellAt to, Change change) {
    if (!holds(table, from) || !holds(table, to)) {
        return makeError(ErrorCode::InvalidArgument, "the table has no such box");
    }
    const int firstRow = std::min(from.row, to.row);
    const int lastRow = std::max(from.row, to.row);
    const int firstColumn = std::min(from.column, to.column);
    const int lastColumn = std::max(from.column, to.column);
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            change(table.cells[indexOf(table, CellAt{.row = row, .column = column})]);
        }
    }
    return table;
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
    const TableCell& said = table.cells[indexOf(table, cell)];
    if (isCovered(said)) {
        return Rect{};
    }
    const float left = table.at.x + edgeBefore(table.columns, cell.column);
    const float top = table.at.y + edgeBefore(table.rows, cell.row);
    const float right = table.at.x + edgeBefore(table.columns, cell.column + said.across);
    const float bottom = table.at.y + edgeBefore(table.rows, cell.row + said.down);
    return Rect{
        .left = left,
        .top = top,
        .right = right,
        .bottom = bottom,
    };
}

std::optional<CellAt> ownerOf(const Table& table, CellAt cell) noexcept {
    if (!holds(table, cell)) {
        return std::nullopt;
    }
    if (!isCovered(table.cells[indexOf(table, cell)])) {
        return cell;
    }
    for (int row = cell.row; row >= 0; --row) {
        for (int column = cell.column; column >= 0; --column) {
            const TableCell& said =
                table.cells[indexOf(table, CellAt{.row = row, .column = column})];
            if (isCovered(said)) {
                continue;
            }
            if (cell.row < row + said.down && cell.column < column + said.across) {
                return CellAt{.row = row, .column = column};
            }
        }
    }
    return std::nullopt;
}

std::optional<CellAt> cellUnder(const Table& table, Point at) noexcept {
    const std::optional<int> column = lineUnder(table.columns, table.at.x, at.x);
    const std::optional<int> row = lineUnder(table.rows, table.at.y, at.y);
    if (!column || !row) {
        return std::nullopt;
    }
    return ownerOf(table, CellAt{.row = *row, .column = *column});
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
    mendSpans(table);
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

Result<Table> withCellAligned(Table table, CellAt cell, TextAlign align) {
    return withRangeAligned(std::move(table), cell, cell, align);
}

Result<Table> withRangeAligned(Table table, CellAt from, CellAt to, TextAlign align) {
    return overEachOf(std::move(table), from, to, [align](TableCell& cell) { cell.align = align; });
}

Result<Table> withRangeRisen(Table table, CellAt from, CellAt to, CellRise rise) {
    return overEachOf(std::move(table), from, to, [rise](TableCell& cell) { cell.rise = rise; });
}

Result<Table> withRangeFilled(Table table, CellAt from, CellAt to, Color fill) {
    return overEachOf(std::move(table), from, to, [fill](TableCell& cell) { cell.fill = fill; });
}

Result<Table> withRangeInked(Table table, CellAt from, CellAt to, Color ink) {
    return overEachOf(std::move(table), from, to, [ink](TableCell& cell) { cell.ink = ink; });
}

Result<Table> withRangeWeighted(Table table, CellAt from, CellAt to, bool bold) {
    return overEachOf(std::move(table), from, to, [bold](TableCell& cell) { cell.bold = bold; });
}

Result<Table> withRangeSlanted(Table table, CellAt from, CellAt to, bool italic) {
    return overEachOf(std::move(table), from, to,
                      [italic](TableCell& cell) { cell.italic = italic; });
}

Result<Table> withRangePlain(Table table, CellAt from, CellAt to) {
    return overEachOf(std::move(table), from, to, [](TableCell& cell) {
        cell.align = TextAlign::Left;
        cell.fill = kNoColor;
        cell.ink = kNoColor;
        cell.bold = false;
        cell.italic = false;
        cell.rise = CellRise::Top;
    });
}

Result<Table> withRangeEmptied(Table table, CellAt from, CellAt to) {
    return overEachOf(std::move(table), from, to, [](TableCell& cell) { cell.text.clear(); });
}

Result<Table> withRowDuplicated(Table table, int at) {
    const int rows = rowsOf(table);
    if (at < 0 || at >= rows) {
        return makeError(ErrorCode::InvalidArgument, "the table has no such row");
    }
    const std::size_t columns = table.columns.size();
    Result<Table> grown = withRowAdded(std::move(table), at + 1);
    if (!grown) {
        return grown;
    }
    Table made = std::move(*grown);
    const auto from = static_cast<std::size_t>(at) * columns;
    for (std::size_t step = 0; step < columns; ++step) {
        made.cells[from + columns + step] = made.cells[from + step];
    }
    made.rows[static_cast<std::size_t>(at) + 1] = made.rows[static_cast<std::size_t>(at)];
    return normalized(std::move(made));
}

Result<Table> withColumnDuplicated(Table table, int at) {
    const int columns = columnsOf(table);
    if (at < 0 || at >= columns) {
        return makeError(ErrorCode::InvalidArgument, "the table has no such column");
    }
    Result<Table> grown = withColumnAdded(std::move(table), at + 1);
    if (!grown) {
        return grown;
    }
    Table made = std::move(*grown);
    const std::size_t wide = made.columns.size();
    for (int row = 0; row < rowsOf(made); ++row) {
        const std::size_t here =
            (static_cast<std::size_t>(row) * wide) + static_cast<std::size_t>(at);
        made.cells[here + 1] = made.cells[here];
    }
    made.columns[static_cast<std::size_t>(at) + 1] = made.columns[static_cast<std::size_t>(at)];
    return normalized(std::move(made));
}

Table spreadAs(Table table, std::vector<float> columns, std::vector<float> rows) {
    if (!columns.empty()) {
        table.columns = std::move(columns);
    }
    if (!rows.empty()) {
        table.rows = std::move(rows);
    }
    return normalized(std::move(table));
}

namespace {

struct Stretch {
    int fromRow{};
    int fromColumn{};
    int toRow{};
    int toColumn{};
};

// The stretch widened until it holds the whole of every box that reaches into it, so that a box
// already joined is never left cut in half.
[[nodiscard]] Stretch widened(const Table& table, Stretch stretch) {
    const int wide = columnsOf(table);
    const int tall = rowsOf(table);
    bool grew = true;
    while (grew) {
        grew = false;
        for (int row = 0; row < tall; ++row) {
            for (int column = 0; column < wide; ++column) {
                const TableCell& said =
                    table.cells[indexOf(table, CellAt{.row = row, .column = column})];
                if (isCovered(said)) {
                    continue;
                }
                const int lastRow = row + said.down - 1;
                const int lastColumn = column + said.across - 1;
                const bool touches = row <= stretch.toRow && lastRow >= stretch.fromRow
                                     && column <= stretch.toColumn
                                     && lastColumn >= stretch.fromColumn;
                if (!touches) {
                    continue;
                }
                const Stretch was = stretch;
                stretch.fromRow = std::min(stretch.fromRow, row);
                stretch.fromColumn = std::min(stretch.fromColumn, column);
                stretch.toRow = std::max(stretch.toRow, lastRow);
                stretch.toColumn = std::max(stretch.toColumn, lastColumn);
                grew = grew || stretch.fromRow != was.fromRow
                       || stretch.fromColumn != was.fromColumn || stretch.toRow != was.toRow
                       || stretch.toColumn != was.toColumn;
            }
        }
    }
    return stretch;
}

}

Result<Table> withMergedRange(Table table, CellAt from, CellAt to) {
    if (!holds(table, from) || !holds(table, to)) {
        return makeError(ErrorCode::InvalidArgument, "the table has no such box");
    }
    const Stretch stretch = widened(table, Stretch{
                                               .fromRow = std::min(from.row, to.row),
                                               .fromColumn = std::min(from.column, to.column),
                                               .toRow = std::max(from.row, to.row),
                                               .toColumn = std::max(from.column, to.column),
                                           });
    if (stretch.fromRow == stretch.toRow && stretch.fromColumn == stretch.toColumn) {
        return makeError(ErrorCode::InvalidArgument, "one box on its own cannot be joined");
    }

    const CellAt head{.row = stretch.fromRow, .column = stretch.fromColumn};
    std::string said;
    for (int row = stretch.fromRow; row <= stretch.toRow; ++row) {
        for (int column = stretch.fromColumn; column <= stretch.toColumn; ++column) {
            sayAsWell(said, table.cells[indexOf(table, CellAt{.row = row, .column = column})].text);
        }
    }

    TableCell& owner = table.cells[indexOf(table, head)];
    owner.text = std::move(said);
    owner.across = stretch.toColumn - stretch.fromColumn + 1;
    owner.down = stretch.toRow - stretch.fromRow + 1;
    return normalized(std::move(table));
}

Result<Table> withCellSplit(Table table, CellAt cell) {
    if (!holds(table, cell)) {
        return makeError(ErrorCode::InvalidArgument, "the table has no such box");
    }
    TableCell& owner = table.cells[indexOf(table, cell)];
    if (isCovered(owner)) {
        return makeError(ErrorCode::InvalidArgument, "a box that is covered cannot be let go of");
    }
    if (owner.across <= 1 && owner.down <= 1) {
        return makeError(ErrorCode::InvalidArgument, "that box reaches over nothing");
    }
    const int across = owner.across;
    const int down = owner.down;
    owner.across = 1;
    owner.down = 1;
    for (int row = cell.row; row < cell.row + down; ++row) {
        for (int column = cell.column; column < cell.column + across; ++column) {
            if (row == cell.row && column == cell.column) {
                continue;
            }
            table.cells[indexOf(table, CellAt{.row = row, .column = column})] =
                TableCell{.text = {}, .align = TextAlign::Left, .across = 1, .down = 1};
        }
    }
    return normalized(std::move(table));
}

Table sizedTo(Table table, float width, float height) noexcept {
    spreadOver(table.columns, width, Table::kNarrowestColumn);
    spreadOver(table.rows, height, Table::kShortestRow);
    return table;
}

}
