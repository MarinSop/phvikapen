import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    property int notebookCount: 0

    function newNotebookPath() {
        notebookCount += 1;
        return temporaryDirectory + "/picturelayer-" + notebookCount + ".phvika";
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

    function test_aPictureCanBeTakenHoldOfAgainAfterItIsLetGo() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);
        notebook.addPicture(AppInfo.fileUrl(samplePicture));
        const box = notebook.pickedPictureBox;
        const middle = Qt.point(box.columnX + (box.boxWidth / 2), box.columnY + (box.boxHeight / 2));

        notebook.pickedPicture = "";

        // The layer must still reach the paper, or nothing can ever be picked up again.
        verify(layer.visible, "the layer went away with the picture it was holding");

        notebook.pickedPicture = notebook.pictureUnder(middle.x, middle.y);

        verify(notebook.pickedPicture !== "");
    }

    function test_theLayerStandsAsideForEveryToolButTheLoop() {
        const notebook = openNotebook(newNotebookPath());
        const tools = pickingTools();
        const layer = openLayer(notebook, tools);

        verify(layer.visible);

        tools.currentTool = ToolViewModel.Pen;

        verify(!layer.visible);
        compare(notebook.pickedPicture, "");
    }

    height: 300
    name: "PictureLayer"
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

        PictureLayer {
            height: testCase.height
            width: testCase.width
        }
    }
}
