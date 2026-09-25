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

        layer.openCellAt(Qt.point(box.columnX + box.widths[0] + 2, box.columnY + box.heights[0] + 2));

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

        layer.openCellAt(Qt.point(box.columnX - 20, box.columnY - 20));

        compare(layer.editingRow, -1);
        compare(layer.editingColumn, -1);
    }

    function test_theNextBoxAlongIsReachedWithoutLeavingTheTable() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);

        layer.openCell(0, 0);
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

    function test_theBoxAfterTheLastIsNeverOpened() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(1, 2);

        layer.openCell(0, 1);
        layer.stepOn(1);

        compare(layer.editingRow, 0);
        compare(layer.editingColumn, 1);
    }

    function test_reachingForAnotherToolLetsGoOfTheTable() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addTable(2, 2);
        layer.openCell(0, 0);

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

        layer.openCell(1, 1);

        compare(layer.editingCell, 3);
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
