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

    AppActions {
        id: actions

        notebooks: notebooks
        settings: settings
        tools: tools
    }

    PagesPanel {
        id: panel

        actions: actions
        anchors.fill: parent
    }
}
