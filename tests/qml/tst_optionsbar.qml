import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function test_a_carriesNoNameOfTheToolInHand() {
        tools.currentTool = ToolViewModel.Pen;

        const labels = [];
        for (const child of bar.children) {
            labels.push(child.objectName);
        }
        verify(findChild(bar, "widthField") !== null);
        verify(!labels.includes("toolTitle"));
    }

    function test_b_theArrowsStepTheWidth() {
        tools.currentTool = ToolViewModel.Pen;
        tools.strokeWidth = 4;
        const field = findChild(bar, "widthField");

        findChild(field, "widthUp").clicked();
        compare(tools.strokeWidth, 4.5);

        findChild(field, "widthDown").clicked();
        findChild(field, "widthDown").clicked();
        compare(tools.strokeWidth, 3.5);
    }

    function test_c_aSliderComesUpAndGoesAwayAgain() {
        const field = findChild(bar, "widthField");
        const slider = findChild(field, "widthSlider");

        verify(!slider.opened);
        slider.open();
        wait(50);
        verify(slider.visible);

        slider.close();
        wait(50);
        verify(!slider.opened);
    }

    function test_d_theShapesAreButtonsOfTheirOwn() {
        tools.currentTool = ToolViewModel.Shape;
        wait(0);

        const box = findChild(bar, "boxShapeButton");
        const circle = findChild(bar, "circleShapeButton");
        verify(box !== null);
        verify(circle !== null);

        box.clicked();
        compare(tools.shape, ToolViewModel.Rectangle);
        verify(box.active);
        verify(!circle.active);

        circle.clicked();
        compare(tools.shape, ToolViewModel.Ellipse);
        verify(circle.active);

        box.clicked();
    }

    function test_f_whatTheEraserTakesIsOneControlWithBothModesInIt() {
        tools.currentTool = ToolViewModel.Eraser;
        wait(0);
        const modes = findChild(bar, "eraseModeField");

        verify(modes !== null);
        verify(modes.visible);
        compare(modes.count, 2);
        // What is set is shown back, and choosing the other mode takes.
        compare(modes.currentIndex, 0);

        modes.activated(1);

        compare(tools.eraserMode, ToolViewModel.WholeStroke);
        compare(modes.currentIndex, 1);

        modes.activated(0);

        compare(tools.eraserMode, ToolViewModel.Touched);
        tools.currentTool = ToolViewModel.Pen;
        wait(0);
        verify(!modes.visible);
    }

    function test_e_theWidthIsHiddenForToolsThatDrawNothing() {
        tools.currentTool = ToolViewModel.Hand;
        wait(0);

        verify(!findChild(bar, "widthField").visible);

        tools.currentTool = ToolViewModel.Eraser;
        wait(0);
        verify(findChild(bar, "widthField").visible);
        tools.currentTool = ToolViewModel.Pen;
    }

    height: 60
    name: "OptionsBar"
    visible: true
    when: windowShown
    width: 900

    ToolViewModel {
        id: tools
    }

    NotebooksViewModel {
        id: notebooks

        directory: temporaryDirectory + "/optionsbar"
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

    OptionsBar {
        id: bar

        actions: actions
        anchors.fill: parent
        tools: tools
    }
}
