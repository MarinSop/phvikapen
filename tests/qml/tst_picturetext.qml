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
        return temporaryDirectory + "/picturetext-" + notebookCount + ".phvika";
    }

    // A small picture with plain words drawn on it, written out as a file the machine can open.
    function pictureOfWords(path, words) {
        saying.said = words;
        wait(60);
        const shot = grabImage(saying);
        shot.save(path);
        return shot.width > 0;
    }

    function test_a_aPictureThatIsNotOnThePageIsRefused() {
        const notebook = openNotebook(newNotebookPath());
        const told = createTemporaryObject(spyComponent, testCase, {
            target: notebook,
            signalName: "pictureUnread"
        });

        notebook.readPicture("nothing-at-all", "");

        compare(told.count, 1);
        verify(told.signalArguments[0][1] !== "", "nothing was said about why");
    }

    function test_b_readingSaysPlainlyWhenTheMachineCannot() {
        const notebook = openNotebook(newNotebookPath());

        // Where the machine carries no reader, every picture is refused with a reason rather than
        // quietly coming back empty.
        if (notebook.canReadPictures()) {
            verify(true);
            return;
        }
        const path = temporaryDirectory + "/words.png";
        verify(pictureOfWords(path, "Hello"), "the picture could not be written");
        notebook.addPicture(Qt.resolvedUrl("file://" + path));
        tryVerify(() => notebook.pickedPicture !== "", 4000);
        const told = createTemporaryObject(spyComponent, testCase, {
            target: notebook,
            signalName: "pictureUnread"
        });

        notebook.readPicture(notebook.pickedPicture, "");

        tryCompare(told, "count", 1, 4000);
    }

    function test_c_theWordsInAPictureAreRead() {
        const notebook = openNotebook(newNotebookPath());
        if (!notebook.canReadPictures()) {
            skip("this machine cannot read the words in a picture");
        }
        const path = temporaryDirectory + "/words.png";
        verify(pictureOfWords(path, "Budget"), "the picture could not be written");
        notebook.addPicture(Qt.resolvedUrl("file://" + path));
        tryVerify(() => notebook.pickedPicture !== "", 4000);
        const pictureId = notebook.pickedPicture;
        const read = createTemporaryObject(spyComponent, testCase, {
            target: notebook,
            signalName: "pictureRead"
        });

        notebook.readPicture(pictureId, "");

        tryCompare(read, "count", 1, 10000, "nothing came back from reading the picture");
        const words = read.signalArguments[0][1];
        verify(words.length > 0, "no words came back");
        compare(notebook.wordsInPicture(pictureId), words, "what was read was not kept");
    }

    function test_d_askingAgainHandsBackWhatWasReadBefore() {
        const notebook = openNotebook(newNotebookPath());
        if (!notebook.canReadPictures()) {
            skip("this machine cannot read the words in a picture");
        }
        const path = temporaryDirectory + "/words.png";
        verify(pictureOfWords(path, "Friday"));
        notebook.addPicture(Qt.resolvedUrl("file://" + path));
        tryVerify(() => notebook.pickedPicture !== "", 4000);
        const pictureId = notebook.pickedPicture;
        const read = createTemporaryObject(spyComponent, testCase, {
            target: notebook,
            signalName: "pictureRead"
        });
        notebook.readPicture(pictureId, "");
        tryCompare(read, "count", 1, 10000);
        const first = read.signalArguments[0][1];

        notebook.readPicture(pictureId, "");

        compare(read.count, 2, "asking again said nothing");
        compare(read.signalArguments[1][1], first);

        notebook.forgetWordsInPicture(pictureId);
        compare(notebook.wordsInPicture(pictureId), "", "what was read was not forgotten");
    }

    function test_e_whatWasReadIsThereAgainWhenTheNotebookIsOpenedAgain() {
        const path = newNotebookPath();
        const picture = temporaryDirectory + "/kept-words.png";
        let middle = {};
        verify(pictureOfWords(picture, "Reykjavik"), "the picture could not be written");
        let words = "";
        {
            const first = openNotebook(path);
            if (!first.canReadPictures()) {
                skip("this machine cannot read the words in a picture");
            }
            first.addPicture(Qt.resolvedUrl("file://" + picture));
            tryVerify(() => first.pickedPicture !== "", 4000);
            const read = createTemporaryObject(spyComponent, testCase, {
                target: first,
                signalName: "pictureRead"
            });
            first.readPicture(first.pickedPicture, "");
            tryCompare(read, "count", 1, 10000, "nothing came back from reading the picture");
            words = read.signalArguments[0][1];
            verify(words.length > 0);
            middle = first.pickedPictureBox;
            first.destroy();
            wait(0);
        }

        const again = openNotebook(path);
        const pictureId = again.pictureUnder(middle.columnX + (middle.boxWidth / 2), middle.columnY + (middle.boxHeight / 2));
        verify(pictureId !== "", "the picture itself was not kept");
        const read = createTemporaryObject(spyComponent, testCase, {
            target: again,
            signalName: "pictureRead"
        });

        again.readPicture(pictureId, "");

        tryCompare(read, "count", 1, 10000, "what was read was not kept");
        compare(read.signalArguments[0][1], words, "what came back is not what was read before");
        compare(again.wordsInPicture(pictureId), words);

        // Searching goes to the notebook itself, so finding the word proves it was written down
        // rather than read out of the picture a second time.
        const found = createTemporaryObject(spyComponent, testCase, {
            target: again,
            signalName: "found"
        });
        again.find("Reykjavik");
        tryVerify(() => found.count > 0, 6000, "the search never came back");
        verify(found.signalArguments[found.count - 1][0].length > 0, "what was read was not written into the notebook");
    }

    function test_f_aNotebookIsSearchedForTheWordsInItsPictures() {
        const notebook = openNotebook(newNotebookPath());
        if (!notebook.canReadPictures()) {
            skip("this machine cannot read the words in a picture");
        }
        const picture = temporaryDirectory + "/searched-words.png";
        verify(pictureOfWords(picture, "Reykjavik"), "the picture could not be written");
        notebook.addPicture(Qt.resolvedUrl("file://" + picture));
        tryVerify(() => notebook.pickedPicture !== "", 4000);
        const read = createTemporaryObject(spyComponent, testCase, {
            target: notebook,
            signalName: "pictureRead"
        });
        notebook.readPicture(notebook.pickedPicture, "");
        tryCompare(read, "count", 1, 10000);
        const found = createTemporaryObject(spyComponent, testCase, {
            target: notebook,
            signalName: "found"
        });

        notebook.find("Reykjavik");

        tryVerify(() => found.count > 0, 6000, "the search never came back");
        const hits = found.signalArguments[found.count - 1][0];
        verify(hits.length > 0, "the search did not reach the words in the picture");
    }

    function test_fa_everyPictureInTheNotebookIsReadInOneGo() {
        const path = newNotebookPath();
        const picture = temporaryDirectory + "/every-words.png";
        verify(pictureOfWords(picture, "Reykjavik"), "the picture could not be written");
        let middle = {};
        {
            const first = openNotebook(path);
            if (!first.canReadPictures()) {
                skip("this machine cannot read the words in a picture");
            }
            first.addPicture(Qt.resolvedUrl("file://" + picture));
            tryVerify(() => first.pickedPicture !== "", 4000);
            middle = first.pickedPictureBox;
            // Nothing is asked about the picture, so nothing has been read out of it.
            first.destroy();
            wait(0);
        }

        const again = openNotebook(path);
        const pictureId = again.pictureUnder(middle.columnX + (middle.boxWidth / 2), middle.columnY + (middle.boxHeight / 2));
        verify(pictureId !== "", "the picture itself was not kept");
        compare(again.wordsInPicture(pictureId), "", "the picture was read without being asked");

        again.readEveryPicture("");

        tryVerify(() => again.wordsInPicture(pictureId) !== "", 20000, "reading every picture did not read this one");
        tryVerify(() => again.picturesToRead === 0, 20000, "the reading never finished");
        const after = createTemporaryObject(spyComponent, testCase, {
            target: again,
            signalName: "found"
        });
        again.find("Reykjavik");
        tryVerify(() => after.count > 0, 6000);
        verify(after.signalArguments[0][0].length > 0, "reading every picture did not reach the search");
    }

    function test_fb_readingEveryPictureIsGivenUpWhenAskedTo() {
        const notebook = openNotebook(newNotebookPath());

        // With nothing to read, giving up is quiet rather than an error.
        notebook.giveUpReadingPictures();

        compare(notebook.picturesToRead, 0);
    }

    function test_fc_whereEachRunOfWordsSitsInThePictureIsHandedBack() {
        const notebook = openNotebook(newNotebookPath());
        if (!notebook.canReadPictures()) {
            skip("this machine cannot read the words in a picture");
        }
        const path = temporaryDirectory + "/where-words.png";
        verify(pictureOfWords(path, "Reykjavik"), "the picture could not be written");
        notebook.addPicture(Qt.resolvedUrl("file://" + path));
        tryVerify(() => notebook.pickedPicture !== "", 4000);
        const pictureId = notebook.pickedPicture;
        compare(notebook.wordsFoundInPicture(pictureId).length, 0, "nothing has been read yet");
        const read = createTemporaryObject(spyComponent, testCase, {
            target: notebook,
            signalName: "pictureRead"
        });

        notebook.readPicture(pictureId, "");

        tryCompare(read, "count", 1, 10000);
        const runs = notebook.wordsFoundInPicture(pictureId);
        verify(runs.length > 0, "no run of words was handed back");
        for (const run of runs) {
            verify(run.text.length > 0, "a run with nothing in it was handed back");
            // The corners are shares of the picture, so they all lie between nothing and one.
            verify(run.left >= 0 && run.left <= 1, "left is not a share of the picture");
            verify(run.top >= 0 && run.top <= 1, "top is not a share of the picture");
            verify(run.right > run.left, "the run has no width");
            verify(run.bottom > run.top, "the run has no height");
        }

        notebook.forgetWordsInPicture(pictureId);
        compare(notebook.wordsFoundInPicture(pictureId).length, 0, "the runs were not forgotten");
    }

    function test_fd_aRunOfWordsIsPutOnThePageAsTypeWhereItStands() {
        const notebook = openNotebook(newNotebookPath());
        if (!notebook.canReadPictures()) {
            skip("this machine cannot read the words in a picture");
        }
        const path = temporaryDirectory + "/typed-words.png";
        verify(pictureOfWords(path, "Reykjavik"), "the picture could not be written");
        notebook.addPicture(Qt.resolvedUrl("file://" + path));
        tryVerify(() => notebook.pickedPicture !== "", 4000);
        const pictureId = notebook.pickedPicture;
        const read = createTemporaryObject(spyComponent, testCase, {
            target: notebook,
            signalName: "pictureRead"
        });
        notebook.readPicture(pictureId, "");
        tryCompare(read, "count", 1, 10000);
        const runs = notebook.wordsFoundInPicture(pictureId);
        verify(runs.length > 0);
        const was = notebook.texts.count;

        notebook.writeDownAt(runs[0].text, 120, 140, {});

        tryCompare(notebook.texts, "count", was + 1, 4000, "the words were not put on the page");
        // The picture itself is untouched: the words are a box of type beside it.
        compare(notebook.pickedPicture !== "" || notebook.pickedText !== "", true);
    }

    function test_fe_severalRunsArePickedOutAndTakenTogether() {
        const notebook = openNotebook(newNotebookPath());
        if (!notebook.canReadPictures()) {
            skip("this machine cannot read the words in a picture");
        }
        const path = temporaryDirectory + "/two-runs.png";
        verify(pictureOfWords(path, "Reykjavik\nGothenburg"), "the picture could not be written");
        notebook.addPicture(Qt.resolvedUrl("file://" + path));
        tryVerify(() => notebook.pickedPicture !== "", 4000);
        const pictureId = notebook.pickedPicture;
        const read = createTemporaryObject(spyComponent, testCase, {
            target: notebook,
            signalName: "pictureRead"
        });
        notebook.readPicture(pictureId, "");
        tryCompare(read, "count", 1, 10000);
        const runs = notebook.wordsFoundInPicture(pictureId);
        if (runs.length < 2) {
            skip("this machine read the picture as one run");
        }
        const canvas = createTemporaryObject(canvasComponent, testCase);
        const tools = createTemporaryObject(toolsComponent, testCase);
        const layer = createTemporaryObject(layerComponent, testCase, {
            canvas: canvas,
            notebook: notebook,
            tools: tools
        });
        tryVerify(() => layer.found.length >= 2, 4000);

        // Picked out in the other order, to prove the words come out in the order they stand.
        layer.chooseRun(1);
        layer.chooseRun(0);

        compare(layer.chosen.length, 2, "two runs were not picked out");
        compare(layer.chosenWords(), runs[0].text + "\n" + runs[1].text);
        const mark = findChild(layer, "pictureWord0");
        verify(mark !== null, "the mark on the first run is not there");
        compare(mark.border.width, 2, "a run picked out is not shown as picked out");

        layer.chooseRun(0);

        compare(layer.chosen.length, 1, "the run was not put back");
        compare(layer.chosenWords(), runs[1].text);
        let all = "";
        for (const run of runs) {
            all += (all === "" ? "" : "\n") + run.text;
        }
        compare(layer.everyWord(), all, "everything read out of the picture is not taken together");
    }

    function test_g_theWordsAreNeverWrittenOntoThePictureItself() {
        const notebook = openNotebook(newNotebookPath());
        const path = temporaryDirectory + "/words.png";
        verify(pictureOfWords(path, "Kept"));
        notebook.addPicture(Qt.resolvedUrl("file://" + path));
        tryVerify(() => notebook.pickedPicture !== "", 4000);
        const pictureId = notebook.pickedPicture;
        const before = notebook.areaOfWhatIsPicked();

        notebook.readPicture(pictureId, "");
        wait(200);

        compare(notebook.pickedPicture, pictureId, "the picture itself was taken off the page");
        const after = notebook.areaOfWhatIsPicked();
        compare(after.width, before.width, "the picture itself was changed");
        compare(after.height, before.height);
    }

    height: 200
    name: "PictureText"
    visible: true
    when: windowShown
    width: 400

    Rectangle {
        id: saying

        property string said: "Hello"

        color: "white"
        height: 120
        width: 360

        Text {
            anchors.centerIn: parent
            color: "black"
            font.pixelSize: 48
            text: saying.said
        }
    }

    Component {
        id: canvasComponent

        InkCanvas {
            height: 200
            width: 400
        }
    }

    Component {
        id: notebookComponent

        NotebookViewModel {
        }
    }

    Component {
        id: toolsComponent

        ToolViewModel {
        }
    }

    Component {
        id: layerComponent

        PictureLayer {
            height: 200
            width: 400
        }
    }

    Component {
        id: spyComponent

        SignalSpy {
        }
    }
}
