#include "core/model/Table.hpp"

#include "core/geometry/Distance.hpp"
#include "core/geometry/Rect.hpp"
#include "core/model/TextBox.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <utility>

namespace phvikapen::core {
namespace {

[[nodiscard]] Table threeByTwo() {
    Table table = gridOf(3, 2);
    table.at = Point{.x = 100.0F, .y = 50.0F};
    table.columns = {40.0F, 60.0F};
    table.rows = {10.0F, 20.0F, 30.0F};
    return table;
}

TEST(TableTest, MakesAGridOfEmptyBoxes) {
    const Table table = gridOf(4, 3);

    EXPECT_EQ(rowsOf(table), 4);
    EXPECT_EQ(columnsOf(table), 3);
    EXPECT_EQ(table.cells.size(), 12U);
    EXPECT_TRUE(table.cells.front().text.empty());
}

TEST(TableTest, AsksForAtLeastOneRowAndOneColumn) {
    const Table table = gridOf(0, -5);

    EXPECT_EQ(rowsOf(table), 1);
    EXPECT_EQ(columnsOf(table), 1);
}

TEST(TableTest, CoversAsMuchAsItsColumnsAndRowsMeasure) {
    const Table table = threeByTwo();

    EXPECT_FLOAT_EQ(widthOf(table), 100.0F);
    EXPECT_FLOAT_EQ(heightOf(table), 60.0F);
    const Rect area = areaOf(table);
    EXPECT_FLOAT_EQ(area.left, 100.0F);
    EXPECT_FLOAT_EQ(area.top, 50.0F);
    EXPECT_FLOAT_EQ(area.right, 200.0F);
    EXPECT_FLOAT_EQ(area.bottom, 110.0F);
}

TEST(TableTest, PutsEachBoxWhereItsOwnColumnAndRowStand) {
    const Table table = threeByTwo();

    const Rect box = areaOfCell(table, CellAt{.row = 2, .column = 1});

    EXPECT_FLOAT_EQ(box.left, 140.0F);
    EXPECT_FLOAT_EQ(box.top, 80.0F);
    EXPECT_FLOAT_EQ(box.right, 200.0F);
    EXPECT_FLOAT_EQ(box.bottom, 110.0F);
}

TEST(TableTest, GivesNoRoomToABoxThatIsNotThere) {
    const Table table = threeByTwo();

    EXPECT_EQ(areaOfCell(table, CellAt{.row = 3, .column = 0}), Rect{});
    EXPECT_EQ(cellAt(table, CellAt{.row = 0, .column = 2}), nullptr);
}

TEST(TableTest, FindsTheBoxATapLandsIn) {
    const Table table = threeByTwo();

    const std::optional<CellAt> found = cellUnder(table, Point{.x = 150.0F, .y = 85.0F});

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->row, 2);
    EXPECT_EQ(found->column, 1);
}

TEST(TableTest, FindsNothingBeyondItsOwnEdges) {
    const Table table = threeByTwo();

    EXPECT_FALSE(cellUnder(table, Point{.x = 99.0F, .y = 60.0F}).has_value());
    EXPECT_FALSE(cellUnder(table, Point{.x = 150.0F, .y = 111.0F}).has_value());
}

TEST(TableTest, AddsARowAndLeavesWhatWasWrittenWhereItWas) {
    Table table = threeByTwo();
    table = withCellWritten(std::move(table), CellAt{.row = 0, .column = 1}, "top").value();

    const Result<Table> grown = withRowAdded(table, 0);

    ASSERT_TRUE(grown.has_value()) << grown.error().message;
    EXPECT_EQ(rowsOf(*grown), 4);
    EXPECT_EQ(grown->cells.size(), 8U);
    EXPECT_TRUE(cellAt(*grown, CellAt{.row = 0, .column = 1})->text.empty());
    EXPECT_EQ(cellAt(*grown, CellAt{.row = 1, .column = 1})->text, "top");
}

TEST(TableTest, AddsAColumnAndLeavesWhatWasWrittenWhereItWas) {
    Table table = threeByTwo();
    table = withCellWritten(std::move(table), CellAt{.row = 2, .column = 0}, "left").value();

    const Result<Table> grown = withColumnAdded(table, 0);

    ASSERT_TRUE(grown.has_value()) << grown.error().message;
    EXPECT_EQ(columnsOf(*grown), 3);
    EXPECT_EQ(grown->cells.size(), 9U);
    EXPECT_TRUE(cellAt(*grown, CellAt{.row = 2, .column = 0})->text.empty());
    EXPECT_EQ(cellAt(*grown, CellAt{.row = 2, .column = 1})->text, "left");
}

TEST(TableTest, AddsAColumnAtTheEnd) {
    const Result<Table> grown = withColumnAdded(threeByTwo(), 2);

    ASSERT_TRUE(grown.has_value()) << grown.error().message;
    EXPECT_EQ(columnsOf(*grown), 3);
    EXPECT_EQ(grown->cells.size(), 9U);
}

TEST(TableTest, TakesARowAwayWithWhatWasWrittenInIt) {
    Table table = threeByTwo();
    table = withCellWritten(std::move(table), CellAt{.row = 1, .column = 0}, "middle").value();
    table = withCellWritten(std::move(table), CellAt{.row = 2, .column = 0}, "last").value();

    const Result<Table> smaller = withRowRemoved(table, 1);

    ASSERT_TRUE(smaller.has_value()) << smaller.error().message;
    EXPECT_EQ(rowsOf(*smaller), 2);
    EXPECT_EQ(cellAt(*smaller, CellAt{.row = 1, .column = 0})->text, "last");
}

TEST(TableTest, TakesAColumnAwayWithWhatWasWrittenInIt) {
    Table table = threeByTwo();
    table = withCellWritten(std::move(table), CellAt{.row = 0, .column = 0}, "gone").value();
    table = withCellWritten(std::move(table), CellAt{.row = 0, .column = 1}, "kept").value();

    const Result<Table> smaller = withColumnRemoved(table, 0);

    ASSERT_TRUE(smaller.has_value()) << smaller.error().message;
    EXPECT_EQ(columnsOf(*smaller), 1);
    EXPECT_EQ(smaller->cells.size(), 3U);
    EXPECT_EQ(cellAt(*smaller, CellAt{.row = 0, .column = 0})->text, "kept");
    EXPECT_FLOAT_EQ(smaller->columns.front(), 60.0F);
}

TEST(TableTest, KeepsTheLastRowAndTheLastColumn) {
    const Table table = gridOf(1, 1);

    EXPECT_FALSE(withRowRemoved(table, 0).has_value());
    EXPECT_FALSE(withColumnRemoved(table, 0).has_value());
}

TEST(TableTest, RefusesARowOrAColumnThatIsNotThere) {
    const Table table = threeByTwo();

    EXPECT_FALSE(withRowAdded(table, 4).has_value());
    EXPECT_FALSE(withRowRemoved(table, 3).has_value());
    EXPECT_FALSE(withColumnAdded(table, -1).has_value());
    EXPECT_FALSE(withCellWritten(table, CellAt{.row = 9, .column = 0}, "nowhere").has_value());
}

TEST(TableTest, LinesTheWordsOfOneBoxUpOnTheirOwn) {
    const Table table = threeByTwo();

    const Result<Table> lined =
        withCellAligned(table, CellAt{.row = 1, .column = 0}, TextAlign::Right);

    ASSERT_TRUE(lined.has_value()) << lined.error().message;
    EXPECT_EQ(cellAt(*lined, CellAt{.row = 1, .column = 0})->align, TextAlign::Right);
    EXPECT_EQ(cellAt(*lined, CellAt{.row = 0, .column = 0})->align, TextAlign::Left);
    EXPECT_FALSE(withCellAligned(table, CellAt{.row = 9, .column = 0}, TextAlign::Right));
}

TEST(TableTest, MeasuresOutEveryColumnAndRowOnItsOwn) {
    const Table table = spreadAs(threeByTwo(), {70.0F, 30.0F}, {});

    EXPECT_FLOAT_EQ(table.columns.front(), 70.0F);
    EXPECT_FLOAT_EQ(table.columns.back(), 30.0F);
    // The rows were not asked about, so they stand as they were.
    EXPECT_EQ(rowsOf(table), 3);
    EXPECT_EQ(table.rows, normalized(threeByTwo()).rows);
}

TEST(TableTest, MeasuresThatMakeNoSenseArePutRight) {
    const Table table = spreadAs(threeByTwo(), {1.0F, -8.0F}, {});

    EXPECT_FLOAT_EQ(table.columns.front(), Table::kNarrowestColumn);
    EXPECT_FLOAT_EQ(table.columns.back(), Table::kNarrowestColumn);
}

TEST(TableTest, GivesEveryColumnAndRowTheSameShareOfASizeChange) {
    const Table table = sizedTo(threeByTwo(), 200.0F, 120.0F);

    EXPECT_FLOAT_EQ(widthOf(table), 200.0F);
    EXPECT_FLOAT_EQ(heightOf(table), 120.0F);
    EXPECT_FLOAT_EQ(table.columns.front(), 80.0F);
    EXPECT_FLOAT_EQ(table.rows.front(), 20.0F);
}

TEST(TableTest, NeverSqueezesAColumnOrARowOutOfSight) {
    const Table table = sizedTo(threeByTwo(), 1.0F, 1.0F);

    EXPECT_FLOAT_EQ(table.columns.front(), Table::kNarrowestColumn);
    EXPECT_FLOAT_EQ(table.rows.front(), Table::kShortestRow);
}

TEST(TableTest, PutsRightATableThatMakesNoSense) {
    Table table;
    table.columns = {-4.0F};
    table.rows = {};
    table.ruleWidth = 400.0F;

    const Table put = normalized(std::move(table));

    EXPECT_FLOAT_EQ(put.columns.front(), Table::kNarrowestColumn);
    EXPECT_EQ(rowsOf(put), 1);
    EXPECT_EQ(put.cells.size(), 1U);
    EXPECT_FLOAT_EQ(put.ruleWidth, Table::kThickestRule);
}

TEST(TableTest, GivesEveryBoxOfTheGridAPlace) {
    Table table;
    table.columns = {30.0F, 30.0F};
    table.rows = {20.0F, 20.0F};

    const Table put = normalized(std::move(table));

    EXPECT_EQ(put.cells.size(), 4U);
}

}
}
