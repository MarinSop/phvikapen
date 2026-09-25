import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function frameOn(side, group) {
        return findChild(view, "panelFrame_" + side + "_" + group);
    }

    function groupsOn(side) {
        return workspace.docks[side].groups;
    }

    function headerOf(frame) {
        return Qt.point(frame.width / 2, Math.round(10 * Theme.scale));
    }

    function carry(frame, toX, toY) {
        tryVerify(() => frame.width > 0 && frame.height > 0);
        const from = testCase.headerOf(frame);
        const at = view.mapFromItem(frame, from.x, from.y);
        mousePress(view, at.x, at.y);
        mouseMove(view, at.x + 20, at.y + 20);
        tryVerify(() => view.dragging);
        wait(Theme.calm + 50);
        mouseMove(view, toX, toY);
        mouseRelease(view, toX, toY);
    }

    function init() {
        workspace.resetWorkspace();
        wait(0);
    }

    function test_a_theContentsStandOnTheLeftToBeginWith() {
        tryVerify(() => testCase.frameOn(WorkspaceViewModel.Left, 0) !== null);
        compare(testCase.groupsOn(WorkspaceViewModel.Left)[0].panels, ["contents"]);
        compare(testCase.frameOn(WorkspaceViewModel.Right, 0), null);
    }

    function test_b_aPanelCarriedToTheRightEdgeDocksThere() {
        tryVerify(() => testCase.frameOn(WorkspaceViewModel.Left, 0) !== null);
        const frame = testCase.frameOn(WorkspaceViewModel.Left, 0);

        testCase.carry(frame, view.width - 30, view.height / 2);

        tryVerify(() => testCase.groupsOn(WorkspaceViewModel.Right).length === 1);
        compare(testCase.groupsOn(WorkspaceViewModel.Right)[0].panels, ["contents"]);
        compare(testCase.groupsOn(WorkspaceViewModel.Left).length, 0);
    }

    function test_c_aPanelCarriedOntoAnotherBecomesATab() {
        workspace.openPanel("layers");
        tryVerify(() => testCase.frameOn(WorkspaceViewModel.Right, 0) !== null);
        tryVerify(() => testCase.frameOn(WorkspaceViewModel.Left, 0) !== null);
        const layers = testCase.frameOn(WorkspaceViewModel.Right, 0);
        const contents = testCase.frameOn(WorkspaceViewModel.Left, 0);
        tryVerify(() => layers.width > 0 && layers.height > 0);
        const onto = view.mapFromItem(layers, layers.width / 2, layers.height / 2);

        testCase.carry(contents, onto.x, onto.y);

        tryVerify(() => testCase.groupsOn(WorkspaceViewModel.Right)[0].panels.length === 2);
        compare(testCase.groupsOn(WorkspaceViewModel.Right)[0].panels, ["layers", "contents"]);
    }

    function test_d_aPanelDroppedOnTheCloseTargetGoesAway() {
        tryVerify(() => testCase.frameOn(WorkspaceViewModel.Left, 0) !== null);
        const frame = testCase.frameOn(WorkspaceViewModel.Left, 0);
        const target = findChild(view, "panelCloseTarget");
        verify(target !== null);
        tryVerify(() => target.opacity === 0, 2000, "the close target is out of the way until a panel is carried");

        testCase.carry(frame, view.width / 2, Math.round(20 * Theme.scale));

        tryVerify(() => !workspace.isOpen("contents"));
        tryVerify(() => target.opacity === 0, 2000, "and it goes away again once the panel is let go");
    }

    function test_e_aPanelPutAwayComesBackFromTheMenu() {
        workspace.closePanel("contents");
        tryVerify(() => testCase.frameOn(WorkspaceViewModel.Left, 0) === null);

        workspace.openPanel("contents");

        tryVerify(() => testCase.frameOn(WorkspaceViewModel.Left, 0) !== null);
    }

    function test_f_aTabCarriedOutOfAGroupStandsOnItsOwnAgain() {
        workspace.openPanel("layers");
        workspace.dockPanel("layers", WorkspaceViewModel.Left, 0);
        tryVerify(() => testCase.groupsOn(WorkspaceViewModel.Left)[0].panels.length === 2);
        const frame = testCase.frameOn(WorkspaceViewModel.Left, 0);

        testCase.carry(frame, view.width - 30, view.height / 2);

        tryVerify(() => testCase.groupsOn(WorkspaceViewModel.Right).length === 1);
        compare(testCase.groupsOn(WorkspaceViewModel.Right)[0].panels, ["layers"]);
        compare(testCase.groupsOn(WorkspaceViewModel.Left)[0].panels, ["contents"]);
    }

    height: 640
    name: "WorkspaceView"
    visible: true
    when: windowShown
    width: 1000

    ToolViewModel {
        id: tools
    }

    SettingsViewModel {
        id: settings
    }

    NotebooksViewModel {
        id: notebooks

        directory: temporaryDirectory + "/workspace"
    }

    WorkspaceViewModel {
        id: workspace
    }

    AppActions {
        id: actions

        notebooks: notebooks
        settings: settings
        tools: tools
        workspace: workspace
    }

    WorkspaceView {
        id: view

        actions: actions
        anchors.fill: parent
        workspace: workspace
    }
}
