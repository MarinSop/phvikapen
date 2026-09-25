import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function groupsOn(workspace, side) {
        return workspace.docks[side].groups;
    }

    function init() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.resetWorkspace();
    }

    function test_a_startsWithTheContentsOnTheLeftAndNothingElse() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);

        verify(workspace.isOpen("contents"));
        verify(!workspace.isOpen("layers"));
        verify(!workspace.isOpen("pageSetup"), "the page setup is off until it is asked for");
        compare(testCase.groupsOn(workspace, WorkspaceViewModel.Left).length, 1);
        compare(testCase.groupsOn(workspace, WorkspaceViewModel.Left)[0].panels, ["contents"]);
    }

    function test_b_aPanelIsOpenedWhereItBelongsAndClosedAgain() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);

        workspace.openPanel("layers");
        verify(workspace.isOpen("layers"));
        compare(testCase.groupsOn(workspace, WorkspaceViewModel.Right)[0].panels, ["layers"]);

        workspace.closePanel("layers");
        verify(!workspace.isOpen("layers"));
        compare(testCase.groupsOn(workspace, WorkspaceViewModel.Right).length, 0);
    }

    function test_c_aPanelDroppedOnAnotherBecomesATab() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);

        workspace.openPanel("layers");
        workspace.dockPanel("pageSetup", WorkspaceViewModel.Right, 0);

        const groups = testCase.groupsOn(workspace, WorkspaceViewModel.Right);
        compare(groups.length, 1, "both stand in one group");
        compare(groups[0].panels, ["layers", "pageSetup"]);
        compare(groups[0].current, 1, "the one just dropped is the one shown");
    }

    function test_d_aPanelDraggedOutOfATabGroupStandsOnItsOwn() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);

        workspace.openPanel("layers");
        workspace.dockPanel("pageSetup", WorkspaceViewModel.Right, 0);
        workspace.dockPanel("pageSetup", WorkspaceViewModel.Bottom, -1);

        compare(testCase.groupsOn(workspace, WorkspaceViewModel.Right)[0].panels, ["layers"]);
        compare(testCase.groupsOn(workspace, WorkspaceViewModel.Bottom)[0].panels, ["pageSetup"]);
    }

    function test_e_anEmptyGroupIsTakenAway() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);

        workspace.openPanel("layers");
        workspace.dockPanel("contents", WorkspaceViewModel.Right, 0);

        compare(testCase.groupsOn(workspace, WorkspaceViewModel.Left).length, 0, "nothing is left standing on the left");
        compare(testCase.groupsOn(workspace, WorkspaceViewModel.Right)[0].panels, ["layers", "contents"]);
    }

    function test_f_aPanelComesBackWhereItWasClosed() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);

        workspace.openPanel("layers");
        workspace.dockPanel("layers", WorkspaceViewModel.Bottom, -1);
        workspace.closePanel("layers");
        workspace.openPanel("layers");

        compare(testCase.groupsOn(workspace, WorkspaceViewModel.Bottom)[0].panels, ["layers"]);
    }

    function test_g_theLayoutIsRememberedForTheNextTime() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.openPanel("layers");
        workspace.dockPanel("layers", WorkspaceViewModel.Bottom, -1);
        workspace.setSideExtent(WorkspaceViewModel.Bottom, 300);

        const later = createTemporaryObject(workspaceComponent, testCase);

        verify(later.isOpen("layers"));
        compare(testCase.groupsOn(later, WorkspaceViewModel.Bottom)[0].panels, ["layers"]);
        compare(later.docks[WorkspaceViewModel.Bottom].extent, 300);
    }

    function test_h_theLayoutCanBePutBackAsItWas() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        workspace.openPanel("layers");
        workspace.closePanel("contents");

        workspace.resetWorkspace();

        verify(workspace.isOpen("contents"));
        verify(!workspace.isOpen("layers"));
    }

    function test_i_aPanelDroppedOnItsOwnLoneGroupStaysWhereItIs() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);

        workspace.dockPanel("contents", WorkspaceViewModel.Left, 0);

        compare(testCase.groupsOn(workspace, WorkspaceViewModel.Left).length, 1);
        compare(testCase.groupsOn(workspace, WorkspaceViewModel.Left)[0].panels, ["contents"]);
    }

    function test_j_showingAPanelThatIsShutOpensIt() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);
        const shown = signalSpy.createObject(testCase, {
            "target": workspace,
            "signalName": "panelShown"
        });

        workspace.showPanel("pageSetup");

        verify(workspace.isOpen("pageSetup"));
        compare(shown.count, 1);
    }

    function test_k_aWidthTheWindowCouldNotHoldIsBroughtBackIntoRange() {
        const workspace = createTemporaryObject(workspaceComponent, testCase);

        workspace.setSideExtent(WorkspaceViewModel.Left, 5000);
        verify(workspace.docks[WorkspaceViewModel.Left].extent <= 640);

        workspace.setSideExtent(WorkspaceViewModel.Left, 1);
        verify(workspace.docks[WorkspaceViewModel.Left].extent >= 140);
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
