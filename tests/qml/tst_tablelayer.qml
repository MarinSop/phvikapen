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
                required property var heights
                required property string tableId
                required property var widths
                required property var words
            }
        }
    }
}
