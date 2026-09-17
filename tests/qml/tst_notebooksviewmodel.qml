import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    property int libraryCount: 0

    function newLibrary() {
        testCase.libraryCount += 1;
        return temporaryDirectory + "/library-" + testCase.libraryCount;
    }

    function addNotebook(notebooks, name) {
        notebooks.createNotebook(name);
        tryCompare(notebooks.current, "loaded", true);
    }

    function openLibrary(directory) {
        const notebooks = createTemporaryObject(notebooksComponent, testCase, {
            directory: directory
        });
        tryVerify(() => notebooks.current !== null);
        tryCompare(notebooks.current, "loaded", true);
        return notebooks;
    }

    function test_a_startsWithOneNotebook() {
        const notebooks = openLibrary(newLibrary());

        compare(notebooks.openNotebooks.length, 1);
        compare(notebooks.library.length, 1);
        compare(notebooks.currentIndex, 0);
        compare(notebooks.current.name, notebooks.openNotebooks[0]);
        compare(notebooks.current.pageCount, 1);
    }

    function test_b_createsAndSwitchesBetweenNotebooks() {
        const notebooks = openLibrary(newLibrary());
        const first = notebooks.current.name;

        testCase.addNotebook(notebooks, "Physics");

        compare(notebooks.openNotebooks, [first, "Physics"]);
        compare(notebooks.currentIndex, 1);
        compare(notebooks.current.name, "Physics");
        verify(notebooks.library.indexOf("Physics") >= 0);

        notebooks.currentIndex = 0;
        compare(notebooks.current.name, first);

        notebooks.openNotebook("Physics");
        compare(notebooks.openNotebooks.length, 2);
        compare(notebooks.currentIndex, 1);
    }

    function test_c_refusesANameThatIsTaken() {
        const notebooks = openLibrary(newLibrary());
        const taken = notebooks.current.name;
        const failures = createTemporaryObject(spyComponent, testCase, {
            target: notebooks,
            signalName: "errorMessage"
        });

        notebooks.createNotebook(taken);

        compare(failures.count, 1);
        compare(notebooks.openNotebooks.length, 1);
    }

    function test_d_renamesANotebookWithItsFile() {
        const directory = newLibrary();
        const notebooks = openLibrary(directory);
        testCase.addNotebook(notebooks, "Physics");
        notebooks.current.addPage();
        compare(notebooks.current.pageCount, 2);

        notebooks.renameNotebook(1, "Chemistry");

        compare(notebooks.openNotebooks[1], "Chemistry");
        verify(notebooks.library.indexOf("Physics") < 0);
        verify(notebooks.library.indexOf("Chemistry") >= 0);
        tryCompare(notebooks.current, "loaded", true);
        compare(notebooks.current.pageCount, 2);
    }

    function test_e_keepsOneNotebookOpen() {
        const notebooks = openLibrary(newLibrary());
        testCase.addNotebook(notebooks, "Physics");

        notebooks.closeNotebook(1);
        compare(notebooks.openNotebooks.length, 1);
        compare(notebooks.currentIndex, 0);

        notebooks.closeNotebook(0);
        compare(notebooks.openNotebooks.length, 1);
    }

    function test_f_deletesANotebookFromTheLibrary() {
        const notebooks = openLibrary(newLibrary());
        testCase.addNotebook(notebooks, "Physics");
        compare(notebooks.library.length, 2);

        notebooks.deleteNotebook("Physics");

        compare(notebooks.library.length, 1);
        compare(notebooks.openNotebooks.length, 1);
        verify(notebooks.library.indexOf("Physics") < 0);
    }

    function test_g_opensTheSameNotebooksAgainLater() {
        const directory = newLibrary();
        const before = openLibrary(directory);
        testCase.addNotebook(before, "Physics");
        testCase.addNotebook(before, "History");
        before.currentIndex = 1;
        const expected = before.openNotebooks.join(",");
        before.destroy();
        wait(0);

        const after = openLibrary(directory);

        compare(after.openNotebooks.join(","), expected);
        compare(after.current.name, "Physics");
    }

    name: "NotebooksViewModel"

    Component {
        id: notebooksComponent

        NotebooksViewModel {
        }
    }

    Component {
        id: spyComponent

        SignalSpy {
        }
    }
}
