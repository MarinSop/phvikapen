import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    property int notebookCount: 0

    function openNotebook(path) {
        const canvas = createTemporaryObject(canvasComponent, testCase);
        const notebook = createTemporaryObject(notebookComponent, testCase, {
            canvas: canvas,
            notebookPath: path
        });
        tryCompare(notebook, "loaded", true);
        return notebook;
    }

    function newNotebookPath() {
        notebookCount += 1;
        return temporaryDirectory + "/elements-" + notebookCount + ".phvika";
    }

    function draw(notebook, fromX, fromY) {
        const canvas = notebook.canvas;
        mousePress(canvas, fromX, fromY);
        mouseMove(canvas, fromX + 20, fromY + 10, -1, Qt.LeftButton);
        mouseMove(canvas, fromX + 40, fromY + 20, -1, Qt.LeftButton);
        mouseRelease(canvas, fromX + 40, fromY + 20);
    }

    function pickEverything(notebook) {
        notebook.canvas.selectEverything();
        tryVerify(() => notebook.canvas.selectedCount > 0, 2000, "nothing was picked up");
    }

    // A small picture written out as a file, so that an element can be kept with one in it.
    function pictureFile(path) {
        wait(60);
        const shot = grabImage(blob);
        shot.save(path);
        return shot.width > 0;
    }

    function idOfNamed(name) {
        for (let step = 0; step < library.elements.count; ++step) {
            if (nameAt(step) === name) {
                return idAt(step);
            }
        }
        return "";
    }

    function anyPictureOn(notebook) {
        for (let down = 5; down < testCase.height; down += 15) {
            for (let across = 5; across < testCase.width; across += 15) {
                if (notebook.pictureUnder(across, down) !== "") {
                    return true;
                }
            }
        }
        return false;
    }

    function nameAt(row) {
        return library.elements.data(library.elements.index(row, 0), Qt.UserRole + 2);
    }

    function idAt(row) {
        return library.elements.data(library.elements.index(row, 0), Qt.UserRole + 1);
    }

    function kindAt(row) {
        return library.elements.data(library.elements.index(row, 0), Qt.UserRole + 3);
    }

    function init() {
        library.looking = "";
        library.kind = "";
    }

    function test_a_theLibraryOpensWhereItIsToldTo() {
        tryVerify(() => library.ready, 4000, "the library never opened");
        compare(library.trouble, "");
    }

    function test_b_nothingIsKeptWhenNothingIsPickedUp() {
        const notebook = openNotebook(newNotebookPath());

        verify(!library.anythingToKeep(notebook));

        library.keep(notebook, "Nothing", "Shapes");

        verify(library.trouble !== "", "nothing was said about why");
        library.forgetTrouble();
    }

    function test_c_whatIsPickedUpIsKeptAndListed() {
        const notebook = openNotebook(newNotebookPath());
        testCase.draw(notebook, 60, 60);
        tryCompare(notebook, "strokeCount", 1);
        pickEverything(notebook);
        verify(library.anythingToKeep(notebook));
        const was = library.elements.count;

        library.keep(notebook, "A squiggle", "Shapes");

        tryCompare(library.elements, "count", was + 1, 4000, "the element was not kept");
        let found = -1;
        for (let step = 0; step < library.elements.count; ++step) {
            if (nameAt(step) === "A squiggle") {
                found = step;
            }
        }
        verify(found >= 0, "the element is not in the list");
        compare(kindAt(found), "Shapes");
        verify(library.kinds.indexOf("Shapes") >= 0, "the kind was not listed");
    }

    function test_d_anElementIsPutOnAPageAgain() {
        const notebook = openNotebook(newNotebookPath());
        testCase.draw(notebook, 60, 60);
        tryCompare(notebook, "strokeCount", 1);
        pickEverything(notebook);
        library.keep(notebook, "Again", "Shapes");
        tryVerify(() => {
            for (let step = 0; step < library.elements.count; ++step) {
                if (nameAt(step) === "Again") {
                    return true;
                }
            }
            return false;
        }, 4000);

        const another = openNotebook(newNotebookPath());
        compare(another.strokeCount, 0);
        let which = "";
        for (let step = 0; step < library.elements.count; ++step) {
            if (nameAt(step) === "Again") {
                which = idAt(step);
            }
        }

        library.put(another, which);

        tryCompare(another, "strokeCount", 1, 4000, "the element was not put on the page");
    }

    function test_e_theSameElementIsPutDownManyTimes() {
        const notebook = openNotebook(newNotebookPath());
        testCase.draw(notebook, 60, 60);
        tryCompare(notebook, "strokeCount", 1);
        pickEverything(notebook);
        library.keep(notebook, "Twice", "Shapes");
        tryVerify(() => {
            for (let step = 0; step < library.elements.count; ++step) {
                if (nameAt(step) === "Twice") {
                    return true;
                }
            }
            return false;
        }, 4000);
        let which = "";
        for (let step = 0; step < library.elements.count; ++step) {
            if (nameAt(step) === "Twice") {
                which = idAt(step);
            }
        }
        const another = openNotebook(newNotebookPath());

        library.put(another, which);
        tryCompare(another, "strokeCount", 1, 4000);
        library.put(another, which);

        tryCompare(another, "strokeCount", 2, 4000, "the same element could not be put down twice");
    }

    function test_f_elementsAreSearchedAndFilteredByKind() {
        const notebook = openNotebook(newNotebookPath());
        testCase.draw(notebook, 60, 60);
        tryCompare(notebook, "strokeCount", 1);
        pickEverything(notebook);
        library.keep(notebook, "Arrow head", "Shapes");
        tryVerify(() => library.elements.count > 0, 4000);
        const all = library.elements.count;

        library.looking = "Arrow";
        verify(library.elements.count >= 1);
        verify(library.elements.count <= all);

        library.looking = "nothing that is there";
        compare(library.elements.count, 0);

        library.looking = "";
        library.kind = "A kind nobody made";
        compare(library.elements.count, 0);

        library.kind = "";
        compare(library.elements.count, all);
    }

    function test_g_anElementIsRenamedAndTakenAway() {
        const notebook = openNotebook(newNotebookPath());
        testCase.draw(notebook, 60, 60);
        tryCompare(notebook, "strokeCount", 1);
        pickEverything(notebook);
        library.keep(notebook, "Short lived", "Shapes");
        tryVerify(() => {
            for (let step = 0; step < library.elements.count; ++step) {
                if (nameAt(step) === "Short lived") {
                    return true;
                }
            }
            return false;
        }, 4000);
        let which = "";
        for (let step = 0; step < library.elements.count; ++step) {
            if (nameAt(step) === "Short lived") {
                which = idAt(step);
            }
        }
        const was = library.elements.count;

        library.rename(which, "Renamed");

        let renamed = false;
        for (let step = 0; step < library.elements.count; ++step) {
            if (idAt(step) === which && nameAt(step) === "Renamed") {
                renamed = true;
            }
        }
        verify(renamed, "the element was not renamed");

        library.remove(which);

        compare(library.elements.count, was - 1);
    }

    function test_h_anElementThatIsNotThereIsRefused() {
        const notebook = openNotebook(newNotebookPath());

        library.put(notebook, "nothing-at-all");

        verify(library.trouble !== "");
        library.forgetTrouble();
        compare(library.trouble, "");
    }

    function test_j_anElementIsPutDownWhereItIsAskedFor() {
        const notebook = openNotebook(newNotebookPath());
        testCase.draw(notebook, 60, 60);
        tryCompare(notebook, "strokeCount", 1);
        pickEverything(notebook);
        library.keep(notebook, "Where I say", "Shapes");
        tryVerify(() => idOfNamed("Where I say") !== "", 4000);
        const which = idOfNamed("Where I say");
        const another = openNotebook(newNotebookPath());

        library.putAt(another, which, 300, 260);

        tryCompare(another, "strokeCount", 1, 4000, "the element was not put on the page");
        another.canvas.selectEverything();
        tryVerify(() => another.canvas.selectedCount > 0, 2000);
        const where = another.areaOfWhatIsPicked();
        verify(where.width > 0, "nothing was measured");
        // The element lands with its corner where it was asked for, not in the middle of the view.
        verify(Math.abs(where.columnX - 300) <= 1, "it did not land where it was asked for");
        verify(Math.abs(where.columnY - 260) <= 1, "it did not land where it was asked for");
    }

    function test_i_aPictureIsKeptInAnElementAndPutDownAgain() {
        const notebook = openNotebook(newNotebookPath());
        const path = temporaryDirectory + "/element-picture.png";
        verify(pictureFile(path), "the picture could not be written");
        notebook.addPicture(Qt.resolvedUrl("file://" + path));
        tryVerify(() => notebook.pickedPicture !== "", 4000, "the picture never went on the page");

        verify(library.anythingToKeep(notebook), "a picture on its own is not worth keeping");

        library.keep(notebook, "A picture", "Stickers");
        tryVerify(() => idOfNamed("A picture") !== "", 4000, "the element was not kept");
        const which = idOfNamed("A picture");

        const another = openNotebook(newNotebookPath());
        verify(!anyPictureOn(another), "the second notebook already had a picture on it");

        library.put(another, which);

        tryVerify(() => anyPictureOn(another), 6000, "the picture was not put on the page");
        compare(library.trouble, "");
    }

    height: 400
    name: "Elements"
    visible: true
    when: windowShown
    width: 400

    ElementsViewModel {
        id: library

        directory: temporaryDirectory + "/elementLibrary"
    }

    Rectangle {
        id: blob

        color: "steelblue"
        height: 60
        width: 80

        Rectangle {
            anchors.centerIn: parent
            color: "orange"
            height: 20
            width: 30
        }
    }

    Component {
        id: canvasComponent

        InkCanvas {
            height: testCase.height
            width: testCase.width
        }
    }

    Component {
        id: notebookComponent

        NotebookViewModel {
        }
    }
}
