#pragma once

#include "core/Error.hpp"
#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/id/Uuid.hpp"
#include "core/model/Color.hpp"
#include "core/model/TextBox.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace phvikapen::core {

// How what is typed in a box sits between the top of the box and its foot.
enum class CellRise : std::uint8_t {
    Top,
    Middle,
    Bottom,
};

// No colour at all, which is what a box says when it is to be shown the way the whole table is:
// nothing behind it, and its words in the table's own colour.
inline constexpr Color kNoColor{.red = 0, .green = 0, .blue = 0, .alpha = 0};

[[nodiscard]] constexpr bool isShown(Color color) noexcept {
    return color.alpha > 0;
}

// One box of a table. What is typed into it belongs to the table; what is written into it with a
// pen belongs to the page and stays where it was written.
//
// A box may reach across several columns and down several rows, and the boxes it swallows are left
// reaching nothing at all: they are covered, and have neither room nor words of their own.
//
// A box may also be shown differently from the rest of the table: filled with a colour behind it,
// its words in a colour of their own, bold, slanted, and standing at the top of its room, at the
// middle or at its foot. What it says nothing about, the table says for it, so that a table is one
// thing unless a reader asks for a heading to stand out.
struct TableCell {
    std::string text;
    TextAlign align{TextAlign::Left};
    int across{1};
    int down{1};
    Color fill{kNoColor};
    Color ink{kNoColor};
    bool bold{};
    bool italic{};
    CellRise rise{CellRise::Top};

    friend bool operator==(const TableCell&, const TableCell&) = default;
};

// Whether a box is shown just as the whole table is, which is what every box starts out as.
[[nodiscard]] constexpr bool isPlain(const TableCell& cell) noexcept {
    return !isShown(cell.fill) && !isShown(cell.ink) && !cell.bold && !cell.italic
           && cell.rise == CellRise::Top && cell.align == TextAlign::Left;
}

[[nodiscard]] constexpr bool isCovered(const TableCell& cell) noexcept {
    return cell.across < 1 || cell.down < 1;
}

// Where a box sits in the grid, counted from the top left.
struct CellAt {
    int row{};
    int column{};

    friend bool operator==(const CellAt&, const CellAt&) = default;
};

// A table standing on a page: where its top left corner is, how wide each column runs, how tall
// each row stands and what is typed in each box. The boxes are kept row by row, so the one at a
// row and a column is the box at row * columns + column.
struct Table {
    static constexpr float kNarrowestColumn = 16.0F;
    static constexpr float kShortestRow = 14.0F;
    static constexpr float kDefaultColumnWidth = 110.0F;
    static constexpr float kDefaultRowHeight = 26.0F;
    static constexpr int kDefaultColumns = 3;
    static constexpr int kDefaultRows = 3;
    static constexpr int kMostColumns = 64;
    static constexpr int kMostRows = 512;
    static constexpr float kDefaultRuleWidth = 1.0F;
    static constexpr float kThinnestRule = 0.25F;
    static constexpr float kThickestRule = 8.0F;
    static constexpr std::uint8_t kDefaultRuleShade = 110;
    static constexpr Color kDefaultRule{
        .red = kDefaultRuleShade,
        .green = kDefaultRuleShade,
        .blue = kDefaultRuleShade,
        .alpha = Color::kOpaque,
    };

    Uuid id;
    Point at{};
    std::vector<float> columns;
    std::vector<float> rows;
    std::vector<TableCell> cells;
    TextStyle style;
    Color rule{kDefaultRule};
    float ruleWidth{kDefaultRuleWidth};

    friend bool operator==(const Table&, const Table&) = default;
};

struct PlacedTable {
    std::int64_t ordinal{};
    Table table;

    friend bool operator==(const PlacedTable&, const PlacedTable&) = default;
};

[[nodiscard]] int columnsOf(const Table& table) noexcept;

[[nodiscard]] int rowsOf(const Table& table) noexcept;

// A table of empty boxes, as many rows and columns as are asked for, within what one may hold.
[[nodiscard]] Table gridOf(int rows, int columns);

