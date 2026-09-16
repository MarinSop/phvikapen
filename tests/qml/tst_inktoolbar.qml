import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function test_a_instantiates() {
        verify(toolBar.width > 0);
        verify(toolBar.height > 0);
    }

    function test_b_followsTheToolModel() {
        tools.strokeWidth = 7;
        compare(toolBar.tools.strokeWidth, 7);

        tools.currentTool = ToolViewModel.Eraser;
        compare(toolBar.tools.currentTool, ToolViewModel.Eraser);

        tools.currentTool = ToolViewModel.Pen;
    }

    height: 64
    name: "InkToolBar"
    visible: true
    when: windowShown
    width: 640

    ToolViewModel {
        id: tools
    }

    InkToolBar {
        id: toolBar

        anchors.fill: parent
        tools: tools
    }
}
