import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    property int notebookCount: 0

    function openNotebook(path) {
        const canvas = createTemporaryObject(canvasComponent, testCase);
        const notebook = createTemporaryObject(notebookComponent, testCase, {
            canvas: canvas,
            notebookPath: path
        });
        tryCompare(notebook, "loaded", true);
        return notebook;
    }

    function newNotebookPath() {
        notebookCount += 1;
        return temporaryDirectory + "/notebook-" + notebookCount + ".phvika";
    }

    function draw(notebook, fromX, fromY) {
        const canvas = notebook.canvas;
        mousePress(canvas, fromX, fromY);
        mouseMove(canvas, fromX + 20, fromY + 10, -1, Qt.LeftButton);
        mouseMove(canvas, fromX + 40, fromY + 20, -1, Qt.LeftButton);
        mouseRelease(canvas, fromX + 40, fromY + 20);
    }

    function test_drawnStrokesCanBeUndoneAndRedone() {
        const notebook = openNotebook(newNotebookPath());
        verify(!notebook.canUndo);

        draw(notebook, 20, 20);
        compare(notebook.strokeCount, 1);
        verify(notebook.canUndo);

        notebook.undo();
        compare(notebook.strokeCount, 0);
        verify(notebook.canRedo);

        notebook.redo();
        compare(notebook.strokeCount, 1);
        verify(!notebook.canRedo);
        compare(notebook.errorMessage, "");
    }

    function test_oneSweepOfTheEraserIsOneChange() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 20, 20);
        draw(notebook, 20, 100);
        draw(notebook, 200, 200);
        const canvas = notebook.canvas;
        canvas.erasing = true;

        mousePress(canvas, 20, 0);
        mouseMove(canvas, 20, 60, -1, Qt.LeftButton);
        mouseMove(canvas, 20, 150, -1, Qt.LeftButton);
        compare(notebook.strokeCount, 3);

        mouseRelease(canvas, 20, 150);
        compare(notebook.strokeCount, 1);

        notebook.undo();
        compare(notebook.strokeCount, 3);

        notebook.redo();
        compare(notebook.strokeCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_theEraserLeavesUntouchedStrokesAlone() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 20, 20);
        const canvas = notebook.canvas;
        canvas.erasing = true;

        mousePress(canvas, 300, 250);
        mouseRelease(canvas, 300, 250);

        compare(notebook.strokeCount, 1);
        verify(notebook.canUndo);
        notebook.undo();
        compare(notebook.strokeCount, 0);
    }

    function test_theCanvasZoomsAroundTheCursor() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        const fitted = canvas.zoom;

        mouseWheel(canvas, 200, 150, 0, 120, Qt.NoButton, Qt.ControlModifier);
        verify(canvas.zoom > fitted);

        canvas.zoomOut();
        fuzzyCompare(canvas.zoom, fitted, 0.0001);

        canvas.zoomIn();
        canvas.fitPage();
        fuzzyCompare(canvas.zoom, fitted, 0.0001);
    }

    function test_drawingLandsWhereTheScrolledPageIs() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        draw(notebook, 150, 100);
        const before = canvas.viewOrigin.y;

        mouseWheel(canvas, 200, 150, 0, -120);
        const shift = (canvas.viewOrigin.y - before) * canvas.zoom;
        verify(shift > 10);

        canvas.erasing = true;
        mousePress(canvas, 150, 100 - shift);
        mouseRelease(canvas, 150, 100 - shift);
        compare(notebook.strokeCount, 0);
    }

    function test_oneUndoBringsAClearedPageBack() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 20, 20);
        draw(notebook, 20, 100);

        notebook.clearPage();
        compare(notebook.strokeCount, 0);

        notebook.undo();
        compare(notebook.strokeCount, 2);
        compare(notebook.errorMessage, "");
    }

    function test_strokesSurviveReopeningTheNotebook() {
        const path = newNotebookPath();
        const first = openNotebook(path);
        draw(first, 20, 20);
        draw(first, 20, 100);
        first.clearPage();
        first.undo();
        first.destroy();
        wait(0);

        const reopened = openNotebook(path);
        compare(reopened.strokeCount, 2);
        verify(!reopened.canUndo);
        compare(reopened.errorMessage, "");
    }

    height: 300
    name: "NotebookViewModel"
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
}
