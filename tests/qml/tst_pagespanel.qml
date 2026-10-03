import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function nameOfPage(index) {
        const model = notebooks.current.pages;
        return model.data(model.index(index, 0), Qt.UserRole + 1);
    }

    function rowOfPage(index) {
        const list = findChild(panel, "pageList");
        return list.itemAtIndex(index);
    }

    function initTestCase() {
        notebooks.createNotebook("Carrying");
        tryCompare(notebooks.current, "loaded", true);
        notebooks.current.addPage();
        notebooks.current.addPage();
        tryCompare(notebooks.current, "pageCount", 3);
        tryVerify(() => testCase.rowOfPage(2) !== null);
    }

    function test_a_carryingAPageDownPutsItThere() {
        compare(testCase.nameOfPage(0), "Page 1");
        const row = testCase.rowOfPage(0);

        mouseDrag(row, row.width / 2, row.height / 2, 0, row.height * 2.2);

        tryCompare(notebooks.current.pages, "count", 3);
        compare(testCase.nameOfPage(2), "Page 1", "the page was not carried to the end");
        compare(testCase.nameOfPage(0), "Page 2");
    }

    function test_b_carryingAPageUpPutsItThere() {
        const row = testCase.rowOfPage(2);

        mouseDrag(row, row.width / 2, row.height / 2, 0, -row.height * 2.2);

        compare(testCase.nameOfPage(0), "Page 1", "the page was not carried back to the front");
        compare(notebooks.current.errorMessage, "");
    }

    function test_c_aPageThatSlipsUnderTheFingerIsStillChosen() {
        const wanted = notebooks.current.currentPage === 1 ? 2 : 1;
        tryVerify(() => testCase.rowOfPage(wanted) !== null);
        const row = testCase.rowOfPage(wanted);
        const along = row.height / 2;

        mousePress(row, row.width / 2, along);
        mouseMove(row, row.width / 2, along + 9);
        mouseRelease(row, row.width / 2, along + 9);

        tryCompare(notebooks.current, "currentPage", wanted, 2000, "the page was not chosen because the finger moved");
    }

    // What can be done to a page is asked of the page itself, by a right click or by holding the
    // pointer down on it, rather than by a button standing on every line.
    function test_d_aPageLineAsksWhatCanBeDoneToIt() {
        const row = testCase.rowOfPage(1);
        verify(findChild(row, "pageOptionsButton") === null, "a line still carries a button of its own");
        const options = findChild(row, "pageOptionsMenu");
        verify(options !== null, "the line has no menu of its own");

        mouseClick(row, row.width / 2, row.height / 2, Qt.RightButton);

        tryVerify(() => options.opened, 2000, "a right click did not ask what can be done to the page");
        options.close();
        tryVerify(() => !options.opened, 2000);

        mousePress(row, row.width / 2, row.height / 2);
        tryVerify(() => options.opened, 4000, "holding the pointer down did not ask what can be done to the page");
        mouseRelease(row, row.width / 2, row.height / 2);
        compare(notebooks.current.currentPage, 1);
        options.close();
    }

    height: 600
    name: "PagesPanel"
    visible: true
    when: windowShown
    width: 320

    ToolViewModel {
        id: tools
    }

    SettingsViewModel {
        id: settings
    }

    NotebooksViewModel {
        id: notebooks

        directory: temporaryDirectory + "/panel"
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

    WorkspaceViewModel {
        id: workspace
    }

    PagesPanel {
        id: panel

        actions: actions
        anchors.fill: parent
    }
}
