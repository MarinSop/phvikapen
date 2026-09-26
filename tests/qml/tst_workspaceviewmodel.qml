import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    readonly property int kindMiddle: 0
    readonly property int kindSplit: 2
    readonly property int kindStack: 1

    function countOf(node, kind) {
        let found = node.kind === kind ? 1 : 0;
        for (const child of node.children) {
            found += testCase.countOf(child, kind);
        }
        return found;
    }

    function init() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.resetWorkspace();
    }

    function nodeAt(workspace, path) {
        let node = workspace.layout;
        if (path === "") {
            return node;
        }
        for (const step of path.split(".")) {
            node = node.children[Number(step)];
        }
        return node;
    }

    function pathOf(workspace, panelId) {
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

    function test_a_startsWithTheSectionsOverThePagesBesideTheSheet() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);

        verify(workspace.isOpen("sections"));
        verify(workspace.isOpen("pages"));
        verify(!workspace.isOpen("layers"));
        verify(!workspace.isOpen("pageSetup"), "the page setup is off until it is asked for");
        compare(testCase.countOf(workspace.layout, testCase.kindMiddle), 1, "the sheet has exactly one place");
        compare(testCase.nodeAt(workspace, testCase.pathOf(workspace, "sections")).panels, ["sections"]);
        compare(testCase.nodeAt(workspace, testCase.pathOf(workspace, "pages")).panels, ["pages"]);
    }

    function test_b_aPanelOpenedStandsBesideTheSheetAndClosesAgain() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        const stacks = testCase.countOf(workspace.layout, testCase.kindStack);

        workspace.openPanel("layers");
        verify(workspace.isOpen("layers"));
        compare(testCase.countOf(workspace.layout, testCase.kindStack), stacks + 1);

        workspace.closePanel("layers");
        verify(!workspace.isOpen("layers"));
        compare(testCase.countOf(workspace.layout, testCase.kindStack), stacks);
    }

    function test_c_aPanelDroppedOnAnEdgeSplitsWhatIsThere() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        const pages = testCase.pathOf(workspace, "pages");

        workspace.dropBeside("layers", pages, WorkspaceViewModel.Right);

        const layers = testCase.pathOf(workspace, "layers");
        verify(layers !== null);
        const split = testCase.nodeAt(workspace, layers.substring(0, layers.lastIndexOf(".")));
        compare(split.kind, testCase.kindSplit);
        verify(split.across, "a drop on the right edge puts the two side by side");
        compare(split.children[1].panels, ["layers"], "and the one dropped stands to the right");
    }

    function test_d_aPanelDroppedInTheMiddleOfAnotherStandsBelowIt() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        const pages = testCase.pathOf(workspace, "pages");

        workspace.dropBeside("layers", pages, WorkspaceViewModel.Bottom);

        const layers = testCase.pathOf(workspace, "layers");
        const split = testCase.nodeAt(workspace, layers.substring(0, layers.lastIndexOf(".")));
        verify(!split.across, "one below the other");
        const order = split.children.map(child => child.panels[0]);
        compare(order.indexOf("layers"), order.indexOf("pages") + 1, "the one dropped stands under it");
    }

    function test_e_aPanelDroppedOnTheTabsBecomesATab() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        const pages = testCase.pathOf(workspace, "pages");

        workspace.dropAsTab("layers", pages, 1);

        const stack = testCase.nodeAt(workspace, testCase.pathOf(workspace, "layers"));
        compare(stack.panels, ["pages", "layers"]);
        compare(stack.current, 1, "the one just dropped is the one shown");
    }

    function test_f_aTabCarriedOutOfAGroupStandsOnItsOwnAgain() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.dropAsTab("layers", testCase.pathOf(workspace, "pages"), 1);

        workspace.dropBeside("layers", testCase.pathOf(workspace, "sections"), WorkspaceViewModel.Left);

        compare(testCase.nodeAt(workspace, testCase.pathOf(workspace, "pages")).panels, ["pages"]);
        compare(testCase.nodeAt(workspace, testCase.pathOf(workspace, "layers")).panels, ["layers"]);
    }

    function test_g_aSplitWithOneSideLeftIsNoLongerASplit() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        const splits = testCase.countOf(workspace.layout, testCase.kindSplit);

        workspace.closePanel("sections");

        compare(testCase.countOf(workspace.layout, testCase.kindSplit), splits - 1);
        verify(workspace.isOpen("pages"));
    }

    function test_h_theLayoutIsRememberedForTheNextTime() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.openPanel("layers");
        const where = testCase.pathOf(workspace, "layers");
        workspace.setExtent(where, 300);

        const later = createTemporaryObject(workspaceComponent, testCase);

        verify(later.isOpen("layers"));
        compare(testCase.nodeAt(later, testCase.pathOf(later, "layers")).extent, 300);
        compare(testCase.countOf(later.layout, testCase.kindMiddle), 1);
    }

    function test_i_theLayoutCanBePutBackAsItWas() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.openPanel("layers");
        workspace.closePanel("pages");

        workspace.resetWorkspace();

        verify(workspace.isOpen("sections"));
        verify(workspace.isOpen("pages"));
        verify(!workspace.isOpen("layers"));
    }

    function test_j_aLonePanelDroppedOnItselfStaysWhereItIs() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        const before = testCase.countOf(workspace.layout, testCase.kindSplit);
        const pages = testCase.pathOf(workspace, "pages");

        workspace.dropBeside("pages", pages, WorkspaceViewModel.Right);

        compare(testCase.countOf(workspace.layout, testCase.kindSplit), before);
        compare(testCase.nodeAt(workspace, testCase.pathOf(workspace, "pages")).panels, ["pages"]);
    }

    function test_k_showingAPanelThatIsShutOpensIt() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        const shown = signalSpy.createObject(testCase, {
            "target": workspace,
            "signalName": "panelShown"
        });

        workspace.showPanel("pageSetup");

        verify(workspace.isOpen("pageSetup"));
        verify(shown.count >= 1);
    }

    function test_l_aSizeTheWindowCouldNotHoldIsBroughtBackIntoRange() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        const pages = testCase.pathOf(workspace, "pages");

        workspace.setExtent(pages, 1);

        verify(testCase.nodeAt(workspace, pages).extent >= workspace.leastExtent);
    }

    function test_m_aPanelComesBackAsTheTabItWas() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.dropAsTab("layers", testCase.pathOf(workspace, "pages"), 1);

        workspace.closePanel("layers");
        workspace.openPanel("layers");

        compare(testCase.nodeAt(workspace, testCase.pathOf(workspace, "layers")).panels, ["pages", "layers"]);
    }

    function test_n_aPanelSetLooseBecomesAWindowOfItsOwn() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);

        workspace.floatPanel("pages", 40, 60, 300, 340);

        verify(workspace.isOpen("pages"));
        verify(workspace.isAfloat("pages"));
        compare(workspace.floating.length, 1);
        compare(workspace.floating[0].node.panels, ["pages"]);
        compare(workspace.floating[0].x, 40);
        compare(testCase.pathOf(workspace, "pages"), null, "it is no longer in the docked tree");
        compare(testCase.countOf(workspace.layout, testCase.kindMiddle), 1);
    }

    function test_o_aWindowIsMovedAndSizedWithoutLeavingItsPlace() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.floatPanel("pages", 40, 60, 300, 340);

        workspace.movePanelWindow("~0", 120, 90);
        workspace.sizePanelWindow("~0", 10, 10);

        compare(workspace.floating.length, 1);
        compare(workspace.floating[0].x, 120);
        compare(workspace.floating[0].y, 90);
        compare(workspace.floating[0].width, workspace.leastExtent, "a window is never smaller than a panel may be");
    }

    function test_p_aWindowDockedAgainJoinsTheTree() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.floatPanel("pages", 40, 60, 300, 340);

        workspace.dockPanel("pages");

        verify(!workspace.isAfloat("pages"));
        compare(workspace.floating.length, 0);
        verify(testCase.pathOf(workspace, "pages") !== null);
    }

    function test_q_aPanelDroppedOnAWindowJoinsItsTabs() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.floatPanel("pages", 40, 60, 300, 340);

        workspace.dropAsTab("sections", "~0", 1);

        compare(workspace.floating.length, 1);
        compare(workspace.floating[0].node.panels, ["pages", "sections"]);
        verify(workspace.isAfloat("sections"));
    }

    function test_r_aPanelDroppedOnTheEdgeOfAWindowSplitsIt() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.floatPanel("pages", 40, 60, 300, 340);

        workspace.dropBeside("sections", "~0", WorkspaceViewModel.Bottom);

        compare(workspace.floating.length, 1);
        compare(workspace.floating[0].node.kind, testCase.kindSplit);
        compare(workspace.floating[0].node.children.length, 2);
        compare(workspace.floating[0].node.children[1].panels, ["sections"]);
    }

    function test_s_aWindowIsThereAgainTheNextTime() {
        const written = createTemporaryObject(workspaceComponent, testCase);
        written.floatPanel("layers", 70, 80, 320, 300);

        const read = createTemporaryObject(workspaceComponent, testCase);

        compare(read.floating.length, 1);
        compare(read.floating[0].node.panels, ["layers"]);
        compare(read.floating[0].x, 70);
        compare(read.floating[0].height, 300);
    }

    function test_t_aWindowPutAwayComesBackAsAWindow() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.floatPanel("layers", 70, 80, 320, 300);

        workspace.closePanel("layers");
        verify(!workspace.isOpen("layers"));
        workspace.openPanel("layers");

        verify(workspace.isAfloat("layers"));
        compare(workspace.floating[0].x, 70);
    }

    function test_u_puttingTheLayoutBackClosesEveryWindow() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.floatPanel("pages", 40, 60, 300, 340);

        workspace.resetWorkspace();

        compare(workspace.floating.length, 0);
        verify(testCase.pathOf(workspace, "pages") !== null);
    }

    name: "WorkspaceViewModel"

    Component {
        id: workspaceComponent

        WorkspaceViewModel {
        }
    }

    Component {
        id: signalSpy

        SignalSpy {
        }
    }
}
