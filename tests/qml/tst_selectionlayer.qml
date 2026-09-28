import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    property int notebookCount: 0

    function draw(notebook, fromX, fromY) {
        const canvas = notebook.canvas;
        mousePress(canvas, fromX, fromY);
        mouseMove(canvas, fromX + 30, fromY + 20, -1, Qt.LeftButton);
        mouseMove(canvas, fromX + 60, fromY + 40, -1, Qt.LeftButton);
        mouseRelease(canvas, fromX + 60, fromY + 40);
    }

    function newNotebookPath() {
        notebookCount += 1;
        return temporaryDirectory + "/selectionlayer-" + notebookCount + ".phvika";
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

    // Ink on the page, picked up, with the frame around it drawn and measured.
    function pickedInk(notebook, tools) {
        const layer = openLayer(notebook, tools);
        testCase.draw(notebook, 60, 60);
        tryCompare(notebook, "strokeCount", 1);
        notebook.canvas.selectEverything();
        tryVerify(() => notebook.canvas.selectedCount > 0, 2000, "nothing was picked up");
        layer.refresh();
        tryVerify(() => layer.area.width > 0 && layer.visible, 2000, "the frame never took a size");
        return layer;
    }

    function pickingTools() {
        const tools = createTemporaryObject(toolsComponent, testCase);
        tools.currentTool = ToolViewModel.Selection;
        return tools;
    }

    function test_a_pickedInkIsSizedByACorner() {
        const notebook = openNotebook(newNotebookPath());
        const layer = pickedInk(notebook, pickingTools());
        const was = notebook.areaOfWhatIsPicked();
        verify(was.width > 0, "nothing was measured");
        // The third grip is the corner that pulls both ways at once, away from the paper's corner.
        const grip = findChild(layer, "sizeGrip2");
        verify(grip !== null, "there is no grip on the corner");
        tryVerify(() => grip.visible && grip.width > 0);

        // The first move only takes hold of the grip: how far it has been dragged is counted from
        // there, so a single move drags by nothing at all.
        mousePress(grip, grip.width / 2, grip.height / 2);
        mouseMove(grip, (grip.width / 2) + 15, (grip.height / 2) + 15);
        tryVerify(() => layer.working, 2000, "the corner was never taken hold of");
        mouseMove(grip, (grip.width / 2) + 70, (grip.height / 2) + 70);
        tryVerify(() => layer.liveWide > 1.05, 2000, "the corner was dragged and nothing grew");
        mouseRelease(grip, (grip.width / 2) + 70, (grip.height / 2) + 70);

        tryVerify(() => notebook.areaOfWhatIsPicked().width > was.width + 4, 4000, "the ink was not made larger");
        verify(notebook.areaOfWhatIsPicked().height > was.height + 4, "it grew one way only");

        notebook.undo();

        tryVerify(() => Math.abs(notebook.areaOfWhatIsPicked().width - was.width) <= 1, 4000, "taking it back did not put the size back");
        compare(notebook.errorMessage, "");
    }

    function test_b_pickedInkIsTurnedByItsKnob() {
        const notebook = openNotebook(newNotebookPath());
        const layer = pickedInk(notebook, pickingTools());
        const was = notebook.areaOfWhatIsPicked();
        const knob = findChild(layer, "turnGrip");
        verify(knob !== null, "there is no knob to turn it by");
        tryVerify(() => knob.visible && knob.width > 0);

        mousePress(knob, knob.width / 2, knob.height / 2);
        mouseMove(knob, (knob.width / 2) + 15, (knob.height / 2) + 15);
        tryVerify(() => layer.working, 2000, "the knob was never taken hold of");
        mouseMove(knob, (knob.width / 2) + 90, (knob.height / 2) + 90);
        tryVerify(() => Math.abs(layer.liveTurn) > 5, 2000, "the knob was dragged and nothing turned");
        mouseRelease(knob, (knob.width / 2) + 90, (knob.height / 2) + 90);

        // A line turned about its middle covers a different patch of the page than before.
        tryVerify(() => Math.abs(notebook.areaOfWhatIsPicked().width - was.width) > 1 || Math.abs(notebook.areaOfWhatIsPicked().height - was.height) > 1, 4000, "the ink was not turned");

        notebook.undo();

        tryVerify(() => Math.abs(notebook.areaOfWhatIsPicked().width - was.width) <= 1, 4000, "taking it back did not put it straight again");
        compare(notebook.errorMessage, "");
    }

    function test_d_inkOnALockedLayerIsNotPickedUp() {
        const notebook = openNotebook(newNotebookPath());
        openLayer(notebook, pickingTools());
        testCase.draw(notebook, 40, 40);
        tryCompare(notebook, "strokeCount", 1);
        const lower = notebook.activeLayer;
        notebook.addLayer();
        tryCompare(notebook.layers, "count", 2);
        verify(notebook.activeLayer !== lower, "the new layer was not taken in hand");
        testCase.draw(notebook, 150, 150);
        tryCompare(notebook, "strokeCount", 2);

        notebook.canvas.selectEverything();

        tryCompare(notebook.canvas, "selectedCount", 2, 4000, "both were not picked up to begin with");

        notebook.lockLayer(lower, true);
        notebook.canvas.selectEverything();

        tryCompare(notebook.canvas, "selectedCount", 1, 4000, "ink on a locked layer was picked up");

        notebook.lockLayer(lower, false);
        notebook.canvas.selectEverything();

        tryCompare(notebook.canvas, "selectedCount", 2, 4000, "unlocking did not give the ink back");
    }

    function test_e_inkOnAHiddenLayerIsNotPickedUp() {
        const notebook = openNotebook(newNotebookPath());
        openLayer(notebook, pickingTools());
        testCase.draw(notebook, 40, 40);
        tryCompare(notebook, "strokeCount", 1);
        const lower = notebook.activeLayer;
        notebook.addLayer();
        tryCompare(notebook.layers, "count", 2);
        testCase.draw(notebook, 150, 150);
        tryCompare(notebook, "strokeCount", 2);

        notebook.showLayer(lower, false);
        notebook.canvas.selectEverything();

        // Ink the reader cannot see must not be taken hold of by a loop drawn over it.
        tryCompare(notebook.canvas, "selectedCount", 1, 4000, "ink on a hidden layer was picked up");
    }

    function test_f_whatIsAlreadyPickedIsLetGoOfWhenItsLayerIsLocked() {
        const notebook = openNotebook(newNotebookPath());
        openLayer(notebook, pickingTools());
        testCase.draw(notebook, 40, 40);
        tryCompare(notebook, "strokeCount", 1);
        const only = notebook.activeLayer;
        notebook.canvas.selectEverything();
        tryCompare(notebook.canvas, "selectedCount", 1);

        notebook.lockLayer(only, true);

        tryCompare(notebook.canvas, "selectedCount", 0, 4000, "the ink was left picked up on a locked layer");
    }

    function test_c_theFrameStandsAsideForEveryToolButTheLoop() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = pickedInk(notebook, tools);

        verify(layer.visible);

        tools.currentTool = ToolViewModel.Pen;

        verify(!layer.visible, "the frame is still drawn with the pen in hand");
    }

    height: 300
    name: "SelectionLayer"
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

        SelectionLayer {
            height: testCase.height
            width: testCase.width
        }
    }
}
