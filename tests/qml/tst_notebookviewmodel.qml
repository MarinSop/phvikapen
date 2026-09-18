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

        draw(notebook, 40, 40);
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
        draw(notebook, 40, 40);
        draw(notebook, 40, 120);
        draw(notebook, 200, 200);
        const canvas = notebook.canvas;
        canvas.erasing = true;

        mousePress(canvas, 40, 30);
        mouseMove(canvas, 40, 80, -1, Qt.LeftButton);
        mouseMove(canvas, 40, 150, -1, Qt.LeftButton);
        compare(notebook.strokeCount, 3);

        mouseRelease(canvas, 40, 150);
        compare(notebook.strokeCount, 1);

        notebook.undo();
        compare(notebook.strokeCount, 3);

        notebook.redo();
        compare(notebook.strokeCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_theEraserLeavesUntouchedStrokesAlone() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 40, 40);
        const canvas = notebook.canvas;
        canvas.erasing = true;

        mousePress(canvas, 300, 250);
        mouseRelease(canvas, 300, 250);

        compare(notebook.strokeCount, 1);
        verify(notebook.canUndo);
        notebook.undo();
        compare(notebook.strokeCount, 0);
    }

    function test_inkStaysOnTheSheet() {
        const notebook = openNotebook(newNotebookPath());

        draw(notebook, 2, 120);
        compare(notebook.strokeCount, 0);

        draw(notebook, 200, 120);
        compare(notebook.strokeCount, 1);

        notebook.paper = PageOptions.Infinite;
        draw(notebook, 2, 120);
        compare(notebook.strokeCount, 2);
        compare(notebook.errorMessage, "");
    }

    function test_importingAPdfAddsAPagePerPageOfIt() {
        const notebook = openNotebook(newNotebookPath());
        compare(notebook.pageCount, 1);

        notebook.importDocument("file://" + samplePdf);

        tryCompare(notebook, "pageCount", 3);
        compare(notebook.currentPage, 1);
        compare(notebook.paper, PageOptions.Custom);
        compare(notebook.background, PageOptions.Blank);
        tryCompare(notebook, "loaded", true);
        compare(notebook.errorMessage, "");

        notebook.undo();
        compare(notebook.pageCount, 1);

        notebook.redo();
        compare(notebook.pageCount, 3);
        compare(notebook.errorMessage, "");
    }

    function test_animportedPageSurvivesReopening() {
        const path = newNotebookPath();
        const first = openNotebook(path);
        first.importDocument("file://" + samplePdf);
        tryCompare(first, "pageCount", 3);
        first.destroy();
        wait(0);

        const reopened = openNotebook(path);
        compare(reopened.pageCount, 3);
        reopened.currentPage = 1;
        compare(reopened.paper, PageOptions.Custom);
        compare(reopened.errorMessage, "");
    }

    function test_theNotebookCanBeWrittenOutAsAPdf() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 120, 120);
        notebook.importDocument("file://" + samplePdf);
        tryCompare(notebook, "pageCount", 3);
        const target = temporaryDirectory + "/exported-" + notebookCount + ".pdf";
        const done = createTemporaryObject(signalSpyComponent, testCase, {
            target: notebook,
            signalName: "exported"
        });

        notebook.exportToPdf("file://" + target);

        tryCompare(done, "count", 1);
        compare(done.signalArguments[0][0], target);
        compare(notebook.exporting, false);
        compare(notebook.errorMessage, "");
    }

    function test_anExportThatCannotBeWrittenIsReported() {
        const notebook = openNotebook(newNotebookPath());
        const target = temporaryDirectory + "/missing/exported.pdf";

        notebook.exportToPdf("file://" + target);

        tryVerify(() => notebook.errorMessage !== "");
        compare(notebook.exporting, false);
    }

    function test_everyPageKeepsItsOwnStrokes() {
        const notebook = openNotebook(newNotebookPath());
        compare(notebook.pageCount, 1);
        compare(notebook.sectionCount, 1);
        draw(notebook, 40, 40);

        notebook.addPage();
        compare(notebook.pageCount, 2);
        compare(notebook.currentPage, 1);
        compare(notebook.strokeCount, 0);
        draw(notebook, 40, 40);
        draw(notebook, 40, 120);

        notebook.previousPage();
        compare(notebook.currentPage, 0);
        compare(notebook.strokeCount, 1);

        notebook.nextPage();
        compare(notebook.currentPage, 1);
        compare(notebook.strokeCount, 2);
        compare(notebook.errorMessage, "");
    }

    function test_undoGoesBackToThePageItChanges() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 40, 40);
        notebook.addPage();
        draw(notebook, 40, 40);
        notebook.previousPage();
        compare(notebook.currentPage, 0);

        notebook.undo();

        compare(notebook.currentPage, 1);
        compare(notebook.strokeCount, 0);
        compare(notebook.errorMessage, "");
    }

    function test_aDeletedPageComesBackWithItsStrokes() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 40, 40);
        notebook.addPage();
        compare(notebook.pageCount, 2);

        notebook.deletePage(0);
        compare(notebook.pageCount, 1);
        compare(notebook.strokeCount, 0);

        notebook.undo();
        compare(notebook.pageCount, 2);
        notebook.currentPage = 0;
        compare(notebook.strokeCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_theLastPageOfASectionStays() {
        const notebook = openNotebook(newNotebookPath());

        notebook.deletePage(0);

        compare(notebook.pageCount, 1);
        verify(notebook.errorMessage !== "");
    }

    function test_sectionsHoldTheirOwnPages() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        compare(notebook.pageCount, 2);

        notebook.addSection();
        compare(notebook.sectionCount, 2);
        compare(notebook.currentSection, 1);
        compare(notebook.pageCount, 1);
        compare(notebook.sections.count, 2);
        compare(notebook.pages.count, 1);

        notebook.previousPage();
        compare(notebook.currentSection, 0);
        compare(notebook.currentPage, 1);

        notebook.nextPage();
        compare(notebook.currentSection, 1);
        compare(notebook.errorMessage, "");
    }

    function test_pageStyleChangesAndSurvivesReopening() {
        const path = newNotebookPath();
        const first = openNotebook(path);
        compare(first.paper, PageOptions.A4);
        compare(first.background, PageOptions.Lined);

        first.background = PageOptions.Grid;
        first.paper = PageOptions.Infinite;
        first.lineSpacing = 5;
        compare(first.background, PageOptions.Grid);

        first.undo();
        compare(first.lineSpacing, 7);

        first.destroy();
        wait(0);

        const reopened = openNotebook(path);
        compare(reopened.paper, PageOptions.Infinite);
        compare(reopened.background, PageOptions.Grid);
        compare(reopened.errorMessage, "");
    }

    function test_theOutlineSurvivesReopening() {
        const path = newNotebookPath();
        const first = openNotebook(path);
        first.addPage();
        first.addSection();
        first.renameSection(1, "Physics");
        first.destroy();
        wait(0);

        const reopened = openNotebook(path);
        compare(reopened.sectionCount, 2);
        compare(reopened.sections.count, 2);
        compare(reopened.pageCount, 2);
        reopened.currentSection = 1;
        compare(reopened.pageCount, 1);
        compare(reopened.errorMessage, "");
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
        draw(notebook, 40, 40);
        draw(notebook, 40, 120);

        notebook.clearPage();
        compare(notebook.strokeCount, 0);

        notebook.undo();
        compare(notebook.strokeCount, 2);
        compare(notebook.errorMessage, "");
    }

    function test_strokesSurviveReopeningTheNotebook() {
        const path = newNotebookPath();
        const first = openNotebook(path);
        draw(first, 40, 40);
        draw(first, 40, 120);
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
        id: signalSpyComponent

        SignalSpy {
        }
    }

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
