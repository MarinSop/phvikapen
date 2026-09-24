import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function initTestCase() {
        notebooks.createNotebook("Searching");
        tryCompare(notebooks.current, "loaded", true);
    }

    function test_aSearchThatFindsNothingSaysSo() {
        dialog.open();
        tryCompare(dialog, "opened", true);
        const field = findChild(dialog, "findField");
        const button = findChild(dialog, "findButton");
        const results = findChild(dialog, "findResults");
        verify(field !== null);
        verify(button !== null);

        field.text = "ekotoksikologija";
        mouseClick(button);

        tryCompare(results, "count", 0);
        compare(notebooks.current.errorMessage, "");
        dialog.close();
    }

    function test_theDialogSaysWhenTheMachineCannotRead() {
        compare(notebooks.current.readsHandwriting, Qt.platform.os === "windows");
    }

    height: 480
    name: "FindDialog"
    visible: true
    when: windowShown
    width: 560

    NotebooksViewModel {
        id: notebooks

        directory: temporaryDirectory + "/find"
    }

    FindDialog {
        id: dialog

        notebook: notebooks.current
    }
}
