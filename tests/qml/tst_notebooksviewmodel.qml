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

    // A notebook is part of the library once the reader has saved it there.
    function keepNotebook(notebooks, directory, name) {
        testCase.addNotebook(notebooks, name);
        notebooks.current.saveAs("file://" + directory + "/" + name + ".phvika");
        tryVerify(() => notebooks.library.indexOf(name) >= 0);
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
        compare(notebooks.library.length, 0, "a notebook nobody saved was put in the library");
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
        testCase.keepNotebook(notebooks, directory, "Physics");
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
    }

    function test_e2_everyNotebookCanBeDeleted() {
        const directory = newLibrary();
        const notebooks = openLibrary(directory);
        testCase.keepNotebook(notebooks, directory, "Notes");
        testCase.keepNotebook(notebooks, directory, "Physics");
        compare(notebooks.library.length, 2);

        for (const name of notebooks.library.slice()) {
            notebooks.deleteNotebook(name);
        }

        compare(notebooks.library.length, 0);
        compare(notebooks.openNotebooks.length, 0);
        compare(notebooks.current, null);
    }

    function test_f_deletesANotebookFromTheLibrary() {
        const directory = newLibrary();
        const notebooks = openLibrary(directory);
        testCase.keepNotebook(notebooks, directory, "Notes");
        testCase.keepNotebook(notebooks, directory, "Physics");
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

    function test_z3_theTabsStandInTheOrderTheyArePutIn() {
        const notebooks = openLibrary(newLibrary());
        testCase.addNotebook(notebooks, "First");
        testCase.addNotebook(notebooks, "Second");
        testCase.addNotebook(notebooks, "Third");
        compare(notebooks.openNotebooks, ["First", "Second", "Third"]);
        notebooks.currentIndex = 2;

        notebooks.moveNotebook(2, 0);

        compare(notebooks.openNotebooks, ["Third", "First", "Second"]);
        compare(notebooks.currentIndex, 0, "the notebook being read changed");
        compare(notebooks.current.name, "Third");
    }

    function test_z4_aNewNotebookIsNoPartOfTheLibraryUntilItIsSaved() {
        const notebooks = openLibrary(newLibrary());
        testCase.addNotebook(notebooks, "Draft");

        compare(notebooks.library.indexOf("Draft"), -1, "an unsaved notebook was put in the library");
        verify(notebooks.current.keptAt === "");

        notebooks.current.saveAs("file://" + temporaryDirectory + "/kept-" + testCase.libraryCount + ".phvika");

        tryVerify(() => notebooks.library.indexOf("kept-" + testCase.libraryCount) >= 0, 3000, "a saved notebook is still missing from the library");
    }

    function test_z5_pagesFromADocumentKeepTheirOwnSizeUnlessPaperIsAskedFor() {
        const kept = openLibrary(newLibrary());

        kept.createNotebookWithSetup("Reader", -1, PageOptions.Blank, false, "file://" + samplePdf);

        tryCompare(kept.current, "pageCount", 2);
        wait(200);
        compare(kept.current.paper, PageOptions.Custom, "the document's own size was thrown away");

        const given = openLibrary(newLibrary());

        given.createNotebookWithSetup("Reader", PageOptions.A5, PageOptions.Blank, false, "file://" + samplePdf);

        tryCompare(given.current, "pageCount", 2);
        tryCompare(given.current, "paper", PageOptions.A5, 3000, "the paper that was asked for never reached the pages");
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
