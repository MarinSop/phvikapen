import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function initTestCase() {
        notebooks.createNotebook("Searching");
        tryCompare(notebooks.current, "loaded", true);
    }

    // Typing is enough to search: there is no button left to press, and none needed.
    function test_aSearchHappensAsItIsTyped() {
        dialog.open();
        tryCompare(dialog, "opened", true);
        const field = findChild(dialog, "findField");
        const results = findChild(dialog, "findResults");
        verify(field !== null);
        verify(findChild(dialog, "findButton") === null, "there is still a button to press");

        field.text = "ekotoksikologija";

        tryVerify(() => dialog.searched, 3000, "typing did not set a search going");
        tryCompare(results, "count", 0);
        compare(notebooks.current.errorMessage, "");
        dialog.close();
    }

    // What was found is gathered under the section and the page it was found on, and a section can
    // be shut to put what is under it out of the way.
    function test_bWhatWasFoundStandsUnderItsSectionAndPage() {
        dialog.open();
        tryCompare(dialog, "opened", true);

        dialog.results = [
            {
                "text": "Reykjavik",
                "sectionTitle": "Travel",
                "pageTitle": "Page 1"
            },
            {
                "text": "Reykjavik again",
                "sectionTitle": "Travel",
                "pageTitle": "Page 1"
            },
            {
                "text": "Reykjavik too",
                "sectionTitle": "Travel",
                "pageTitle": "Page 7"
            },
            {
                "text": "Reykjavik elsewhere",
                "sectionTitle": "Notes",
                "pageTitle": "Page 2"
            }
        ];

        const kinds = dialog.rows.map(row => row.kind);
        compare(kinds, ["section", "page", "hit", "hit", "page", "hit", "section", "page", "hit"], "what was found was not gathered under where it was found");
        compare(dialog.rows[2].at, 0, "a find lost the place it has in what came back");
        compare(dialog.rows[8].at, 3);

        dialog.openOrShut("Travel");

        compare(dialog.rows.map(row => row.kind), ["section", "section", "page", "hit"], "shutting a section did not put what is under it away");

        dialog.openOrShut("Travel");

        compare(dialog.rows.length, 9, "opening the section again did not bring it back");
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
