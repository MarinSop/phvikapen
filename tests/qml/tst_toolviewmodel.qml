import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function test_a_startsWithThePen() {
        compare(tools.currentTool, ToolViewModel.Pen);
        verify(tools.strokeWidth > 0);
        verify(tools.strokeColor.a > 0);
    }

    function test_b_switchesTool() {
        tools.currentTool = ToolViewModel.Eraser;
        compare(tools.currentTool, ToolViewModel.Eraser);

        tools.currentTool = ToolViewModel.Pen;
        compare(tools.currentTool, ToolViewModel.Pen);
    }

    function test_c_changesTheStrokeWidth() {
        tools.strokeWidth = 5;
        compare(tools.strokeWidth, 5);
    }

    name: "ToolViewModel"

    ToolViewModel {
        id: tools
    }
}