[[nodiscard]] float widthOf(const Table& table) noexcept;

[[nodiscard]] float heightOf(const Table& table) noexcept;

[[nodiscard]] Rect areaOf(const Table& table) noexcept;

// The room a box covers, in the units a page is measured in, taking in every column and row it
// reaches. A box the table does not hold, or one that is covered by another, covers nothing.
[[nodiscard]] Rect areaOfCell(const Table& table, CellAt cell) noexcept;

// Which box a point falls in: the one that covers that place, which for a covered cell is the box
// that swallowed it.
[[nodiscard]] std::optional<CellAt> cellUnder(const Table& table, Point at) noexcept;

// The box that covers a place, which is that place itself unless another box reaches over it.
[[nodiscard]] std::optional<CellAt> ownerOf(const Table& table, CellAt cell) noexcept;

[[nodiscard]] const TableCell* cellAt(const Table& table, CellAt cell) noexcept;

[[nodiscard]] Table normalized(Table table);

// A table with one row or one column more. Adding at the end is asking for the place one past the
// last.
[[nodiscard]] Result<Table> withRowAdded(Table table, int at);

[[nodiscard]] Result<Table> withColumnAdded(Table table, int at);

// A table with one row or one column less. The last row and the last column cannot go: a table
// with nothing in it is not a table.
[[nodiscard]] Result<Table> withRowRemoved(Table table, int at);

[[nodiscard]] Result<Table> withColumnRemoved(Table table, int at);

[[nodiscard]] Result<Table> withCellWritten(Table table, CellAt cell, std::string words);

[[nodiscard]] Result<Table> withCellAligned(Table table, CellAt cell, TextAlign align);

// Every box of a stretch of the table changed at once, which is what a reader who has marked out a
// row, a column or a corner of the table is asking for. The stretch is the rectangle the two boxes
// stand at the corners of, whichever way round they are given.
[[nodiscard]] Result<Table> withRangeAligned(Table table, CellAt from, CellAt to, TextAlign align);

[[nodiscard]] Result<Table> withRangeRisen(Table table, CellAt from, CellAt to, CellRise rise);

[[nodiscard]] Result<Table> withRangeFilled(Table table, CellAt from, CellAt to, Color fill);

[[nodiscard]] Result<Table> withRangeInked(Table table, CellAt from, CellAt to, Color ink);

[[nodiscard]] Result<Table> withRangeWeighted(Table table, CellAt from, CellAt to, bool bold);

[[nodiscard]] Result<Table> withRangeSlanted(Table table, CellAt from, CellAt to, bool italic);

// Every box of a stretch left saying what it said, and shown the way the whole table is.
[[nodiscard]] Result<Table> withRangePlain(Table table, CellAt from, CellAt to);

// Every box of a stretch emptied of what was typed in it, and left looking as it did.
[[nodiscard]] Result<Table> withRangeEmptied(Table table, CellAt from, CellAt to);

// A row or a column put down again just after itself, with everything the boxes say and everything
// they are shown in.
[[nodiscard]] Result<Table> withRowDuplicated(Table table, int at);

[[nodiscard]] Result<Table> withColumnDuplicated(Table table, int at);

// Every box of a stretch of the table joined into one, which is the box at its top left. What each
// said is kept, one after another, in reading order. A stretch that cuts through a box already
// reaching over others is widened until it holds the whole of it, so that no box is ever left cut
// in half.
[[nodiscard]] Result<Table> withMergedRange(Table table, CellAt from, CellAt to);

// A box let go of everything it reached over. What it says stays with it, and the boxes it gave
// back are empty.
[[nodiscard]] Result<Table> withCellSplit(Table table, CellAt cell);

// The same table drawn to a given size, every column and every row given the same share of the
// change, so that a table dragged by its corner keeps its proportions.
[[nodiscard]] Table sizedTo(Table table, float width, float height) noexcept;

// The same table with its columns and rows measured out one by one, for a rule pulled about on its
// own. Measures that make no sense are put right, and a table is never left without any.
[[nodiscard]] Table spreadAs(Table table, std::vector<float> columns, std::vector<float> rows);

}
