import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    property int notebookCount: 0

    function newNotebookPath() {
        notebookCount += 1;
        return temporaryDirectory + "/tablelayer-" + notebookCount + ".phvika";
    }

    function openLayer(notebook, tools) {
        return createTemporaryObject(layerComponent, testCase, {
            canvas: notebook.canvas,
            notebook: notebook,
            tools: tools
        });
    }

    function openNotebook(path) {
        const canvas = createTemporaryObject(canvasComponent, testCase);
        const notebook = createTemporaryObject(notebookComponent, testCase, {
            canvas: canvas,
            notebookPath: path
        });
        tryCompare(notebook, "loaded", true);
        return notebook;
    }

    function pickingTools() {
        const tools = createTemporaryObject(toolsComponent, testCase);
        tools.currentTool = ToolViewModel.Selection;
        return tools;
    }

    function tables(notebook) {
        return createTemporaryObject(rowsComponent, testCase, {
            model: notebook.tables
        });
    }

    function test_theKnobThatCarriesATableStandsClearOfTheGripThatSizesIt() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);
        tryCompare(layer, "holding", true);

        const knob = findChild(layer, "tableMoveGrip");
        const grip = findChild(layer, "tableGrip0");

        verify(knob !== null);
        verify(grip !== null);
        // Two controls on the same spot mean the one drawn last takes every press, and the table
        // could never be carried at all.
        const apart = knob.x + knob.width <= grip.x || grip.x + grip.width <= knob.x || knob.y + knob.height <= grip.y || grip.y + grip.height <= knob.y;
        verify(apart, "the knob and the corner grip stand on the same spot");
    }

    function test_aTableBeingCarriedFollowsThePointerBeforeItIsSetDown() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);
        tryCompare(layer, "holding", true);
        const drawn = findChild(layer, "tableBox");
        verify(drawn !== null);
        const before = drawn.x;

        layer.liveX = 40;

        compare(drawn.x, before + 40);
    }

    function test_theStripDownTheSideMarksAWholeRow() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(3, 2);
        tryCompare(layer, "holding", true);

        layer.markRow(1);

        verify(layer.marking);
        compare(layer.firstRow, 1);
        compare(layer.lastRow, 1);
        compare(layer.firstColumn, 0);
        compare(layer.lastColumn, 1);
    }

    function test_theStripAboveMarksAWholeColumn() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(3, 2);
        tryCompare(layer, "holding", true);

        layer.markColumn(1);

        verify(layer.marking);
        compare(layer.firstColumn, 1);
        compare(layer.lastColumn, 1);
        compare(layer.firstRow, 0);
        compare(layer.lastRow, 2);
    }

    function test_theKnobOnItsOwnMarksTheWholeTable() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(3, 2);
        tryCompare(layer, "holding", true);

        layer.markAll();

        verify(layer.marking);
        compare(layer.firstRow, 0);
        compare(layer.firstColumn, 0);
        compare(layer.lastRow, 2);
        compare(layer.lastColumn, 1);
    }

    function test_aStretchOfBoxesIsGivenAColourAndAFaceOfItsOwn() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        openLayer(notebook, tools);
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;

        notebook.fillCells(tableId, 0, 0, 0, 1, "#ffcc00");
        notebook.weighCells(tableId, 0, 0, 0, 1, true);
        notebook.riseCells(tableId, 1, 0, 1, 1, 1);

        const rows = tables(notebook);
        compare(rows.count, 1);
        const shown = rows.itemAt(0);
        compare(shown.bolds[0], true);
        compare(shown.bolds[1], true);
        compare(shown.bolds[2], false);
        compare(shown.fills[0].toString(), "#ffcc00");
        // A box that asked for no colour must say so with nothing in it: a colour the window
        // cannot read is shown as black, and the box would be filled in with it.
        compare(shown.fills[3].a, 0);
        compare(shown.inks[0].a, 0);
        compare(shown.rises[2], 1);
        compare(shown.rises[0], 0);
    }

    function test_aBoxWithNoColourOfItsOwnSaysSoWithNothingInIt() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        openLayer(notebook, tools);
        notebook.addTable(2, 2);

        const look = notebook.cellLook(notebook.pickedTable, 0, 0);

        compare(look.fill.a, 0);
        compare(look.ink.a, 0);
    }

    function test_whatABoxIsShownInIsAskedForWhenTheBarNeedsIt() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        openLayer(notebook, tools);
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;
        notebook.weighCells(tableId, 0, 0, 0, 0, true);

        const look = notebook.cellLook(tableId, 0, 0);

        compare(look.bold, true);
        compare(look.italic, false);
        compare(look.align, 0);
        compare(notebook.cellLook(tableId, 9, 9).bold, undefined);
    }

    function test_rubbingOutAStretchLeavesTheBoxesLookingAsTheyDid() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        openLayer(notebook, tools);
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;
        notebook.writeCell(tableId, 0, 0, "Monday");
        notebook.weighCells(tableId, 0, 0, 0, 0, true);

        notebook.emptyCells(tableId, 0, 0, 0, 1);

        compare(notebook.wordsOfCell(tableId, 0, 0), "");
        compare(notebook.cellLook(tableId, 0, 0).bold, true);
    }

    function test_aRowPutDownAgainBringsItsWordsWithIt() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        openLayer(notebook, tools);
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;
        notebook.writeCell(tableId, 0, 1, "Monday");

        notebook.duplicateRow(tableId, 0);

        compare(notebook.pickedTableBox.heights.length, 3);
        compare(notebook.wordsOfCell(tableId, 1, 1), "Monday");
    }

    function test_aTableRuledOnThePageIsDrawnWithItsBoxes() {
        const notebook = openNotebook(newNotebookPath());

        notebook.addTable(2, 3);

        const rows = tables(notebook);
        compare(rows.count, 1);
        compare(rows.itemAt(0).widths.length, 3);
        compare(rows.itemAt(0).heights.length, 2);
        compare(rows.itemAt(0).words.length, 6);
    }

    function test_aTableIsKeptAndComesBackWhenTheNotebookIsOpenedAgain() {
        const path = newNotebookPath();
        const notebook = openNotebook(path);
        notebook.addTable(2, 2);
        notebook.writeCell(notebook.pickedTable, 0, 1, "Monday");
        compare(notebook.errorMessage, "");

        notebook.destroy();
        wait(0);

        const reopened = openNotebook(path);
        const rows = tables(reopened);
        compare(rows.count, 1);
        compare(rows.itemAt(0).words[1], "Monday");
    }

    function test_aTapOpensTheBoxItLandsInAndTypingIsKept() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;
        const box = notebook.pickedTableBox;

        layer.openCellAt(tableId, Qt.point(box.columnX + box.widths[0] + 2, box.columnY + box.heights[0] + 2));

        compare(layer.editingRow, 1);
        compare(layer.editingColumn, 1);

        layer.leave();

        compare(notebook.wordsOfCell(tableId, 1, 1), "");
        compare(layer.editingRow, -1);
    }

    function test_aTapBeyondTheTableOpensNothing() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);
        const box = notebook.pickedTableBox;

        layer.openCellAt(notebook.pickedTable, Qt.point(box.columnX - 20, box.columnY - 20));

        compare(layer.editingRow, -1);
        compare(layer.editingColumn, -1);
    }

    function test_theNextBoxAlongIsReachedWithoutLeavingTheTable() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);

        layer.openCell(notebook.pickedTable, 0, 0);
        layer.stepOn(1);

        compare(layer.editingRow, 0);
        compare(layer.editingColumn, 1);

        layer.stepOn(1);

        compare(layer.editingRow, 1);
        compare(layer.editingColumn, 0);

        layer.stepOn(-1);

        compare(layer.editingRow, 0);
        compare(layer.editingColumn, 1);
    }

    function test_theBoxBeforeTheFirstIsNeverOpened() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(1, 2);

        layer.openCell(notebook.pickedTable, 0, 0);
        layer.stepOn(-1);

        compare(layer.editingRow, 0);
        compare(layer.editingColumn, 0);
        compare(notebook.pickedTableBox.heights.length, 1);
    }

    function test_reachingForAnotherToolLetsGoOfTheTable() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);
        layer.openCell(notebook.pickedTable, 0, 0);

        tools.currentTool = ToolViewModel.Pen;

        compare(layer.editingRow, -1);
        compare(notebook.pickedTable, "");
    }

    function test_theBoxBeingTypedInIsNotDrawnTwice() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);

        compare(layer.editingCell, -1);

        layer.openCell(notebook.pickedTable, 1, 1);

        compare(layer.editingCell, 3);
    }

    function test_oneTapTakesHoldOfTheTableAndOpensTheBoxItLandedIn() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;
        // What the notebook says about the table it holds follows the table, so the numbers are
        // taken down before it is let go of.
        const atX = notebook.pickedTableBox.columnX + 4;
        const atY = notebook.pickedTableBox.columnY + 4;
        notebook.pickedTable = "";
        layer.leave();

        // What a tap does, without going through the pointer.
        notebook.pickedTable = notebook.tableUnder(atX, atY);
        layer.openCellAt(notebook.pickedTable, Qt.point(atX, atY));

        compare(notebook.pickedTable, tableId);
        compare(layer.editingRow, 0);
        compare(layer.editingColumn, 0);
    }

    function test_walkingOffTheEndOfATableAddsARow() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;

        layer.openCell(tableId, 1, 1);
        layer.stepOn(1);

        compare(notebook.pickedTableBox.heights.length, 3);
        compare(layer.editingRow, 2);
        compare(layer.editingColumn, 0);
    }

    function test_aRuleBetweenTwoColumnsIsPulledAboutWithoutChangingTheWidth() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 3);
        const tableId = notebook.pickedTable;
        // The measures the notebook publishes follow the table, so they are taken down as plain
        // numbers before it is asked to change.
        const first = notebook.pickedTableBox.widths[0];
        const second = notebook.pickedTableBox.widths[1];
        const third = notebook.pickedTableBox.widths[2];
        const wide = first + second + third;

        layer.pullColumn(1, 20);

        // The ruling follows the pointer before anything is written down.
        compare(layer.widths[0], first + 20);
        compare(layer.widths[1], second - 20);

        layer.settleRules();

        const now = notebook.pickedTableBox.widths;
        fuzzyCompare(now[0], first + 20, 0.01);
        fuzzyCompare(now[1], second - 20, 0.01);
        fuzzyCompare(now[2], third, 0.01);
        fuzzyCompare(now[0] + now[1] + now[2], wide, 0.01);

        notebook.undo();

        fuzzyCompare(notebook.pickedTableBox.widths[0], first, 0.01);
    }

    function test_ARuleIsNeverPulledPastWhatABoxNeeds() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);
        const first = notebook.pickedTableBox.widths[0];
        const second = notebook.pickedTableBox.widths[1];

        layer.pullColumn(1, 100000);

        compare(layer.widths[1], layer.narrowest);
        fuzzyCompare(layer.widths[0], first + second - layer.narrowest, 0.01);
    }

    function test_theWordsOfABoxCanBeLinedUpOnTheirOwn() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;

        notebook.alignCell(tableId, 1, 0, 2);

        compare(notebook.pickedTableBox.aligns[2], 2);
        compare(notebook.pickedTableBox.aligns[0], 0);

        notebook.undo();

        compare(notebook.pickedTableBox.aligns[2], 0);
    }

    function test_aStretchOfBoxesIsMarkedOutByDraggingAcrossThem() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;
        const atX = notebook.pickedTableBox.columnX + 4;
        const atY = notebook.pickedTableBox.columnY + 4;
        const wide = notebook.pickedTableBox.widths[0];
        const tall = notebook.pickedTableBox.heights[0];

        layer.openCellAt(tableId, Qt.point(atX, atY));

        verify(!layer.marking, "one box on its own is not a stretch");

        layer.markTo(tableId, Qt.point(atX + wide, atY + tall));

        verify(layer.marking);
        compare(layer.lastRow, 1);
        compare(layer.lastColumn, 1);
        // Nothing is typed into a stretch of boxes.
        compare(layer.editingRow, -1);
    }

    function test_aStretchOfBoxesIsJoinedIntoOneAndLetGoOfAgain() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;
        notebook.writeCell(tableId, 0, 0, "left");
        notebook.writeCell(tableId, 0, 1, "right");

        notebook.mergeCells(tableId, 0, 0, 0, 1);

        compare(notebook.errorMessage, "");
        compare(notebook.pickedTableBox.acrosses[0], 2);
        compare(notebook.pickedTableBox.downs[0], 1);
        compare(notebook.pickedTableBox.acrosses[1], 0);
        compare(notebook.wordsOfCell(tableId, 0, 0), "left right");

        notebook.splitCell(tableId, 0, 0);

        compare(notebook.pickedTableBox.acrosses[0], 1);
        compare(notebook.pickedTableBox.acrosses[1], 1);
        compare(notebook.wordsOfCell(tableId, 0, 0), "left right");

        notebook.undo();

        compare(notebook.pickedTableBox.acrosses[0], 2);
        void layer;
    }

    function test_aTapOnAJoinedBoxFindsTheBoxThatSwallowedTheRest() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;
        notebook.mergeCells(tableId, 0, 0, 0, 1);
        const atX = notebook.pickedTableBox.columnX + notebook.pickedTableBox.widths[0] + 4;
        const atY = notebook.pickedTableBox.columnY + 4;

        layer.openCellAt(tableId, Qt.point(atX, atY));

        compare(layer.editingRow, 0);
        compare(layer.editingColumn, 0);
    }

    function test_walkingATableWithTheKeyboardPassesOverWhatIsCovered() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 3);
        const tableId = notebook.pickedTable;
        notebook.mergeCells(tableId, 0, 0, 0, 1);

        layer.openCell(tableId, 0, 0);
        layer.stepOn(1);

        // The box beside the joined one is covered, so the next along is the third column.
        compare(layer.editingRow, 0);
        compare(layer.editingColumn, 2);
    }

    function test_aJoinedBoxIsTypedIntoAcrossTheWholeOfIt() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;
        const one = notebook.pickedTableBox.widths[0];
        const two = notebook.pickedTableBox.widths[1];

        notebook.mergeCells(tableId, 0, 0, 0, 1);
        layer.openCell(tableId, 0, 0);

        fuzzyCompare(layer.widthRoomOf(0, 0), one + two, 0.01);
        fuzzyCompare(layer.downRoomOf(0, 0), notebook.pickedTableBox.heights[0], 0.01);
    }

    height: 300
    name: "TableLayer"
    visible: true
    when: windowShown
    width: 400

    Component {
        id: canvasComponent

        InkCanvas {
            height: testCase.height
            width: testCase.width
        }
    }

    Component {
        id: notebookComponent

        NotebookViewModel {
        }
    }

    Component {
        id: toolsComponent

        ToolViewModel {
        }
    }

    Component {
        id: layerComponent

        TableLayer {
            height: testCase.height
            width: testCase.width
        }
    }

    Component {
        id: rowsComponent

        Repeater {
            delegate: Item {
                required property var bolds
                required property var fills
                required property var inks
                required property var heights
                required property var rises
                required property string tableId
                required property var widths
                required property var words
            }
        }
    }
}
