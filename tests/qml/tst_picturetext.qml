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

    function test_e_theWordsAreNeverWrittenOntoThePictureItself() {
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
        id: spyComponent

        SignalSpy {
        }
    }
}
