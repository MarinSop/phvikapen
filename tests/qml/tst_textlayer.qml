import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    property int notebookCount: 0

    function boxes(notebook) {
        return createTemporaryObject(rowsComponent, testCase, {
            model: notebook.texts
        });
    }

    function newNotebookPath() {
        notebookCount += 1;
        return temporaryDirectory + "/textlayer-" + notebookCount + ".phvika";
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

    function placeOnPage(notebook, x, y) {
        const sheet = notebook.canvas.sheetRect(0);
        return Qt.point(sheet.x + x, sheet.y + y);
    }

    function test_aBoxLeftEmptyIsNeverKept() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        const at = placeOnPage(notebook, 40, 40);

        notebook.addTextAt(at.x, at.y, tools.textStyle);
        const rows = boxes(notebook);
        compare(rows.count, 1);
        verify(!notebook.canUndo);

        notebook.finishText(notebook.pickedText, "", 20);
        compare(rows.count, 0);
        verify(!notebook.canUndo);
        compare(notebook.pickedText, "");
    }

    function test_aBoxThatIsTypedInBecomesPartOfThePage() {
        const path = newNotebookPath();
        const notebook = openNotebook(path);
        const tools = createTemporaryObject(toolsComponent, testCase);
        const at = placeOnPage(notebook, 30, 50);

        notebook.addTextAt(at.x, at.y, tools.textStyle);
        notebook.finishText(notebook.pickedText, "Hello paper", 24);

        const rows = boxes(notebook);
        compare(rows.count, 1);
        compare(rows.itemAt(0).text, "Hello paper");
        verify(notebook.canUndo);
        compare(notebook.errorMessage, "");

        notebook.destroy();
        wait(0);
        const reopened = openNotebook(path);
        const kept = boxes(reopened);
        compare(kept.count, 1);
        compare(kept.itemAt(0).text, "Hello paper");
    }

    function test_oneUndoTakesAWholeBoxAway() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        const at = placeOnPage(notebook, 30, 50);
        notebook.addTextAt(at.x, at.y, tools.textStyle);
        notebook.finishText(notebook.pickedText, "Gone soon", 24);
        const rows = boxes(notebook);

        notebook.undo();
        compare(rows.count, 0);

        notebook.redo();
        compare(rows.count, 1);
        compare(rows.itemAt(0).text, "Gone soon");
    }

    function test_aBoxWearsTheFaceTheBarChose() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        tools.textBold = true;
        tools.textSize = 22;
        tools.textColor = "#ff2211";
        const at = placeOnPage(notebook, 20, 20);

        notebook.addTextAt(at.x, at.y, tools.textStyle);
        notebook.finishText(notebook.pickedText, "Bold words", 30);

        const rows = boxes(notebook);
        compare(rows.count, 1);
        compare(rows.itemAt(0).bold, true);
        compare(rows.itemAt(0).size, 22);
        compare(rows.itemAt(0).color, "#ff2211");
    }

    function test_theFaceOfABoxCanBeChangedAfterwards() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        const at = placeOnPage(notebook, 20, 20);
        notebook.addTextAt(at.x, at.y, tools.textStyle);
        notebook.finishText(notebook.pickedText, "Plain words", 30);
        const rows = boxes(notebook);
        const textId = rows.itemAt(0).textId;

        tools.useTextStyle(notebook.styleOfText(textId));
        tools.textItalic = true;
        notebook.styleText(textId, tools.textStyle);

        compare(rows.itemAt(0).italic, true);
        compare(rows.itemAt(0).text, "Plain words");

        notebook.undo();
        compare(rows.itemAt(0).italic, false);
    }

    function test_aBoxGoesWhereItIsDraggedAndTakesTheWidthItIsGiven() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        const at = placeOnPage(notebook, 20, 20);
        notebook.addTextAt(at.x, at.y, tools.textStyle);
        notebook.finishText(notebook.pickedText, "Movable", 30);
        const rows = boxes(notebook);
        const textId = rows.itemAt(0).textId;

        notebook.placeText(textId, at.x + 60, at.y + 90, 150, 44);

        fuzzyCompare(rows.itemAt(0).columnX, at.x + 60, 0.01);
        fuzzyCompare(rows.itemAt(0).columnY, at.y + 90, 0.01);
        fuzzyCompare(rows.itemAt(0).boxWidth, 150, 0.01);
    }

    function test_aTapWithTheTextToolPutsABoxOnThePaper() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        tools.currentTool = ToolViewModel.Text;
        const layer = createTemporaryObject(layerComponent, testCase, {
            canvas: notebook.canvas,
            notebook: notebook,
            tools: tools
        });
        const at = placeOnPage(notebook, 40, 60);
        const onScreen = Qt.point((at.x - notebook.canvas.viewOrigin.x) * notebook.canvas.zoom, (at.y - notebook.canvas.viewOrigin.y) * notebook.canvas.zoom);

        mouseClick(layer, onScreen.x, onScreen.y);

        const rows = boxes(notebook);
        compare(rows.count, 1);
        verify(notebook.pickedText !== "");
        fuzzyCompare(rows.itemAt(0).columnX, at.x, 1.0);
        fuzzyCompare(rows.itemAt(0).columnY, at.y, 1.0);
        notebook.pickedText = "";
    }

    function test_typingInTheEditorPutsTheWordsOnThePage() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        tools.currentTool = ToolViewModel.Text;
        const layer = createTemporaryObject(layerComponent, testCase, {
            canvas: notebook.canvas,
            notebook: notebook,
            tools: tools
        });
        const at = placeOnPage(notebook, 40, 60);
        const onScreen = Qt.point((at.x - notebook.canvas.viewOrigin.x) * notebook.canvas.zoom, (at.y - notebook.canvas.viewOrigin.y) * notebook.canvas.zoom);
        mouseClick(layer, onScreen.x, onScreen.y);

        const editor = findChild(layer, "textEditor");
        verify(editor !== null);
        editor.text = "Typed here";
        mouseClick(layer, onScreen.x, onScreen.y + 120);

        const rows = boxes(notebook);
        compare(rows.count, 1);
        compare(rows.itemAt(0).text, "Typed here");
        compare(notebook.pickedText, "");
        notebook.pickedText = "";
    }

    function test_aBoxNobodyTypedInIsGivenUp() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        tools.currentTool = ToolViewModel.Text;
        const layer = createTemporaryObject(layerComponent, testCase, {
            canvas: notebook.canvas,
            notebook: notebook,
            tools: tools
        });
        const at = placeOnPage(notebook, 40, 60);
        const onScreen = Qt.point((at.x - notebook.canvas.viewOrigin.x) * notebook.canvas.zoom, (at.y - notebook.canvas.viewOrigin.y) * notebook.canvas.zoom);

        mouseClick(layer, onScreen.x, onScreen.y);
        mouseClick(layer, onScreen.x, onScreen.y + 120);

        const rows = boxes(notebook);
        compare(rows.count, 0);
        verify(!notebook.canUndo);
        compare(notebook.pickedText, "");
    }

    function test_handwritingBecomesTextOnlyWhereItCanBeRead() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        const canvas = notebook.canvas;
        mousePress(canvas, 120, 120);
        mouseMove(canvas, 140, 130, -1, Qt.LeftButton);
        mouseMove(canvas, 160, 140, -1, Qt.LeftButton);
        mouseRelease(canvas, 160, 140);
        compare(notebook.strokeCount, 1);
        canvas.selecting = true;
        mousePress(canvas, 100, 100);
        mouseMove(canvas, 150, 150, -1, Qt.LeftButton);
        mouseMove(canvas, 200, 200, -1, Qt.LeftButton);
        mouseRelease(canvas, 200, 200);
        compare(canvas.selectedCount, 1);

        notebook.convertSelectionToText(tools.textStyle);

        const rows = boxes(notebook);
        if (notebook.readsHandwriting) {
            tryVerify(() => rows.count === 1 || notebook.errorMessage !== "");
        } else {
            tryVerify(() => notebook.errorMessage !== "");
            compare(rows.count, 0);
            compare(notebook.strokeCount, 1);
        }
        canvas.selecting = false;
    }

    function test_nothingHappensWhenThereIsNothingToConvert() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);

        notebook.convertSelectionToText(tools.textStyle);

        compare(boxes(notebook).count, 0);
        compare(notebook.errorMessage, "");
    }

    function test_theCanvasLeavesThePenAloneWhileTextIsBeingPlaced() {
        const notebook = openNotebook(newNotebookPath());
        const canvas = notebook.canvas;
        canvas.typing = true;

        mousePress(canvas, 60, 60);
        mouseMove(canvas, 90, 80, -1, Qt.LeftButton);
        mouseRelease(canvas, 90, 80);

        compare(notebook.strokeCount, 0);
        canvas.typing = false;
    }

    function test_clearingThePageTakesTheTextWithItAndUndoBringsItBack() {
        const notebook = openNotebook(newNotebookPath());
        const tools = createTemporaryObject(toolsComponent, testCase);
        const at = placeOnPage(notebook, 20, 20);
        notebook.addTextAt(at.x, at.y, tools.textStyle);
        notebook.finishText(notebook.pickedText, "Swept away", 30);
        const rows = boxes(notebook);

        notebook.clearPage();
        compare(rows.count, 0);

        notebook.undo();
        compare(rows.count, 1);
        compare(rows.itemAt(0).text, "Swept away");
    }

    height: 300
    name: "TextLayer"
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

        TextLayer {
            height: testCase.height
            width: testCase.width
        }
    }

    Component {
        id: rowsComponent

        Repeater {
            delegate: Item {
                required property bool bold
                required property real boxWidth
                required property color color
                required property real columnX
                required property real columnY
                required property bool italic
                required property real size
                required property string text
                required property string textId
            }
        }
    }
}
