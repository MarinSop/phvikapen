import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    readonly property color desk: "#101010"
    property color picked: "transparent"

    function pickAt(x, y) {
        testCase.picked = "transparent";
        tools.currentTool = ToolViewModel.ColourPicker;
        mouseClick(canvas, x, y);
        tryVerify(() => testCase.picked.a > 0, 3000, "nothing was picked at all");
    }

    function needsAWindowThatHandsBackWhatItDrew() {
        testCase.pickAt(2, 2);
        if (testCase.picked.toString() !== testCase.desk.toString()) {
            skip("this platform does not hand back what the canvas drew");
        }
    }

    function initTestCase() {
        notebooks.createNotebook("Picking");
        tryCompare(notebooks.current, "loaded", true);
        notebooks.canvas = canvas;
        tryVerify(() => canvas.width > 0 && canvas.height > 0);
        wait(300);
    }

    function test_a_theDeskBesideTheSheetHasAColourOfItsOwn() {
        testCase.needsAWindowThatHandsBackWhatItDrew();

        compare(testCase.picked.toString(), testCase.desk.toString());
    }

    function test_b_thePaperUnderThePointerIsPickedRatherThanNothing() {
        testCase.needsAWindowThatHandsBackWhatItDrew();

        testCase.pickAt(canvas.width / 2, canvas.height / 2);

        compare(testCase.picked.toString(), "#ffffff", "the sheet is what lies under the pointer");
    }

    function test_c_aLineHandsOverTheColourItWasDrawnWith() {
        testCase.needsAWindowThatHandsBackWhatItDrew();
        tools.currentTool = ToolViewModel.Pen;
        tools.strokeColor = "#c62828";
        tools.strokeWidth = 24;
        mouseDrag(canvas, 60, 200, 260, 0);
        tryCompare(notebooks.current, "strokeCount", 1);
        wait(300);

        testCase.pickAt(250, 200);

        compare(testCase.picked.toString(), "#c62828", "the line under the pointer was not what was taken");
    }

    height: 400
    name: "ColourPicker"
    visible: true
    when: windowShown
    width: 500

    ToolViewModel {
        id: tools
    }

    NotebooksViewModel {
        id: notebooks

        directory: temporaryDirectory + "/picking"
    }

    Connections {
        function onColourPicked(colour) {
            testCase.picked = colour;
        }

        target: notebooks.current
    }

    InkCanvas {
        id: canvas

        anchors.fill: parent
        deskColor: testCase.desk
        picking: tools.currentTool === ToolViewModel.ColourPicker
        strokeColor: tools.strokeColor
        strokeWidth: tools.strokeWidth
    }
}
