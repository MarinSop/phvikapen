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
        return createTemporaryObject(notebooksComponent, testCase, {
            directory: directory
        });
    }

    function libraryWithOne(directory) {
        const notebooks = openLibrary(directory);
        testCase.addNotebook(notebooks, "Notes");
        return notebooks;
    }

    function test_a_startsWithNoNotebookAtAll() {
        const notebooks = openLibrary(newLibrary());

        compare(notebooks.current, null);
        compare(notebooks.openNotebooks.length, 0);
        compare(notebooks.library.length, 0);

        testCase.addNotebook(notebooks, "Notes");

        compare(notebooks.openNotebooks.length, 1);
        compare(notebooks.library.length, 1);
        compare(notebooks.current.name, "Notes");
        compare(notebooks.current.pageCount, 1);
    }

    function test_b_createsAndSwitchesBetweenNotebooks() {
        const notebooks = libraryWithOne(newLibrary());
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
        const notebooks = libraryWithOne(newLibrary());
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
        const notebooks = libraryWithOne(directory);
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

    function test_e_theLastNotebookCanBeClosedToo() {
        const notebooks = libraryWithOne(newLibrary());
        testCase.addNotebook(notebooks, "Physics");

        notebooks.closeNotebook(1);
        compare(notebooks.openNotebooks.length, 1);
        compare(notebooks.currentIndex, 0);

        notebooks.closeNotebook(0);
        compare(notebooks.openNotebooks.length, 0);
        compare(notebooks.current, null);
        compare(notebooks.library.length, 2);
    }

    function test_e2_everyNotebookCanBeDeleted() {
        const notebooks = libraryWithOne(newLibrary());
        testCase.addNotebook(notebooks, "Physics");
        compare(notebooks.library.length, 2);

        for (const name of notebooks.library.slice()) {
            notebooks.deleteNotebook(name);
        }

        compare(notebooks.library.length, 0);
        compare(notebooks.openNotebooks.length, 0);
        compare(notebooks.current, null);
    }

    function test_f_deletesANotebookFromTheLibrary() {
        const notebooks = libraryWithOne(newLibrary());
        testCase.addNotebook(notebooks, "Physics");
        compare(notebooks.library.length, 2);

        notebooks.deleteNotebook("Physics");

        compare(notebooks.library.length, 1);
        compare(notebooks.openNotebooks.length, 1);
        verify(notebooks.library.indexOf("Physics") < 0);
    }

    function test_g_opensTheSameNotebooksAgainLater() {
        const directory = newLibrary();
        const before = libraryWithOne(directory);
        testCase.addNotebook(before, "Physics");
        testCase.addNotebook(before, "History");
        before.currentIndex = 1;
        const expected = before.openNotebooks.join(",");
        before.destroy();
        wait(0);

        const after = openLibrary(directory);

        tryVerify(() => after.current !== null);
        compare(after.openNotebooks.join(","), expected);
        compare(after.current.name, "Physics");
    }

    function test_y_aNewNotebookCanBeGivenItsPaperWhenItIsMade() {
        const notebooks = openLibrary(newLibrary());

        notebooks.createNotebookWithSetup("Maths", PageOptions.A5, PageOptions.Grid, true);

        tryCompare(notebooks.current, "loaded", true);
        compare(notebooks.current.name, "Maths");
        tryCompare(notebooks.current, "paper", PageOptions.A5);
        compare(notebooks.current.background, PageOptions.Grid);
        compare(notebooks.current.orientation, PageOptions.Landscape);
    }

    function test_z_aNewNotebookStartsOnThePaperThatWasChosen() {
        const settings = createTemporaryObject(settingsComponent, testCase);
        settings.paper = PageOptions.A5;
        settings.background = PageOptions.Grid;
        const notebooks = openLibrary(newLibrary());

        testCase.addNotebook(notebooks, "Chemistry");

        tryCompare(notebooks.current, "paper", PageOptions.A5);
        compare(notebooks.current.background, PageOptions.Grid);
        settings.paper = PageOptions.A4;
        settings.background = PageOptions.Lined;
    }

    function test_z2_aNotebookStartedFromADocumentHoldsOnlyItsPages() {
        const notebooks = openLibrary(newLibrary());

        notebooks.createNotebookWithSetup("Reader", PageOptions.A4, PageOptions.Blank, false, "file://" + samplePdf);

        tryCompare(notebooks.current, "loaded", true);
        tryCompare(notebooks.current, "pageCount", 2);
        wait(200);
        compare(notebooks.current.pageCount, 2, "the empty page the notebook opened with is still there");
        compare(notebooks.current.errorMessage, "");
    }

    name: "NotebooksViewModel"

    Component {
        id: settingsComponent

        SettingsViewModel {
        }
    }

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
