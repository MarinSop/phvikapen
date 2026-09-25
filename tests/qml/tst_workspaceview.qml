import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    readonly property int kindStack: 1

    function carry(frame, toX, toY) {
        tryVerify(() => frame.width > 0 && frame.height > 0);
        const at = view.mapFromItem(frame, frame.width / 2, Math.round(10 * Theme.scale));
        mousePress(view, at.x, at.y);
        mouseMove(view, at.x + 20, at.y + 20);
        tryVerify(() => view.dragging);
        wait(Theme.calm + 50);
        mouseMove(view, toX, toY);
        mouseRelease(view, toX, toY);
    }

    function cornerOf(item) {
        return view.mapFromItem(item, 0, 0);
    }

    function frameOf(panelId) {
        const path = testCase.pathOf(panelId);
        return path === null ? null : findChild(view, "panelFrame_" + path);
    }

    function init() {
        workspace.resetWorkspace();
        // The frames of the layout before this one are on their way out; measuring one of those
        // would measure where it used to be.
        wait(80);
    }

    function pathOf(panelId) {
        const look = node => {
            if (node.kind === testCase.kindStack && node.panels.indexOf(panelId) >= 0) {
                return node.path;
            }
            for (const child of node.children) {
                const found = look(child);
                if (found !== null) {
                    return found;
                }
            }
            return null;
        };
        return look(workspace.layout);
    }

    function stackOf(panelId) {
        const path = testCase.pathOf(panelId);
        let node = workspace.layout;
        for (const step of path.split(".")) {
            node = node.children[Number(step)];
        }
        return node;
    }

    function test_a_theSectionsAndThePagesStandOnTheirOwn() {
        tryVerify(() => testCase.frameOf("sections") !== null);
        tryVerify(() => testCase.frameOf("pages") !== null);
        compare(testCase.stackOf("sections").panels, ["sections"]);
        compare(testCase.stackOf("pages").panels, ["pages"]);
    }

    function test_b_aPanelCarriedToTheRightEdgeOfTheSheetDocksThere() {
        tryVerify(() => testCase.frameOf("sections") !== null);
        const frame = testCase.frameOf("sections");

        testCase.carry(frame, view.width - 8, view.height / 2);

        tryVerify(() => {
            const moved = testCase.frameOf("sections");
            return moved !== null && testCase.cornerOf(moved).x > view.width / 2;
        }, 2000, "the panel did not move to the right of the window");
    }

    function test_c_aPanelCarriedOntoTheTabsOfAnotherBecomesATab() {
        tryVerify(() => testCase.frameOf("pages") !== null);
        const pages = testCase.frameOf("pages");
        const sections = testCase.frameOf("sections");
        const tab = findChild(pages, "panelTab_pages");
        verify(tab !== null);
        const onto = view.mapFromItem(tab, tab.width / 2, tab.height / 2);

        testCase.carry(sections, onto.x, onto.y);

        tryVerify(() => testCase.stackOf("sections").panels.length === 2, 2000, "the two did not become one group");
        compare(testCase.stackOf("pages").panels.length, 2);
    }

    function test_d_aPanelCarriedIntoTheBodyOfAnotherStandsBelowIt() {
        tryVerify(() => testCase.frameOf("pages") !== null);
        const pages = testCase.frameOf("pages");
        const sections = testCase.frameOf("sections");
        const onto = view.mapFromItem(pages, pages.width / 2, pages.height / 2);

        testCase.carry(sections, onto.x, onto.y);

        tryVerify(() => {
            const moved = testCase.frameOf("sections");
            const stayed = testCase.frameOf("pages");
            return moved !== null && stayed !== null && testCase.cornerOf(moved).y > testCase.cornerOf(stayed).y;
        }, 2000, "the panel did not land under the other one");
    }

    function test_e_aPanelDroppedOnTheCloseTargetGoesAway() {
        tryVerify(() => testCase.frameOf("sections") !== null);
        const frame = testCase.frameOf("sections");
        const target = findChild(view, "panelCloseTarget");
        verify(target !== null);
        tryVerify(() => target.opacity === 0, 2000, "the close target is out of the way until a panel is carried");

        testCase.carry(frame, view.width / 2, Math.round(20 * Theme.scale));

        tryVerify(() => !workspace.isOpen("sections"));
        tryVerify(() => target.opacity === 0, 2000, "and it goes away again once the panel is let go");
    }

    function test_f_aPanelPutAwayComesBackFromTheMenu() {
        workspace.closePanel("sections");
        tryVerify(() => testCase.frameOf("sections") === null);

        workspace.openPanel("sections");

        tryVerify(() => testCase.frameOf("sections") !== null);
    }

    function test_g_theSheetKeepsSomewhereToStandWhilePanelsMoveAround() {
        tryVerify(() => view.middleSlot !== null);

        workspace.openPanel("layers");
        tryVerify(() => testCase.frameOf("layers") !== null);

        tryVerify(() => view.middleSlot !== null && view.middleSlot.width > 0 && view.middleSlot.height > 0, 2000, "the sheet lost its place");
    }

    height: 640
    name: "WorkspaceView"
    visible: true
    when: windowShown
    width: 1100

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
