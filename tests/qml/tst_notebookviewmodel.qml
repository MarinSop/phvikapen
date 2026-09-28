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

    function pageNames(notebook) {
        const names = [];
        for (let step = 0; step < notebook.pages.count; ++step) {
            names.push(notebook.pages.data(notebook.pages.index(step, 0), Qt.UserRole + 1));
        }
        return names;
    }

    function layerNames(notebook) {
        const names = [];
        for (let step = 0; step < notebook.layers.count; ++step) {
            names.push(notebook.layers.data(notebook.layers.index(step, 0), Qt.UserRole + 2));
        }
        return names;
    }

    function layerIdAt(notebook, row) {
        return notebook.layers.data(notebook.layers.index(row, 0), Qt.UserRole + 1);
    }

    function test_aPageAlwaysHasOneLayerToStandThingsOn() {
        const notebook = openNotebook(newNotebookPath());

        compare(notebook.layers.count, 1);
        verify(notebook.activeLayer !== "");
        compare(layerIdAt(notebook, 0), notebook.activeLayer);
    }

    function test_aLayerIsAddedRenamedHiddenAndLocked() {
        const notebook = openNotebook(newNotebookPath());

        notebook.addLayer();

        compare(notebook.layers.count, 2);
        // The panel lists them top first, so the new one stands at the head.
        compare(layerIdAt(notebook, 0), notebook.activeLayer);

        notebook.renameLayer(notebook.activeLayer, "Tracing");

        compare(layerNames(notebook)[0], "Tracing");

        notebook.showLayer(notebook.activeLayer, false);
        compare(notebook.layers.data(notebook.layers.index(0, 0), Qt.UserRole + 3), false);

        notebook.lockLayer(notebook.activeLayer, true);
        compare(notebook.layers.data(notebook.layers.index(0, 0), Qt.UserRole + 4), true);
    }

    function test_aLayerIsCarriedUpAndDownTheOrder() {
        const notebook = openNotebook(newNotebookPath());
        notebook.renameLayer(notebook.activeLayer, "Paper");
        notebook.addLayer();
        notebook.renameLayer(notebook.activeLayer, "Notes");

        compare(layerNames(notebook), ["Notes", "Paper"]);

        // Counting from the bottom, so nought is the foot of the pile.
        notebook.moveLayer(notebook.activeLayer, 0);

        compare(layerNames(notebook), ["Paper", "Notes"]);
    }

    function test_aPageKeepsAtLeastOneLayer() {
        const notebook = openNotebook(newNotebookPath());

        notebook.removeLayer(notebook.activeLayer);

        compare(notebook.layers.count, 1);
    }

    function test_whatIsDrawnStandsOnTheLayerInHand() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addLayer();
        const notes = notebook.activeLayer;

        draw(notebook, 40, 40);

        compare(notebook.strokeCount, 1);
        compare(notebook.layers.data(notebook.layers.index(0, 0), Qt.UserRole + 5), 1);
        compare(notebook.layers.data(notebook.layers.index(1, 0), Qt.UserRole + 5), 0);
        compare(notes, notebook.activeLayer);
    }

    function test_aLayerTakenAwayTakesWhatStandsOnItAndOneUndoBringsBothBack() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addLayer();
        draw(notebook, 40, 40);
        compare(notebook.strokeCount, 1);

        notebook.removeLayer(notebook.activeLayer);

        compare(notebook.layers.count, 1);
        compare(notebook.strokeCount, 0);

        notebook.undo();

        compare(notebook.layers.count, 2);
        compare(notebook.strokeCount, 1);
    }

    function test_aLayerPutDownAgainBringsACopyOfWhatStoodOnIt() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 40, 40);
        compare(notebook.strokeCount, 1);

        notebook.duplicateLayer(layerIdAt(notebook, 0));

        compare(notebook.layers.count, 2);
        compare(notebook.strokeCount, 2);
        compare(notebook.layers.data(notebook.layers.index(0, 0), Qt.UserRole + 5), 1);
        compare(notebook.layers.data(notebook.layers.index(1, 0), Qt.UserRole + 5), 1);
    }

    function test_aTableIsCarriedFromOneLayerToAnother() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;
        notebook.addLayer();
        const notes = notebook.activeLayer;

        notebook.moveToLayer(tableId, notes);

        compare(notebook.layers.data(notebook.layers.index(0, 0), Qt.UserRole + 5), 1);
        compare(notebook.layers.data(notebook.layers.index(1, 0), Qt.UserRole + 5), 0);

        notebook.undo();

        compare(notebook.layers.data(notebook.layers.index(1, 0), Qt.UserRole + 5), 1);
    }

    function test_nothingOnALockedLayerCanBeTakenHoldOf() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addTable(2, 2);
        const box = notebook.pickedTableBox;
        const at = Qt.point(box.columnX + 4, box.columnY + 4);
        verify(notebook.tableUnder(at.x, at.y) !== "");

        notebook.lockLayer(notebook.activeLayer, true);

        compare(notebook.tableUnder(at.x, at.y), "");

        notebook.lockLayer(notebook.activeLayer, false);

        verify(notebook.tableUnder(at.x, at.y) !== "");
    }

    function test_theLayersOfAPageComeBackWhenTheNotebookIsOpenedAgain() {
        const path = newNotebookPath();
        const notebook = openNotebook(path);
        notebook.addLayer();
        notebook.renameLayer(notebook.activeLayer, "Tracing");
        notebook.showLayer(notebook.activeLayer, false);
        notebook.save();
        wait(200);

        const again = openNotebook(path);

        tryCompare(again.layers, "count", 2);
        compare(layerNames(again)[0], "Tracing");
        compare(again.layers.data(again.layers.index(0, 0), Qt.UserRole + 3), false);
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

    function test_everythingOnAPageIsPickedUpAtOnce() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 40, 40);
        draw(notebook, 60, 110);
        compare(notebook.strokeCount, 2);

        notebook.canvas.selectEverything();

        compare(notebook.canvas.selectedCount, 2);
        compare(notebook.errorMessage, "");
    }

    function test_whatIsPickedIsCopiedBesideItselfWithoutTouchingTheClipboard() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 40, 40);
        notebook.canvas.selectEverything();

        notebook.duplicateSelection();

        compare(notebook.strokeCount, 2);
        verify(!notebook.hasCopiedStrokes);

        notebook.undo();

        compare(notebook.strokeCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_cuttingTakesWhatIsPickedAndKeepsIt() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 40, 40);
        notebook.canvas.selectEverything();

        notebook.cutSelection();

        compare(notebook.strokeCount, 0);
        verify(notebook.hasCopiedStrokes);

        notebook.pasteStrokes();

        compare(notebook.strokeCount, 1);
        compare(notebook.errorMessage, "");
    }

    function test_turningWhatIsPickedIsOneChangeThatCanBeUndone() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 60, 60);
        notebook.canvas.selectEverything();
        const before = notebook.selectionArea();
        verify(before.width > 0);

        notebook.turnSelection(90);

        const turned = notebook.selectionArea();
        verify(Math.abs(turned.width - before.height) < 2);
        verify(Math.abs(turned.height - before.width) < 2);

        notebook.undo();

        const back = notebook.selectionArea();
        verify(Math.abs(back.width - before.width) < 0.01);
        verify(Math.abs(back.height - before.height) < 0.01);
        compare(notebook.errorMessage, "");
    }

    function test_sizingWhatIsPickedShowsBeforeItIsKept() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 60, 60);
        notebook.canvas.selectEverything();
        const before = notebook.selectionArea();
        const change = {
            "pivotX": before.x,
            "pivotY": before.y,
            "wide": 2,
            "tall": 2
        };

        notebook.showTransform(change);

        compare(notebook.strokeCount, 1);
        verify(!notebook.canRedo);

        notebook.applyTransform(change);

        const larger = notebook.selectionArea();
        verify(larger.width > before.width * 1.5);

        notebook.undo();

        verify(Math.abs(notebook.selectionArea().width - before.width) < 0.01);
        compare(notebook.errorMessage, "");
    }

    function test_aPictureGoesOnThePageAndComesOffAgain() {
        const notebook = openNotebook(newNotebookPath());

        notebook.addPicture(AppInfo.fileUrl(samplePicture));

        verify(notebook.pickedPicture !== "");
        const shape = notebook.pickedPictureBox.boxWidth / notebook.pickedPictureBox.boxHeight;
        verify(notebook.pickedPictureBox.boxWidth > 0);
        // The picture keeps the shape it came with: twice as wide as it is tall.
        fuzzyCompare(shape, 2.0, 0.05);
        verify(notebook.canUndo);

        notebook.undo();

        compare(notebook.pickedPictureBox.pictureId, undefined);
        compare(notebook.errorMessage, "");
    }

    function test_aPictureIsFoundWhereItStandsAndNowhereElse() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPicture(AppInfo.fileUrl(samplePicture));
        const box = notebook.pickedPictureBox;

        const inside = notebook.pictureUnder(box.columnX + (box.boxWidth / 2), box.columnY + (box.boxHeight / 2));
        const outside = notebook.pictureUnder(box.columnX - 40, box.columnY - 40);

        compare(inside, notebook.pickedPicture);
        compare(outside, "");
    }

    function test_aPictureIsMovedTurnedAndSizedAsOneChange() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPicture(AppInfo.fileUrl(samplePicture));
        const pictureId = notebook.pickedPicture;
        // What the notebook says about the picture it holds follows the picture, so the numbers
        // are taken down before it is asked to change.
        const wasX = notebook.pickedPictureBox.columnX;
        const wasWide = notebook.pickedPictureBox.boxWidth;

        notebook.placePicture(pictureId, {
            "columnX": wasX + 30,
            "columnY": notebook.pickedPictureBox.columnY + 40,
            "boxWidth": wasWide * 2,
            "boxHeight": notebook.pickedPictureBox.boxHeight * 2,
            "turn": 45
        });

        compare(notebook.errorMessage, "");
        fuzzyCompare(notebook.pickedPictureBox.turn, 45, 0.01);
        fuzzyCompare(notebook.pickedPictureBox.boxWidth, wasWide * 2, 0.01);
        fuzzyCompare(notebook.pickedPictureBox.columnX, wasX + 30, 0.01);

        notebook.undo();

        fuzzyCompare(notebook.pickedPictureBox.columnX, wasX, 0.01);
        fuzzyCompare(notebook.pickedPictureBox.boxWidth, wasWide, 0.01);
        fuzzyCompare(notebook.pickedPictureBox.turn, 0, 0.01);
        compare(notebook.errorMessage, "");
    }

    function test_takingAPictureAwayLetsGoOfIt() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPicture(AppInfo.fileUrl(samplePicture));
        const pictureId = notebook.pickedPicture;

        notebook.removePicture(pictureId);

        compare(notebook.pickedPicture, "");
        compare(notebook.pictureUnder(0, 0), "");

        notebook.undo();

        compare(notebook.errorMessage, "");
    }

    function test_somethingThatIsNoPictureIsRefusedPlainly() {
        const notebook = openNotebook(newNotebookPath());

        notebook.addPicture(AppInfo.fileUrl(samplePdf));

        compare(notebook.pickedPicture, "");
        verify(notebook.errorMessage !== "");
    }

    function test_aTableIsRuledOnThePageAndComesOffAgain() {
        const notebook = openNotebook(newNotebookPath());

        notebook.addTable(3, 4);

        verify(notebook.pickedTable !== "");
        const box = notebook.pickedTableBox;
        compare(box.heights.length, 3);
        compare(box.widths.length, 4);
        compare(box.words.length, 12);
        verify(box.widths[0] > 0);
        verify(notebook.canUndo);

        notebook.undo();

        compare(notebook.pickedTableBox.tableId, undefined);
        compare(notebook.errorMessage, "");
    }

    function test_aTableIsFoundWhereItStandsAndNowhereElse() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addTable(2, 2);
        const box = notebook.pickedTableBox;
        const wide = box.widths[0] + box.widths[1];
        const tall = box.heights[0] + box.heights[1];

        const inside = notebook.tableUnder(box.columnX + (wide / 2), box.columnY + (tall / 2));
        const outside = notebook.tableUnder(box.columnX - 40, box.columnY - 40);

        compare(inside, notebook.pickedTable);
        compare(outside, "");
    }

    function test_wordsTypedIntoABoxAreKeptAndCanBeTakenBack() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;

        notebook.writeCell(tableId, 1, 0, "Monday");

        compare(notebook.wordsOfCell(tableId, 1, 0), "Monday");
        compare(notebook.wordsOfCell(tableId, 0, 0), "");

        notebook.undo();

        compare(notebook.wordsOfCell(tableId, 1, 0), "");
        compare(notebook.errorMessage, "");
    }

    function test_aRowAndAColumnComeAndGoWithoutMovingWhatWasTyped() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;
        notebook.writeCell(tableId, 1, 1, "Monday");

        notebook.addRow(tableId, 0);

        compare(notebook.pickedTableBox.heights.length, 3);
        compare(notebook.wordsOfCell(tableId, 2, 1), "Monday");

        notebook.addColumn(tableId, 0);

        compare(notebook.pickedTableBox.widths.length, 3);
        compare(notebook.wordsOfCell(tableId, 2, 2), "Monday");

        notebook.removeColumn(tableId, 0);
        notebook.removeRow(tableId, 0);

        compare(notebook.wordsOfCell(tableId, 1, 1), "Monday");
        compare(notebook.errorMessage, "");
    }

    function test_theLastRowOfATableIsSaidToBeTheLast() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addTable(1, 1);

        notebook.removeRow(notebook.pickedTable, 0);

        verify(notebook.errorMessage !== "");
        compare(notebook.pickedTableBox.heights.length, 1);
    }

    function test_aTableIsMovedAndSizedAsOneChange() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;
        // What the notebook says about the table it holds follows the table, so the numbers are
        // taken down before it is asked to change.
        const wasX = notebook.pickedTableBox.columnX;
        const wasWide = notebook.pickedTableBox.widths[0] + notebook.pickedTableBox.widths[1];
        const wasTall = notebook.pickedTableBox.heights[0] + notebook.pickedTableBox.heights[1];

        notebook.placeTable(tableId, {
            "columnX": wasX + 30,
            "columnY": notebook.pickedTableBox.columnY,
            "boxWidth": wasWide * 2,
            "boxHeight": wasTall
        });

        compare(notebook.errorMessage, "");
        const moved = notebook.pickedTableBox;
        fuzzyCompare(moved.columnX, wasX + 30, 0.01);
        fuzzyCompare(moved.widths[0] + moved.widths[1], wasWide * 2, 0.01);

        notebook.undo();

        const back = notebook.pickedTableBox;
        fuzzyCompare(back.columnX, wasX, 0.01);
        fuzzyCompare(back.widths[0] + back.widths[1], wasWide, 0.01);
    }

    function test_takingATableAwayLetsGoOfIt() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addTable(2, 2);
        const tableId = notebook.pickedTable;

        notebook.removeTable(tableId);

        compare(notebook.pickedTable, "");
        compare(notebook.tableUnder(0, 0), "");

        notebook.undo();

        compare(notebook.errorMessage, "");
    }

    function test_aSumIsPutOnThePageReadyToBeTypedInto() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);

        notebook.addEquation(tools.textStyle);

        const textId = notebook.pickedText;
        verify(textId !== "", "nothing was put down to type a sum into");
        compare(notebook.wordsOf(textId), "");

        notebook.finishText(textId, "sqrt(81) * 2", 24);
        notebook.pickedText = textId;
        notebook.solveSelection(tools.textStyle);

        compare(notebook.errorMessage, "");
        compare(notebook.wordsOf(textId), "sqrt(81) * 2 = 18");
    }

    function test_aSumPutOnThePageIsDrawnAsArithmeticIsWritten() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);

        notebook.addEquation(tools.textStyle);
        const textId = notebook.pickedText;
        notebook.finishText(textId, "1/2 + sqrt(9)", 24);

        const box = notebook.pickedBox;
        verify(box.formula, "the box does not know it holds a sum");
        verify(box.drawing.glyphs.length > 0, "nothing was laid out to draw");
        verify(box.drawing.bars.length >= 2, "neither the fraction bar nor the root roof is there");
        // A fraction stands taller than one line of type.
        verify(box.drawing.height > box.size * 2, "the drawing is no taller than plain words");
    }

    function test_aBoxOfPlainWordsIsNotDrawnAsASum() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        const sheet = notebook.canvas.sheetRect(0);

        notebook.addTextAt(sheet.x + 40, sheet.y + 40, tools.textStyle);
        const textId = notebook.pickedText;
        notebook.finishText(textId, "1/2 + sqrt(9)", 24);

        verify(!notebook.pickedBox.formula);
        // Nothing is laid out for a box that holds plain words.
        compare(notebook.pickedBox.drawing, undefined);
    }

    function test_aSumThatCannotBeReadIsStillShownAsWhatWasTyped() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);

        notebook.addEquation(tools.textStyle);
        const textId = notebook.pickedText;
        notebook.finishText(textId, "1/2 +", 24);

        verify(notebook.pickedBox.formula);
        // Half a sum lays nothing out, so what was typed is shown instead.
        compare(notebook.pickedBox.drawing.glyphs.length, 0);
        compare(notebook.wordsOf(textId), "1/2 +");
    }

    function test_aSumThatIsTypedIsWorkedOutInPlace() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        const sheet = notebook.canvas.sheetRect(0);
        notebook.addTextAt(sheet.x + 40, sheet.y + 40, tools.textStyle);
        const textId = notebook.pickedText;
        notebook.finishText(textId, "12 + 7 =", 24);
        notebook.pickedText = textId;

        notebook.solveSelection(tools.textStyle);

        compare(notebook.errorMessage, "");
        compare(notebook.wordsOf(textId), "12 + 7 = 19");

        notebook.undo();

        compare(notebook.wordsOf(textId), "12 + 7 =");
    }

    function test_aSumWithoutAnEqualsSignGetsOne() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        const sheet = notebook.canvas.sheetRect(0);
        notebook.addTextAt(sheet.x + 40, sheet.y + 40, tools.textStyle);
        const textId = notebook.pickedText;
        notebook.finishText(textId, "2(3+4)", 24);
        notebook.pickedText = textId;

        notebook.solveSelection(tools.textStyle);

        compare(notebook.wordsOf(textId), "2(3+4) = 14");
    }

    function test_whatIsNoSumIsRefusedAndLeftAlone() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        const sheet = notebook.canvas.sheetRect(0);
        notebook.addTextAt(sheet.x + 40, sheet.y + 40, tools.textStyle);
        const textId = notebook.pickedText;
        notebook.finishText(textId, "shopping list", 24);
        notebook.pickedText = textId;

        notebook.solveSelection(tools.textStyle);

        verify(notebook.errorMessage !== "");
        compare(notebook.wordsOf(textId), "shopping list");
    }

    function test_aSumDividedByNothingIsRefusedPlainly() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        const sheet = notebook.canvas.sheetRect(0);
        notebook.addTextAt(sheet.x + 40, sheet.y + 40, tools.textStyle);
        const textId = notebook.pickedText;
        notebook.finishText(textId, "5 : 0", 24);
        notebook.pickedText = textId;

        notebook.solveSelection(tools.textStyle);

        verify(notebook.errorMessage !== "");
        compare(notebook.wordsOf(textId), "5 : 0");
    }

    function test_whetherThisMachineReadsHandwritingIsSaidPlainly() {
        const notebook = openNotebook(newNotebookPath());

        compare(typeof notebook.readsHandwriting, "boolean");
        compare(notebook.readsHandwriting, Qt.platform.os === "windows");
        compare(notebook.pagesToRead, 0);
    }

    function test_searchingForNothingFindsNothing() {
        const notebook = openNotebook(newNotebookPath());
        const results = createTemporaryObject(signalSpyComponent, testCase, {
            target: notebook,
            signalName: "found"
        });

        notebook.find("   ");

        compare(results.count, 1);
        compare(results.signalArguments[0][0].length, 0);
    }

    function test_whatWasNeverWrittenIsNeverFound() {
        const notebook = openNotebook(newNotebookPath());
        draw(notebook, 40, 40);
        const results = createTemporaryObject(signalSpyComponent, testCase, {
            target: notebook,
            signalName: "found"
        });

        notebook.find("zzzqqq");

        tryCompare(results, "count", 1);
        compare(results.signalArguments[0][0].length, 0);
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

        // One sweep runs along the first stroke and back down the second.
        mousePress(canvas, 40, 40);
        mouseMove(canvas, 80, 60, -1, Qt.LeftButton);
        mouseMove(canvas, 40, 120, -1, Qt.LeftButton);
        mouseMove(canvas, 80, 140, -1, Qt.LeftButton);
        mouseRelease(canvas, 80, 140);
        const afterErasing = notebook.strokeCount;
        compare(afterErasing, 1);

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

        notebook.importDocument(AppInfo.fileUrl(samplePdf));

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
        first.importDocument(AppInfo.fileUrl(samplePdf));
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
        notebook.importDocument(AppInfo.fileUrl(samplePdf));
        tryCompare(notebook, "pageCount", 3);
        const target = temporaryDirectory + "/exported-" + notebookCount + ".pdf";
        const done = createTemporaryObject(signalSpyComponent, testCase, {
            target: notebook,
            signalName: "exported"
        });

        notebook.exportToPdf(AppInfo.fileUrl(target));

        tryCompare(done, "count", 1);
        compare(done.signalArguments[0][0], target);
        compare(notebook.exporting, false);
        compare(notebook.errorMessage, "");
    }

    function test_anExportThatCannotBeWrittenIsReported() {
        const notebook = openNotebook(newNotebookPath());
        const target = temporaryDirectory + "/missing/exported.pdf";

        notebook.exportToPdf(AppInfo.fileUrl(target));

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

    function test_aPageThatIsPutBackGoesBackWhereItStood() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        notebook.addPage();
        tryCompare(notebook, "pageCount", 3);
        notebook.renamePage(0, "One");
        notebook.renamePage(1, "Two");
        notebook.renamePage(2, "Three");
        tryVerify(() => pageNames(notebook).join(",") === "One,Two,Three", 4000);

        notebook.deletePage(1);
        tryCompare(notebook, "pageCount", 2);
        notebook.refreshTrash();
        tryCompare(notebook.trash, "count", 1);

        notebook.restoreTrashed(0);

        tryCompare(notebook, "pageCount", 3);
        // It goes back between the two it stood between, not on the end.
        tryVerify(() => pageNames(notebook).join(",") === "One,Two,Three", 4000, "the page did not go back where it stood");
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

        notebook.saveAs(AppInfo.fileUrl(target));

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

    function test_pickedWritingIsCopiedAsTextOnlyWhereItCanBeRead() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        draw(notebook, 120, 120);
        canvas.selecting = true;
        marquee(canvas, 100, 100, 200, 200);
        compare(canvas.selectedCount, 1);
        const copies = createTemporaryObject(signalSpyComponent, testCase, {
            target: notebook,
            signalName: "copiedAsText"
        });

        notebook.copySelectionAsText();

        if (notebook.readsHandwriting) {
            tryVerify(() => copies.count === 1 || notebook.errorMessage !== "");
        } else {
            tryVerify(() => notebook.errorMessage !== "");
            compare(copies.count, 0);
        }
        canvas.selecting = false;
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

    function test_sheetsNeverOverlapWhateverThePaperIs() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        notebook.addPage();
        notebook.currentPage = 0;
        notebook.continuous = true;
        const canvas = notebook.canvas;

        for (const paper of [PageOptions.A3, PageOptions.A4, PageOptions.A5, PageOptions.A3]) {
            notebook.paper = paper;
            wait(50);
            for (let sheet = 1; sheet < 3; ++sheet) {
                const above = canvas.sheetRect(sheet - 1);
                const below = canvas.sheetRect(sheet);
                verify(below.y >= above.y + above.height, "sheet " + sheet + " runs into the one above it on " + paper);
                compare(below.height, above.height, "the sheets of a section are not the same size");
            }
        }

        notebook.continuous = false;
        compare(notebook.errorMessage, "");
    }

    function test_thePageBeingReadFollowsTheNewPaperAtOnce() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        notebook.importDocument(AppInfo.fileUrl(samplePdf));
        tryCompare(notebook, "pageCount", 3);
        notebook.continuous = true;
        tryVerify(() => canvas.mediaArea.width > 0);
        const wasWide = canvas.mediaArea.width;

        notebook.paper = PageOptions.A3;

        verify(wasWide > 0);
        tryVerify(() => canvas.mediaArea.width === canvas.sheetRect(notebook.currentPage).width, 5000, "the picture never caught up with the new paper");
        notebook.continuous = false;
        compare(notebook.errorMessage, "");
    }

    function test_endlessPaperStillKeepsThePagesOfADocumentApart() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        notebook.importDocument(AppInfo.fileUrl(samplePdf));
        tryCompare(notebook, "pageCount", 3);
        notebook.continuous = true;
        notebook.currentPage = 1;
        tryVerify(() => canvas.sheetRect(1).height > 0);
        const wasTall = canvas.sheetRect(1).height;

        notebook.paper = PageOptions.Infinite;

        tryCompare(notebook, "paper", PageOptions.Infinite, 3000, "the pages refused endless paper");
        wait(300);
        compare(canvas.sheetRect(1).height, 0, "an endless page should have no sheet drawn");
        verify(canvas.sheetRect(1).y - canvas.sheetRect(0).y >= wasTall, "the endless pages stand on top of each other");
        verify(canvas.sheetRect(2).y - canvas.sheetRect(1).y >= wasTall);
        notebook.continuous = false;
        compare(notebook.errorMessage, "");
    }

    function test_onePageAtATimeAlsoFollowsTheNewPaper() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        notebook.importDocument(AppInfo.fileUrl(samplePdf));
        tryCompare(notebook, "pageCount", 3);
        verify(!notebook.continuous);
        tryVerify(() => canvas.mediaArea.width > 0);

        notebook.paper = PageOptions.A3;

        tryVerify(() => canvas.mediaArea.width === canvas.sheetRect(0).width, 5000, "the picture never caught up with the new paper");
        compare(notebook.errorMessage, "");
    }

    function test_aPageKeepsItsNameWhereverItIsCarried() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        notebook.addPage();
        const nameOf = index => notebook.pages.data(notebook.pages.index(index, 0), Qt.UserRole + 1);
        compare(nameOf(0), "Page 1");
        compare(nameOf(2), "Page 3");

        notebook.movePage(0, 2);

        compare(nameOf(2), "Page 1", "the page took the name of the place it was carried to");
        compare(nameOf(0), "Page 2");
        compare(notebook.errorMessage, "");
    }

    function test_drawingOnAnotherSheetLeavesTheViewWhereItIs() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        notebook.currentPage = 0;
        notebook.continuous = true;
        const canvas = notebook.canvas;
        canvas.fitPage();
        for (let step = 0; step < 8; ++step) {
            canvas.zoomOut();
        }
        const zoom = canvas.zoom;
        const origin = canvas.viewOrigin;
        const second = canvas.sheetRect(1);
        const on = canvas.height / 2 + 20;

        // A line started on the sheet below must not carry the view anywhere.
        mousePress(canvas, canvas.width / 2, on);
        mouseMove(canvas, canvas.width / 2 + 30, on + 10, -1, Qt.LeftButton);
        mouseRelease(canvas, canvas.width / 2 + 30, on + 10);

        compare(canvas.zoom, zoom, "drawing changed how close the page is");
        compare(canvas.viewOrigin.x, origin.x, "drawing moved the view sideways");
        compare(canvas.viewOrigin.y, origin.y, "drawing moved the view up or down");
        verify(second.height > 0);
        notebook.continuous = false;
        compare(notebook.errorMessage, "");
    }

    function test_thePaperCanBeGivenItsOwnColoursAndLines() {
        const path = newNotebookPath();
        const notebook = openNotebook(path);
        notebook.addPage();

        notebook.paperColor = "#fffbe6";
        notebook.lineColor = "#c8a2c8";
        notebook.marginColor = "#3366cc";
        notebook.lineWidth = 2.5;
        notebook.marginAt = 40;
        notebook.margin = false;

        compare(notebook.paperColor.toString(), "#fffbe6");
        compare(notebook.lineWidth, 2.5);
        compare(Math.round(notebook.marginAt), 40);
        verify(!notebook.margin);

        // The notebook lets go of its file before the same one is opened again.
        notebook.destroy();
        wait(100);
        const reopened = openNotebook(path);
        compare(reopened.paperColor.toString(), "#fffbe6", "the paper colour was not kept");
        compare(reopened.lineColor.toString(), "#c8a2c8");
        compare(reopened.marginColor.toString(), "#3366cc");
        compare(reopened.lineWidth, 2.5);
        compare(Math.round(reopened.marginAt), 40);
        verify(!reopened.margin);
        reopened.currentPage = 1;
        compare(reopened.paperColor.toString(), "#fffbe6", "the rest of the section was left behind");
        compare(reopened.errorMessage, "");
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
        notebook.saveAs(AppInfo.fileUrl(target));

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
        notebook.importDocument(AppInfo.fileUrl(samplePdf));
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
        first.importDocument(AppInfo.fileUrl(samplePdf));
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

        for (let step = 0; step < 16; ++step) {
            canvas.zoomIn();
        }
        verify(canvas.zoom > 2, "the test did not come close enough");

        tryVerify(() => canvas.mediaSize.width > faraway, 5000, "the document stayed as coarse as it was");
        compare(notebook.errorMessage, "");
    }

    function test_theSheetsAroundAreDrawnFinelyWhenTheReaderComesCloser() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        notebook.importDocument(AppInfo.fileUrl(samplePdf));
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
        notebook.importDocument(AppInfo.fileUrl(samplePdf));
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

        notebook.importDocument(AppInfo.fileUrl(samplePdf));
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
        notebook.importDocument(AppInfo.fileUrl(samplePdf));
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

    function test_aNewPageComesWithALayerToWriteOn() {
        const notebook = openNotebook(newNotebookPath());

        notebook.addPage();

        compare(notebook.currentPage, 1);
        tryCompare(notebook.layers, "count", 1, 2000, "the new page has nowhere to write");
        verify(notebook.activeLayer !== "");

        notebook.addLayer();

        tryCompare(notebook.layers, "count", 2, 2000, "a layer could not be added to a new page");
        compare(notebook.errorMessage, "");
    }

    function test_theFirstPageOfANewSectionComesWithALayer() {
        const notebook = openNotebook(newNotebookPath());

        notebook.addSection();

        compare(notebook.currentSection, 1);
        tryCompare(notebook.layers, "count", 1, 2000, "the first page of the section has nowhere to write");

        notebook.addLayer();

        tryCompare(notebook.layers, "count", 2, 2000, "a layer could not be added to a new section");
    }

    function test_theSectionANotebookIsCreatedWithIsNamedInTheReadersLanguage() {
        const notebook = openNotebook(newNotebookPath());

        compare(notebook.sectionCount, 1);
        const named = notebook.sections.data(notebook.sections.index(0, 0), Qt.UserRole + 1);
        // The part that writes the file has no words of its own, so it leaves the name empty and
        // whatever shows it names it. An empty name reaching the window would be the bug.
        verify(named !== "", "the section a notebook is created with has no name");
        compare(named, qsTr("Section 1"), "the section was not named by the window");

        notebook.renameSection(0, "Dnevnik");

        compare(notebook.sections.data(notebook.sections.index(0, 0), Qt.UserRole + 1), "Dnevnik");
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

    Component {
        id: toolsComponent

        ToolViewModel {
        }
    }
}
