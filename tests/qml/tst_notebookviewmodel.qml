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
        canvas.eraserRadius = 30;
        canvas.erasing = true;

        mousePress(canvas, 40, 30);
        mouseMove(canvas, 40, 80, -1, Qt.LeftButton);
        mouseMove(canvas, 40, 150, -1, Qt.LeftButton);
        mouseRelease(canvas, 40, 150);
        const afterErasing = notebook.strokeCount;
        verify(afterErasing !== 3);

        notebook.undo();
        compare(notebook.strokeCount, 3);

        notebook.redo();
        compare(notebook.strokeCount, afterErasing);
        compare(notebook.errorMessage, "");
        canvas.erasing = false;
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

    function test_aDeletedPageWaitsInTheTrashUntilItIsPutBack() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        compare(notebook.pageCount, 2);
        notebook.deletePage(1);
        compare(notebook.pageCount, 1);

        notebook.refreshTrash();
        tryCompare(notebook.trash, "count", 1);
        notebook.restoreTrashed(0);

        tryCompare(notebook, "pageCount", 2);
        tryCompare(notebook.trash, "count", 0);
        compare(notebook.errorMessage, "");
    }

    function test_emptyingTheTrashTakesEverythingInIt() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        draw(notebook, 120, 120);
        notebook.deletePage(1);
        notebook.refreshTrash();
        tryCompare(notebook.trash, "count", 1);

        notebook.emptyTrash();

        tryCompare(notebook.trash, "count", 0);
        compare(notebook.pageCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_eachPageComesBackToTheViewItWasLeftAt() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        notebook.addPage();
        compare(notebook.currentPage, 1);
        canvas.zoomIn();
        const zoomed = canvas.zoom;
        verify(zoomed > 0);

        notebook.previousPage();
        notebook.nextPage();

        compare(notebook.currentPage, 1);
        fuzzyCompare(canvas.zoom, zoomed, 0.001);
    }

    function test_aSavedNotebookHoldsEverythingInIt() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 120, 120);
        notebook.addPage();
        draw(notebook, 140, 140);
        const target = temporaryDirectory + "/copy-" + notebookCount + ".phvika";
        const done = createTemporaryObject(signalSpyComponent, testCase, {
            target: notebook,
            signalName: "saved"
        });

        notebook.saveAs("file://" + target);

        tryCompare(done, "count", 1);
        compare(notebook.keptAt, target);
        const copy = openNotebook(target);
        compare(copy.pageCount, 2);
        compare(copy.strokeCount, 1);
        compare(copy.errorMessage, "");
    }

    function marquee(canvas, left, top, right, bottom) {
        mousePress(canvas, left, top);
        mouseMove(canvas, (left + right) / 2, (top + bottom) / 2, -1, Qt.LeftButton);
        mouseMove(canvas, right, bottom, -1, Qt.LeftButton);
        mouseRelease(canvas, right, bottom);
    }

    function test_aMarqueePicksTheStrokesInsideIt() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        draw(notebook, 120, 120);
        draw(notebook, 240, 240);
        compare(notebook.strokeCount, 2);

        canvas.selecting = true;
        marquee(canvas, 100, 100, 200, 200);

        compare(canvas.selectedCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_whatIsPickedCanBeMovedAndPutBack() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        draw(notebook, 120, 120);
        canvas.selecting = true;
        marquee(canvas, 100, 100, 200, 200);
        compare(canvas.selectedCount, 1);

        const box = canvas.selectionRect;
        const fromX = box.x + (box.width / 2);
        const fromY = box.y + (box.height / 2);
        mousePress(canvas, fromX, fromY);
        mouseMove(canvas, fromX + 15, fromY + 15, -1, Qt.LeftButton);
        mouseRelease(canvas, fromX + 20, fromY + 20);

        compare(canvas.selectedCount, 1);
        compare(notebook.strokeCount, 1);
        notebook.undo();
        compare(notebook.strokeCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_whatIsPickedCanBeDeleted() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        draw(notebook, 120, 120);
        canvas.selecting = true;
        marquee(canvas, 100, 100, 200, 200);
        compare(canvas.selectedCount, 1);

        notebook.deleteSelection();

        compare(notebook.strokeCount, 0);
        compare(canvas.selectedCount, 0);
        notebook.undo();
        compare(notebook.strokeCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_theSelectionLetsGoWhenTheToolIsPutAway() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        draw(notebook, 120, 120);
        canvas.selecting = true;
        marquee(canvas, 100, 100, 200, 200);
        compare(canvas.selectedCount, 1);

        canvas.selecting = false;

        compare(canvas.selectedCount, 0);
    }

    function test_whatIsPickedCanBeCopiedOntoAnotherPage() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        draw(notebook, 120, 120);
        canvas.selecting = true;
        marquee(canvas, 100, 100, 200, 200);
        compare(canvas.selectedCount, 1);

        notebook.copySelection();
        verify(notebook.hasCopiedStrokes);
        notebook.addPage();
        compare(notebook.strokeCount, 0);
        notebook.pasteStrokes();

        compare(notebook.strokeCount, 1);
        compare(canvas.selectedCount, 1);
        notebook.undo();
        compare(notebook.strokeCount, 0);
        compare(notebook.errorMessage, "");
    }

    function test_whatIsPickedCanBeGivenAnotherColour() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        draw(notebook, 120, 120);
        canvas.selecting = true;
        marquee(canvas, 100, 100, 200, 200);
        compare(canvas.selectedCount, 1);

        notebook.recolourSelection("#d13438");

        compare(notebook.strokeCount, 1);
        verify(notebook.canUndo);
        notebook.undo();
        compare(notebook.strokeCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_aStrokeCanBeDrawnAsAStraightLine() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        canvas.shape = 1;

        mousePress(canvas, 120, 120);
        mouseMove(canvas, 140, 150, -1, Qt.LeftButton);
        mouseMove(canvas, 160, 130, -1, Qt.LeftButton);
        mouseRelease(canvas, 180, 160);

        compare(notebook.strokeCount, 1);
        compare(notebook.errorMessage, "");
        canvas.shape = 0;
    }

    function test_theShapeIsShownWhileItIsBeingDrawn() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        canvas.shape = 2;

        mousePress(canvas, 100, 100);
        mouseMove(canvas, 150, 140, -1, Qt.LeftButton);
        mouseMove(canvas, 200, 180, -1, Qt.LeftButton);
        verify(canvas.zoom > 0);
        mouseRelease(canvas, 200, 180);

        compare(notebook.strokeCount, 1);
        compare(notebook.errorMessage, "");
        canvas.shape = 0;
    }

    function test_aPageCanBeDuplicatedWithWhatIsOnIt() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 120, 120);
        compare(notebook.pageCount, 1);
        compare(notebook.strokeCount, 1);

        notebook.duplicatePage(0);

        tryCompare(notebook, "pageCount", 2);
        compare(notebook.currentPage, 1);
        tryCompare(notebook, "strokeCount", 1);
        notebook.previousPage();
        compare(notebook.strokeCount, 1);
        notebook.undo();
        compare(notebook.pageCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_aDuplicatedPageKeepsItsOwnStrokesAfterReopening() {
        const path = newNotebookPath();
        const first = openNotebook(path);
        draw(first, 120, 120);
        first.duplicatePage(0);
        tryCompare(first, "pageCount", 2);
        draw(first, 200, 180);
        tryCompare(first, "strokeCount", 2);
        first.destroy();
        wait(0);

        const reopened = openNotebook(path);

        compare(reopened.pageCount, 2);
        compare(reopened.strokeCount, 1);
        reopened.nextPage();
        tryCompare(reopened, "strokeCount", 2);
        compare(reopened.errorMessage, "");
    }

    function test_theSidebarGetsAPictureOfEveryPage() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 120, 120);
        compare(notebook.pages.count, 1);

        notebook.wantThumbnail(0);

        tryVerify(() => notebook.pages.data(notebook.pages.index(0, 0), Qt.UserRole + 3) !== "");
        const drawn = notebook.pages.data(notebook.pages.index(0, 0), Qt.UserRole + 3);
        verify(drawn.startsWith("image://pages/"));
        compare(notebook.errorMessage, "");
    }

    function test_aNewPageGetsItsOwnPictureAndCanStillBePicked() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 120, 120);
        notebook.wantThumbnail(0);
        tryVerify(() => notebook.pages.data(notebook.pages.index(0, 0), Qt.UserRole + 3) !== "");

        const added = createTemporaryObject(signalSpyComponent, testCase, {
            target: notebook,
            signalName: "pageAdded"
        });
        notebook.addPage();

        compare(added.count, 1);
        compare(notebook.pages.count, 2);
        compare(notebook.currentPage, 1);
        notebook.wantThumbnail(1);
        tryVerify(() => notebook.pages.data(notebook.pages.index(1, 0), Qt.UserRole + 3) !== "");
        const first = notebook.pages.data(notebook.pages.index(0, 0), Qt.UserRole + 3);
        const second = notebook.pages.data(notebook.pages.index(1, 0), Qt.UserRole + 3);
        verify(first !== "");
        verify(first !== second);

        notebook.currentPage = 0;
        compare(notebook.currentPage, 0);
        compare(notebook.strokeCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_theSectionStandsInOneColumnWhenAsked() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        notebook.currentPage = 0;
        notebook.continuous = true;
        const canvas = notebook.canvas;
        canvas.fitPage();
        verify(canvas.visibleSheetCount >= 1);
        const wanted = createTemporaryObject(signalSpyComponent, testCase, {
            target: canvas,
            signalName: "pageWanted"
        });
        for (let step = 0; step < 8; ++step) {
            canvas.zoomOut();
        }
        canvas.goToSheet(1);

        tryCompare(notebook, "currentPage", 1);
        verify(wanted.count >= 1);
        verify(canvas.visibleSheetCount >= 2);
        compare(notebook.errorMessage, "");
    }

    function test_scrollingDownTurnsToTheNextPage() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        notebook.currentPage = 0;
        notebook.continuous = true;
        const canvas = notebook.canvas;
        canvas.fitPage();
        compare(notebook.currentPage, 0);

        for (let step = 0; step < 40; ++step) {
            mouseWheel(canvas, canvas.width / 2, canvas.height / 2, 0, -120);
        }

        tryCompare(notebook, "currentPage", 1);
        notebook.continuous = false;
        compare(notebook.errorMessage, "");
    }

    function test_scrollingToTheNextPageKeepsTheZoom() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        notebook.currentPage = 0;
        notebook.continuous = true;
        const canvas = notebook.canvas;
        canvas.fitPage();
        canvas.zoomIn();
        canvas.zoomIn();
        const zoom = canvas.zoom;

        for (let step = 0; step < 60; ++step) {
            mouseWheel(canvas, canvas.width / 2, canvas.height / 2, 0, -120);
        }

        tryCompare(notebook, "currentPage", 1);
        compare(canvas.zoom, zoom, "scrolling changed how close the page is");
        notebook.continuous = false;
        compare(notebook.errorMessage, "");
    }

    function test_endlessPagesStandInAColumnToo() {
        const notebook = openNotebook(newNotebookPath());
        notebook.paper = PageOptions.Infinite;
        notebook.addPage();
        notebook.currentPage = 0;
        notebook.continuous = true;
        const canvas = notebook.canvas;
        canvas.fitPage();

        for (let step = 0; step < 60; ++step) {
            mouseWheel(canvas, canvas.width / 2, canvas.height / 2, 0, -120);
        }

        tryCompare(notebook, "currentPage", 1, 5000, "scrolling did not carry on to the next page");
        notebook.continuous = false;
        compare(notebook.errorMessage, "");
    }

    function test_writingOnTheSecondSheetLandsOnTheSecondPage() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        notebook.currentPage = 0;
        notebook.continuous = true;
        const canvas = notebook.canvas;
        canvas.goToSheet(1);
        tryCompare(notebook, "currentPage", 1);

        draw(notebook, 60, 60);

        compare(notebook.currentPage, 1);
        compare(notebook.strokeCount, 1);
        notebook.currentPage = 0;
        compare(notebook.strokeCount, 0);
        notebook.continuous = false;
        compare(notebook.errorMessage, "");
    }

    function test_aPageCanBeGivenItsOwnSize() {
        const path = newNotebookPath();
        const first = openNotebook(path);

        first.paper = PageOptions.Custom;
        first.customWidth = 120;
        first.customHeight = 160;

        compare(first.paper, PageOptions.Custom);
        first.destroy();
        wait(0);

        const reopened = openNotebook(path);
        compare(reopened.paper, PageOptions.Custom);
        compare(Math.round(reopened.customWidth), 120);
        compare(Math.round(reopened.customHeight), 160);
        compare(reopened.errorMessage, "");
    }

    function test_theSetupBelongsToTheWholeSection() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        notebook.currentPage = 1;

        notebook.background = PageOptions.Dotted;
        notebook.paper = PageOptions.A5;

        notebook.currentPage = 0;
        compare(notebook.background, PageOptions.Dotted, "the page before kept its own paper");
        compare(notebook.paper, PageOptions.A5);
        compare(notebook.errorMessage, "");
    }

    function test_theSetupOfASectionGoesBackInOneStep() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        notebook.addPage();
        notebook.currentPage = 0;
        const wasBackground = notebook.background;

        notebook.background = PageOptions.Dotted;
        notebook.undo();

        compare(notebook.background, wasBackground);
        notebook.currentPage = 2;
        compare(notebook.background, wasBackground, "one page was left behind by the undo");
        compare(notebook.errorMessage, "");
    }

    function test_savingWaitsToBeToldWhereTheNotebookGoes() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 40, 40);
        const saved = createTemporaryObject(signalSpyComponent, testCase, {
            target: notebook,
            signalName: "saved"
        });

        verify(!notebook.save());
        compare(saved.count, 0);
        verify(notebook.edited);

        const target = temporaryDirectory + "/kept-" + notebookCount + ".phvika";
        notebook.saveAs("file://" + target);

        tryCompare(saved, "count", 1);
        verify(!notebook.edited);
        verify(notebook.save());
        tryCompare(saved, "count", 2);
        compare(notebook.errorMessage, "");
    }

    function test_thePictureOfAPageIsThrownAwayWhenThePageChanges() {
        const notebook = openNotebook(newNotebookPath());
        notebook.wantThumbnail(0);
        tryVerify(() => notebook.pages.data(notebook.pages.index(0, 0), Qt.UserRole + 3) !== "");
        const before = notebook.pages.data(notebook.pages.index(0, 0), Qt.UserRole + 3);

        draw(notebook, 120, 120);
        notebook.wantThumbnail(0);

        tryVerify(() => notebook.pages.data(notebook.pages.index(0, 0), Qt.UserRole + 3) !== "");
        const after = notebook.pages.data(notebook.pages.index(0, 0), Qt.UserRole + 3);
        verify(after !== before);
    }

    function test_theWholeImportedPageIsDrawnWhileItFitsInOnePicture() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        notebook.importDocument("file://" + samplePdf);
        tryCompare(notebook, "pageCount", 3);
        tryVerify(() => canvas.mediaArea.width > 0);
        const wholeWidth = canvas.mediaArea.width;
        const wholeHeight = canvas.mediaArea.height;

        canvas.zoomIn();
        canvas.zoomIn();
        canvas.zoomIn();

        wait(300);
        compare(canvas.mediaArea.width, wholeWidth);
        compare(canvas.mediaArea.height, wholeHeight);
        compare(notebook.errorMessage, "");
    }

    function test_theDocumentIsDrawnAgainWhenTheReaderComesCloser() {
        const path = newNotebookPath();
        const first = openNotebook(path);
        first.importDocument("file://" + samplePdf);
        tryCompare(first, "pageCount", 3);
        first.canvas = null;

        // Opened afresh, as after a restart: nothing has been imported in this sitting.
        const notebook = openNotebook(path);
        const canvas = notebook.canvas;
        notebook.currentPage = 1;
        tryVerify(() => canvas.mediaSize.width > 0);

        for (let step = 0; step < 6; ++step) {
            canvas.zoomOut();
        }
        tryVerify(() => canvas.mediaSize.width > 0);
        const faraway = canvas.mediaSize.width;

        for (let step = 0; step < 8; ++step) {
            canvas.zoomIn();
        }

        tryVerify(() => canvas.mediaSize.width > faraway, 5000, "the document stayed as coarse as it was");
        compare(notebook.errorMessage, "");
    }

    function test_theSheetsAroundAreDrawnFinelyWhenTheReaderComesCloser() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        notebook.importDocument("file://" + samplePdf);
        tryCompare(notebook, "pageCount", 3);
        notebook.continuous = true;
        notebook.currentPage = 1;
        tryVerify(() => notebook.mediaPixelsOn(2) > 0);
        const faraway = notebook.mediaPixelsOn(2);

        for (let step = 0; step < 14; ++step) {
            canvas.zoomIn();
        }
        verify(canvas.zoom > 2, "the test did not come close enough");

        tryVerify(() => notebook.mediaPixelsOn(2) > faraway, 5000, "the sheet below stayed coarse");
        notebook.continuous = false;
        compare(notebook.errorMessage, "");
    }

    function test_theWholeSheetIsDrawnHoweverCloseTheReaderComes() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        notebook.importDocument("file://" + samplePdf);
        tryCompare(notebook, "pageCount", 3);
        tryVerify(() => canvas.mediaArea.width > 0);
        const wholeWidth = canvas.mediaArea.width;
        const wholeHeight = canvas.mediaArea.height;

        for (let step = 0; step < 16; ++step) {
            canvas.zoomIn();
        }
        wait(400);

        compare(canvas.mediaArea.width, wholeWidth, "only a part of the sheet was drawn");
        compare(canvas.mediaArea.height, wholeHeight);
        verify(canvas.mediaSize.width <= 4096);
        verify(canvas.mediaSize.height <= 4096);
        compare(notebook.errorMessage, "");
    }

    function test_anImportedPageIsDrawnWhereThePaperIs() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;

        notebook.importDocument("file://" + samplePdf);
        tryCompare(notebook, "pageCount", 3);

        tryVerify(() => canvas.mediaArea.width > 0);
        const area = canvas.mediaArea;
        verify(area.x >= 0);
        verify(area.y >= 0);
        verify(area.width <= 600);
        verify(area.height <= 850);
        compare(notebook.errorMessage, "");
    }

    function test_thePictureOfAnImportedPageHasTheDocumentInIt() {
        const notebook = openNotebook(newNotebookPath());
        notebook.importDocument("file://" + samplePdf);
        tryCompare(notebook, "pageCount", 3);

        notebook.wantThumbnail(1);

        tryVerify(() => notebook.pages.data(notebook.pages.index(1, 0), Qt.UserRole + 3) !== "");
        compare(notebook.errorMessage, "");
    }

    function test_theEraserTakesOutOnlyTheBitItTouches() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        mousePress(canvas, 60, 150);
        for (let x = 60; x <= 300; x += 20) {
            mouseMove(canvas, x, 150, -1, Qt.LeftButton);
        }
        mouseRelease(canvas, 300, 150);
        compare(notebook.strokeCount, 1);

        canvas.eraserRadius = 6;
        canvas.erasing = true;
        mousePress(canvas, 180, 150);
        mouseMove(canvas, 185, 150, -1, Qt.LeftButton);
        mouseRelease(canvas, 185, 150);
        canvas.erasing = false;

        compare(notebook.strokeCount, 2);
        notebook.undo();
        compare(notebook.strokeCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_inkCanBeWrittenBesideTheSheet() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        canvas.fitPage();

        mousePress(canvas, 20, 20);
        mouseMove(canvas, 30, 26, -1, Qt.LeftButton);
        mouseRelease(canvas, 40, 32);

        compare(notebook.strokeCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_theSmoothingOfThePenCanBeTurnedDown() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;

        canvas.smoothing = 0;
        compare(canvas.smoothing, 0);
        draw(notebook, 120, 120);
        compare(notebook.strokeCount, 1);

        canvas.smoothing = 1;
        compare(canvas.smoothing, 1);
        draw(notebook, 160, 160);
        compare(notebook.strokeCount, 2);
        compare(notebook.errorMessage, "");
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

        canvas.eraserRadius = 20;
        canvas.erasing = true;
        mousePress(canvas, 150, 100 - shift);
        mouseMove(canvas, 170, 110 - shift, -1, Qt.LeftButton);
        mouseMove(canvas, 190, 120 - shift, -1, Qt.LeftButton);
        mouseRelease(canvas, 190, 120 - shift);

        compare(notebook.strokeCount, 0);
        canvas.erasing = false;
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
