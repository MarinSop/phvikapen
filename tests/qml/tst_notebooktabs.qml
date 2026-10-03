import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function tabOf(index) {
        const bar = findChild(tabs, "notebookTab" + index);
        return bar;
    }

    function initTestCase() {
        notebooks.createNotebook("First");
        tryCompare(notebooks.current, "loaded", true);
        tryCompare(notebooks, "openNotebooks", ["First"]);
    }

    // A plus makes a notebook and opens it, rather than asking first in a window of its own.
    function test_a_thePlusMakesANotebookAtOnce() {
        const plus = findChild(tabs, "notebooksButton");
        verify(plus !== null, "there is no plus beside the tabs");
        const was = notebooks.openNotebooks.length;

        mouseClick(plus, plus.width / 2, plus.height / 2);

        tryVerify(() => notebooks.openNotebooks.length === was + 1, 4000, "the plus made no notebook");
        tryCompare(notebooks.current, "loaded", true);
    }

    // What can be done to a notebook is asked of its tab, by a right click or by holding the
    // pointer down on it.
    function test_b_aTabAsksWhatCanBeDoneToIt() {
        const tab = testCase.tabOf(0);
        verify(tab !== null, "the first tab is not there");
        const menu = findChild(tab, "notebookTabMenu");
        verify(menu !== null, "the tab has no menu of its own");

        mouseClick(tab, tab.width / 2, tab.height / 2, Qt.RightButton);

        tryVerify(() => menu.opened, 2000, "a right click did not ask what can be done to the notebook");
        menu.close();
        tryVerify(() => !menu.opened, 2000);

        mousePress(tab, tab.width / 2, tab.height / 2);
        tryVerify(() => menu.opened, 4000, "holding the pointer down did not ask what can be done to the notebook");
        mouseRelease(tab, tab.width / 2, tab.height / 2);
        menu.close();
    }

    height: 120
    name: "NotebookTabs"
    visible: true
    when: windowShown
    width: 640

    ToolViewModel {
        id: tools
    }

    SettingsViewModel {
        id: settings
    }

    NotebooksViewModel {
        id: notebooks

        directory: temporaryDirectory + "/tabs"
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

    WorkspaceViewModel {
        id: workspace
    }

    AppActions {
        id: actions

        library: elementLibrary
        maths: maths
        notebooks: notebooks
        settings: settings
        sound: recordings
        timeKeeper: timeKeeper
        tools: tools
        workspace: workspace
    }

    NotebookTabs {
        id: tabs

        actions: actions
        anchors.fill: parent
        notebooks: notebooks
    }
}
