import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function newTools() {
        return createTemporaryObject(toolsComponent, testCase);
    }

    function test_a_startsWithTheFirstPen() {
        const tools = testCase.newTools();

        compare(tools.currentTool, ToolViewModel.Pen);
        compare(tools.pen, 0);
        compare(tools.penCount, 3);
        verify(tools.palette.length >= 6);
        verify(tools.strokeWidth > 0);
        compare(tools.strokeColor, tools.colorOfPen(0));
        verify(tools.pressureSensitive);
    }

    function test_b_eachPenKeepsItsOwnColourAndWidth() {
        const tools = testCase.newTools();

        tools.strokeColor = "#112233";
        tools.strokeWidth = 4;
        tools.pen = 1;

        verify(tools.strokeColor.toString() !== "#112233");
        tools.strokeWidth = 9;
        compare(tools.widthOfPen(1), 9);

        tools.pen = 0;
        compare(tools.strokeColor.toString(), "#112233");
        compare(tools.strokeWidth, 4);
    }

    function test_c_theHighlighterIsWideFlatAndTranslucent() {
        const tools = testCase.newTools();
        const penWidth = tools.strokeWidth;

        tools.currentTool = ToolViewModel.Highlighter;

        verify(tools.strokeWidth > penWidth);
        verify(tools.strokeColor.a < 1);
        verify(!tools.pressureSensitive);

        tools.currentTool = ToolViewModel.Pen;
        compare(tools.strokeWidth, penWidth);
        verify(tools.pressureSensitive);
    }

    function test_d_keepsSizesWithinBounds() {
        const tools = testCase.newTools();

        tools.strokeWidth = 1000;
        verify(tools.strokeWidth <= 24);

        tools.strokeWidth = -5;
        verify(tools.strokeWidth >= 0.5);

        tools.eraserRadius = 1000;
        verify(tools.eraserRadius <= 40);

        tools.eraserRadius = 0;
        verify(tools.eraserRadius >= 4);
    }

    function test_e_remembersTheToolsForNextTime() {
        const before = testCase.newTools();
        before.pen = 2;
        before.strokeColor = "#445566";
        before.strokeWidth = 6;
        before.eraserRadius = 20;
        before.currentTool = ToolViewModel.Highlighter;
        before.destroy();
        wait(0);

        const after = testCase.newTools();

        compare(after.currentTool, ToolViewModel.Highlighter);
        compare(after.pen, 2);
        compare(after.eraserRadius, 20);
        compare(after.colorOfPen(2).toString(), "#445566");
        compare(after.widthOfPen(2), 6);
    }

    function test_f_aPickedColourIsUsedByThePenAndTheHighlighter() {
        const tools = testCase.newTools();
        tools.currentTool = ToolViewModel.Pen;
        tools.currentTool = ToolViewModel.ColourPicker;

        tools.usePickedColour("#20a040");

        compare(tools.currentTool, ToolViewModel.Pen);
        compare(tools.colorOfPen(tools.pen).toString(), "#20a040");

        tools.currentTool = ToolViewModel.Highlighter;
        compare(tools.strokeColor.r, tools.colorOfPen(tools.pen).r);
        compare(tools.strokeColor.g, tools.colorOfPen(tools.pen).g);
        verify(tools.strokeColor.a < 1);
        tools.currentTool = ToolViewModel.Pen;
    }

    function test_g_thePickerHandsTheToolBackToTheOneBeforeIt() {
        const tools = testCase.newTools();
        tools.currentTool = ToolViewModel.Highlighter;
        tools.currentTool = ToolViewModel.ColourPicker;

        tools.usePickedColour("#334455");

        compare(tools.currentTool, ToolViewModel.Highlighter);
        tools.currentTool = ToolViewModel.Pen;
    }

    function test_z_theShapeIsRememberedForTheNextTime() {
        const tools = testCase.newTools();
        compare(tools.shape, ToolViewModel.Rectangle);

        tools.shape = ToolViewModel.Ellipse;

        const later = testCase.newTools();
        compare(later.shape, ToolViewModel.Ellipse);
        later.shape = ToolViewModel.Rectangle;
    }

    name: "ToolViewModel"

    Component {
        id: toolsComponent

        ToolViewModel {
        }
    }
}
