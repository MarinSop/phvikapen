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

    function aimAt(frame, toX, toY) {
        tryVerify(() => frame.width > 0 && frame.height > 0);
        const at = view.mapFromItem(frame, frame.width / 2, Math.round(10 * Theme.scale));
        mousePress(view, at.x, at.y);
        mouseMove(view, at.x + 20, at.y + 20);
        tryVerify(() => view.dragging);
        wait(Theme.calm + 50);
        mouseMove(view, toX, toY);
    }

    function anyFrameOf(panelId) {
        const docked = testCase.frameOf(panelId);
        if (docked !== null) {
            return docked;
        }
        for (const window of workspace.floating) {
            if (window.node.panels.indexOf(panelId) >= 0) {
                return findChild(view, "panelFrame_" + window.path);
            }
        }
        return null;
    }

    function windowOf(panelId) {
        for (const window of workspace.floating) {
            if (window.node.panels.indexOf(panelId) >= 0) {
                return findChild(view, "panelWindow_" + window.path);
            }
        }
        return null;
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
        testCase.settled("sections");
        testCase.settled("pages");
    }

    function settled(panelId) {
        tryVerify(() => {
            const frame = testCase.anyFrameOf(panelId);
            return frame !== null && frame.width > 0 && frame.height > 0;
        }, 2000, "the frame of " + panelId + " never took a size");
        return testCase.anyFrameOf(panelId);
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

    function test_h_theLineForANewTabStaysClearOfTheTabItWouldFollow() {
        tryVerify(() => testCase.frameOf("pages") !== null);
        const pages = testCase.frameOf("pages");
        const sections = testCase.frameOf("sections");
        const tab = findChild(pages, "panelTab_pages");
        verify(tab !== null);
        const onto = view.mapFromItem(tab, Math.round(4 * Theme.scale), tab.height / 2);

        testCase.aimAt(sections, onto.x, onto.y);

        compare(view.dropKind, "tab");
        const left = testCase.cornerOf(tab).x;
        verify(view.dropHint.width > 0, "no line was drawn for the tab it would become");
        verify(view.dropHint.x + view.dropHint.width <= left + 1, "the line runs over the tab beside it");
        mouseRelease(view, onto.x, onto.y);
    }

    function test_ha_theLineForDroppingIntoAStackedPanelSitsOnTheDividerItWouldMake() {
        tryVerify(() => testCase.frameOf("pages") !== null);
        workspace.openPanel("layers");
        const layers = testCase.settled("layers");
        const pages = testCase.settled("pages");
        wait(Theme.calm);
        const onto = view.mapFromItem(pages, pages.width / 2, pages.height / 2);

        testCase.aimAt(layers, onto.x, onto.y);

        compare(view.dropKind, "edge");
        const foot = testCase.cornerOf(pages).y + pages.height;
        verify(Math.abs(view.dropHint.y - (foot - 3)) <= 2, "the line was not drawn on the edge the panel would land against");
        mouseRelease(view, onto.x, onto.y);
    }

    function test_i_aPanelCarriedOverTheSheetBecomesAWindowOfItsOwn() {
        tryVerify(() => view.middleSlot !== null && view.middleSlot.width > 0);
        const sections = testCase.settled("sections");
        const sheet = view.sheetRect();

        testCase.carry(sections, sheet.x + (sheet.width / 2), sheet.y + (sheet.height / 2));

        tryVerify(() => workspace.isAfloat("sections"), 2000, "the panel did not come loose");
        tryVerify(() => testCase.windowOf("sections") !== null);
        compare(testCase.pathOf("sections"), null, "it is no longer part of the docked tree");
    }

    function test_j_aPanelCarriedToACornerOfTheSheetSitsInThatCorner() {
        tryVerify(() => view.middleSlot !== null && view.middleSlot.width > 0);
        const sections = testCase.settled("sections");
        const sheet = view.sheetRect();
        const reach = Math.round(40 * Theme.scale);

        testCase.aimAt(sections, sheet.x + reach, sheet.y + reach);
        compare(view.dropKind, "corner");
        mouseRelease(view, sheet.x + reach, sheet.y + reach);

        tryVerify(() => workspace.isAfloat("sections"), 2000, "the panel did not come loose");
        tryVerify(() => testCase.windowOf("sections") !== null);
        const window = testCase.windowOf("sections");
        verify(Math.abs(window.x - sheet.x) <= 2, "the window did not sit against the left of the sheet");
        verify(Math.abs(window.y - sheet.y) <= 2, "the window did not sit against the top of the sheet");
        verify(window.width < sheet.width / 2, "the window was stretched along the edge");
    }

    function test_k_aWindowCarriedBackOntoAnEdgeDocksAgain() {
        tryVerify(() => view.middleSlot !== null && view.middleSlot.width > 0);
        const sections = testCase.settled("sections");
        const sheet = view.sheetRect();

        testCase.carry(sections, sheet.x + (sheet.width / 2), sheet.y + (sheet.height / 2));
        tryVerify(() => workspace.isAfloat("sections"), 2000);
        const loose = testCase.settled("sections");

        testCase.carry(loose, view.width - 6, view.height / 2);

        tryVerify(() => !workspace.isAfloat("sections"), 2000, "the window did not dock again");
        tryVerify(() => testCase.pathOf("sections") !== null);
    }

    function test_l_aPanelSetFreeGetsAWindowOfItsOwnAndGivesItUpAgain() {
        tryVerify(() => view.middleSlot !== null && view.middleSlot.width > 0);
        const sections = testCase.settled("sections");
        const sheet = view.sheetRect();
        testCase.carry(sections, sheet.x + (sheet.width / 2), sheet.y + (sheet.height / 2));
        tryVerify(() => workspace.isAfloat("sections"), 2000, "the panel did not come loose");

        workspace.setPanelLoose("~0", true);

        // A panel set free is no longer drawn over the sheet: it stands in a window the machine owns.
        tryVerify(() => testCase.windowOf("sections") === null, 2000, "it is still over the sheet");
        const holder = findChild(view, "loosePanelHolder_~0");
        verify(holder !== null, "no window of its own was made");
        tryVerify(() => holder.own !== null && holder.own.visible, 2000, "the window of its own never showed");
        compare(holder.own.objectName, "loosePanelWindow_~0");

        workspace.setPanelLoose("~0", false);

        tryVerify(() => testCase.windowOf("sections") !== null, 2000, "it did not come back over the sheet");
        verify(workspace.isAfloat("sections"), "bringing it back must not dock it");
    }

    // A tab picked up by itself, rather than by the header of the group it stands in.
    function carryTab(frame, panelId, toX, toY) {
        const tab = findChild(frame, "panelTab_" + panelId);
        verify(tab !== null, "there is no tab for " + panelId);
        tryVerify(() => tab.width > 0);
        const at = view.mapFromItem(tab, tab.width / 2, tab.height / 2);
        mousePress(view, at.x, at.y);
        mouseMove(view, at.x + 20, at.y + 20);
        tryVerify(() => view.dragging, 2000, "the tab was never picked up");
        wait(Theme.calm + 50);
        mouseMove(view, toX, toY);
        return tab;
    }

    function test_m_aTabIsPickedUpOnItsOwnAndCarriedOut() {
        tryVerify(() => view.middleSlot !== null && view.middleSlot.width > 0);
        const pages = testCase.settled("pages");
        const sections = testCase.settled("sections");
        // Put the two into one group, so there is a tab behind the one in front.
        const onto = view.mapFromItem(pages, pages.width / 2, Math.round(10 * Theme.scale));
        testCase.carry(sections, onto.x, onto.y);
        tryVerify(() => testCase.pathOf("sections") === testCase.pathOf("pages"), 2000, "the two did not join");
        const together = testCase.settled("pages");
        compare(testCase.stackOf("pages").panels.length, 2);

        // Carry the tab that is not in front out onto the far edge of the window.
        const behind = testCase.stackOf("pages").panels.slice()[0];
        testCase.carryTab(together, behind, view.width - 6, view.height / 2);
        compare(view.dropPanel, behind, "the whole group was carried instead of the one tab");
        mouseRelease(view, view.width - 6, view.height / 2);

        tryVerify(() => testCase.stackOf(behind).panels.length === 1, 2000, "the tab did not come out on its own");
        verify(testCase.pathOf(behind) !== testCase.pathOf(behind === "pages" ? "sections" : "pages"), "the two are still together");
    }

    function test_n_aTabIsDraggedPastAnotherInsideItsOwnGroup() {
        tryVerify(() => view.middleSlot !== null && view.middleSlot.width > 0);
        const pages = testCase.settled("pages");
        const sections = testCase.settled("sections");
        const onto = view.mapFromItem(pages, pages.width / 2, Math.round(10 * Theme.scale));
        testCase.carry(sections, onto.x, onto.y);
        tryVerify(() => testCase.pathOf("sections") === testCase.pathOf("pages"), 2000);
        const together = testCase.settled("pages");
        // A list from a property is not a snapshot: it follows the layout as it changes.
        const was = testCase.stackOf("pages").panels.slice();
        compare(was.length, 2);

        // Carry the first tab past the second, inside the same strip.
        const second = findChild(together, "panelTab_" + was[1]);
        verify(second !== null);
        const past = view.mapFromItem(second, second.width - 2, second.height / 2);
        testCase.carryTab(together, was[0], past.x, past.y);
        compare(view.dropKind, "tab", "no place among the tabs was offered");
        mouseRelease(view, past.x, past.y);

        tryVerify(() => testCase.stackOf(was[0]).panels[1] === was[0], 2000, "the tab did not move past the other");
        compare(testCase.stackOf(was[0]).panels.length, 2, "reordering must not lose a panel");
    }

    function test_o_theTabsOpenAGapForTheOneBeingCarried() {
        tryVerify(() => view.middleSlot !== null && view.middleSlot.width > 0);
        const pages = testCase.settled("pages");
        const sections = testCase.settled("sections");
        const onto = view.mapFromItem(pages, pages.width / 2, Math.round(10 * Theme.scale));
        testCase.carry(sections, onto.x, onto.y);
        tryVerify(() => testCase.pathOf("sections") === testCase.pathOf("pages"), 2000);
        const together = testCase.settled("pages");
        const was = testCase.stackOf("pages").panels.slice();
        const second = findChild(together, "panelTab_" + was[1]);
        verify(second !== null);
        const restingAt = testCase.cornerOf(second).x;

        // Carry the first tab onto the far side of the second, so the second must slide aside.
        const past = view.mapFromItem(second, second.width - 2, second.height / 2);
        testCase.carryTab(together, was[0], past.x, past.y);

        tryVerify(() => testCase.cornerOf(second).x > restingAt + 4, 2000, "the tabs did not open a gap");
        mouseRelease(view, past.x, past.y);

        // Once the hand lets go the gap closes again, whatever the new order is.
        const settledSecond = findChild(testCase.settled(was[0]), "panelTab_" + was[1]);
        tryVerify(() => settledSecond !== null && Math.abs(testCase.cornerOf(settledSecond).x - restingAt) <= 4 || testCase.cornerOf(settledSecond).x < restingAt, 2000, "the gap never closed");
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

    MathViewModel {
        id: maths
    }

    TimeKeeperViewModel {
        id: timeKeeper
    }

    RecordingViewModel {
        id: recordings

        notebook: notebooks.current
    }

    ElementsViewModel {
        id: elementLibrary

        directory: notebooks.directory + "/elements"
    }

    AppActions {
        id: actions

        library: elementLibrary
        maths: maths
        sound: recordings
        timeKeeper: timeKeeper
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
