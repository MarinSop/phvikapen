import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function test_a_instantiates() {
        verify(palette.width > 0);
        verify(palette.height > 0);
    }

    function test_b_everyToolHasAButtonWithATooltip() {
        const names = ["selectTool", "handTool", "penTool", "highlighterTool", "shapeTool", "eraserTool"];
        for (const name of names) {
            const button = findChild(palette, name);
            verify(button !== null, name + " is missing");
            verify(button.toolName.length > 0);
            verify(button.tooltipText.indexOf(button.toolName) >= 0);
        }
    }

    function test_c_theToolThatIsPickedIsMarked() {
        tools.currentTool = ToolViewModel.Eraser;

        verify(findChild(palette, "eraserTool").checked);
        verify(!findChild(palette, "penTool").checked);

        tools.currentTool = ToolViewModel.Pen;
        tools.shape = ToolViewModel.Freehand;

        verify(findChild(palette, "penTool").checked);
        verify(!findChild(palette, "shapeTool").checked);
    }

    function test_d_theShapeToolPicksTheShapeAndThePenPutsItBack() {
        findChild(palette, "shapeTool").action.trigger();

        compare(tools.currentTool, ToolViewModel.Pen);
        verify(tools.shape !== ToolViewModel.Freehand);

        findChild(palette, "penTool").action.trigger();

        compare(tools.currentTool, ToolViewModel.Pen);
        compare(tools.shape, ToolViewModel.Freehand);
    }

    function test_e2_theKeysPickTheTools() {
        findChild(palette, "penTool").action.trigger();

        keyClick(Qt.Key_V);
        compare(tools.currentTool, ToolViewModel.Selection);

        keyClick(Qt.Key_E);
        compare(tools.currentTool, ToolViewModel.Eraser);

        keyClick(Qt.Key_M);
        compare(tools.currentTool, ToolViewModel.Highlighter);

        keyClick(Qt.Key_P);
        compare(tools.currentTool, ToolViewModel.Pen);
        compare(tools.shape, ToolViewModel.Freehand);
    }

    function test_e_theHandToolPansInsteadOfDrawing() {
        findChild(palette, "handTool").action.trigger();

        compare(tools.currentTool, ToolViewModel.Hand);

        findChild(palette, "penTool").action.trigger();
    }

    height: 400
    name: "ToolPalette"
    visible: true
    when: windowShown
    width: 200

    ToolViewModel {
        id: tools
    }

    NotebooksViewModel {
        id: notebooks

        directory: temporaryDirectory + "/palette"
    }

    SettingsViewModel {
        id: settings
    }

    AppActions {
        id: actions

        notebooks: notebooks
        settings: settings
        tools: tools
    }

    ToolPalette {
        id: palette

        actions: actions
        anchors.fill: parent
    }
}
