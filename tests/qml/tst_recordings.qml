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
        return temporaryDirectory + "/sound-" + notebookCount + ".phvika";
    }

    function draw(notebook, fromX, fromY) {
        const canvas = notebook.canvas;
        mousePress(canvas, fromX, fromY);
        mouseMove(canvas, fromX + 20, fromY + 10, -1, Qt.LeftButton);
        mouseMove(canvas, fromX + 40, fromY + 20, -1, Qt.LeftButton);
        mouseRelease(canvas, fromX + 40, fromY + 20);
    }

    function soundOfFourBytes() {
        // Not a real recording: what is being tested is that the notebook keeps whatever bytes it
        // is handed and hands them back.
        return "abcd";
    }

    function recordingIdAt(notebook, row) {
        return notebook.recordings.data(notebook.recordings.index(row, 0), Qt.UserRole + 1);
    }

    function recordingNameAt(notebook, row) {
        return notebook.recordings.data(notebook.recordings.index(row, 0), Qt.UserRole + 2);
    }

    function recordingLengthAt(notebook, row) {
        return notebook.recordings.data(notebook.recordings.index(row, 0), Qt.UserRole + 3);
    }

    function recordingMarksAt(notebook, row) {
        return notebook.recordings.data(notebook.recordings.index(row, 0), Qt.UserRole + 5);
    }

    function test_aRecordingIsBegunKeptAndListed() {
        const notebook = openNotebook(newNotebookPath());

        const id = notebook.beginRecording();

        verify(id !== "", "the recording was never begun");
        compare(notebook.recordings.count, 1);
        compare(notebook.shownRecording, id);

        notebook.keepRecording(id, testCase.soundOfFourBytes(), 4200);

        compare(notebook.recordings.count, 1);
        compare(recordingIdAt(notebook, 0), id);
        compare(recordingLengthAt(notebook, 0), 4200);
    }

    function test_aRecordingThatCameToNothingIsTakenOffAgain() {
        const notebook = openNotebook(newNotebookPath());
        const id = notebook.beginRecording();
        compare(notebook.recordings.count, 1);

        notebook.giveUpRecording(id);

        compare(notebook.recordings.count, 0);
        compare(notebook.shownRecording, "");
    }

    function test_whatIsWrittenWhileRecordingIsTiedToTheMoment() {
        const notebook = openNotebook(newNotebookPath());
        const id = notebook.beginRecording();

        notebook.markFrom(id, 1500);
        testCase.draw(notebook, 60, 60);
        notebook.markFrom(id, 7000);
        testCase.draw(notebook, 60, 140);
        notebook.markFrom("", 0);

        compare(recordingMarksAt(notebook, 0), 2, "what was drawn was not tied to the recording");
    }

    function test_nothingIsTiedWhenNothingIsRecording() {
        const notebook = openNotebook(newNotebookPath());
        const id = notebook.beginRecording();

        notebook.markFrom("", 0);
        testCase.draw(notebook, 60, 60);

        compare(recordingMarksAt(notebook, 0), 0);
        compare(notebook.momentOf(id), -1);
    }

    function test_aTypedBoxIsTiedToTheMomentItWasTyped() {
        const notebook = openNotebook(newNotebookPath());
        const id = notebook.beginRecording();
        notebook.markFrom(id, 2500);

        notebook.writeDown("While it was being said", {}, false);

        compare(recordingMarksAt(notebook, 0), 1);
        const boxId = notebook.pickedText;
        if (boxId !== "") {
            compare(notebook.momentOf(boxId), 2500);
            compare(notebook.recordingOf(boxId), id);
        }
    }

    function test_whatWasBeingWrittenAboutAtAMomentIsFound() {
        const notebook = openNotebook(newNotebookPath());
        const id = notebook.beginRecording();
        notebook.markFrom(id, 1000);
        notebook.writeDown("First", {}, false);
        const first = notebook.pickedText;
        notebook.markFrom(id, 9000);
        notebook.writeDown("Second", {}, false);
        const second = notebook.pickedText;
        notebook.markFrom("", 0);

        compare(notebook.thingWrittenAt(id, 0), "", "nothing had been written yet");
        compare(notebook.thingWrittenAt(id, 4000), first);
        compare(notebook.thingWrittenAt(id, 20000), second);
    }

    function test_aRecordingIsRenamedAndTakenAway() {
        const notebook = openNotebook(newNotebookPath());
        const id = notebook.beginRecording();
        notebook.keepRecording(id, testCase.soundOfFourBytes(), 1000);

        notebook.renameRecording(id, "The meeting");
        compare(recordingNameAt(notebook, 0), "The meeting");

        notebook.markFrom(id, 100);
        testCase.draw(notebook, 60, 60);
        notebook.markFrom("", 0);
        compare(recordingMarksAt(notebook, 0), 1);

        notebook.removeRecording(id);

        compare(notebook.recordings.count, 0);
    }

    function test_aRecordingIsThereAgainWhenTheNotebookIsOpenedAgain() {
        const path = newNotebookPath();
        let id = "";
        {
            const first = openNotebook(path);
            id = first.beginRecording();
            first.renameRecording(id, "Kept");
            first.keepRecording(id, testCase.soundOfFourBytes(), 3300);
            first.markFrom(id, 700);
            testCase.draw(first, 60, 60);
            first.markFrom("", 0);
            first.destroy();
            wait(0);
        }

        const again = openNotebook(path);

        tryCompare(again.recordings, "count", 1, 4000, "the recording was not kept");
        compare(recordingNameAt(again, 0), "Kept");
        compare(recordingLengthAt(again, 0), 3300);
        compare(recordingMarksAt(again, 0), 1, "what was tied to it was not kept");
    }

    function test_whatWasSaidIsKeptAndComesBackAgain() {
        const path = newNotebookPath();
        let id = "";
        {
            const first = openNotebook(path);
            id = first.beginRecording();
            first.keepRecording(id, testCase.soundOfFourBytes(), 5000);
            first.keepSayings(id, [
                {
                    from: 0,
                    to: 2000,
                    text: "We need to finish by Friday."
                },
                {
                    from: 2000,
                    to: 5000,
                    text: "The budget has been approved."
                }
            ], "en");
            compare(first.sayings.count, 2);
            compare(first.shownReading, 2, "it should say it has been read");
            first.destroy();
            wait(0);
        }

        const again = openNotebook(path);

        tryCompare(again.recordings, "count", 1, 4000);
        again.showRecording(recordingIdAt(again, 0));
        tryCompare(again.sayings, "count", 2, 4000, "what was said was not kept");
        compare(again.sayings.data(again.sayings.index(1, 0), Qt.UserRole + 3), "The budget has been approved.");
        compare(again.shownReading, 2);
    }

    function test_aReadingThatFailedSaysSoAndCanBeAskedAgain() {
        const notebook = openNotebook(newNotebookPath());
        const id = notebook.beginRecording();
        notebook.keepRecording(id, testCase.soundOfFourBytes(), 1000);

        notebook.markReading(id, 3, "the microphone was too far away");

        compare(notebook.shownReading, 3);
        compare(notebook.shownTrouble, "the microphone was too far away");

        notebook.markReading(id, 1, "");

        compare(notebook.shownReading, 1, "it should say it is being read");
        compare(notebook.shownTrouble, "");
    }

    function test_readingNothingAtAllCountsAsAFailure() {
        const notebook = openNotebook(newNotebookPath());
        const id = notebook.beginRecording();
        notebook.keepRecording(id, testCase.soundOfFourBytes(), 1000);

        notebook.keepSayings(id, [], "en");

        compare(notebook.sayings.count, 0);
        compare(notebook.shownReading, 3, "nothing made out is a failure, not a reading");
    }

    function test_theTimeIsReadOutTheWayAClockShowsIt() {
        compare(sound.saidTime(0), "00:00");
        compare(sound.saidTime(65000), "01:05");
        compare(sound.saidTime(3725000), "1:02:05");
        compare(sound.saidTime(-5), "00:00");
    }

    function test_aMachineWithNothingToListenWithSaysSo() {
        const told = createTemporaryObject(spyComponent, testCase, {
            target: sound,
            signalName: "cannotRecord"
        });
        sound.notebook = null;

        sound.startRecording();

        compare(told.count, 1, "nothing was said about why it could not record");
        verify(!sound.recording);
    }

    function test_playingFromSomethingNeverWrittenDuringARecordingDoesNothing() {
        const notebook = openNotebook(newNotebookPath());
        sound.notebook = notebook;

        sound.playFromThing("nothing-at-all");

        compare(sound.playingId, "");
        sound.notebook = null;
    }

    function test_aMachineThatReadsSpeechNamesTheLanguagesItReads() {
        if (!sound.canRead) {
            compare(sound.languages.length, 0, "a machine that reads nothing names no language");
            return;
        }
        verify(sound.languages.length > 0, "the reader named no language at all");
        verify(sound.languages.indexOf("") < 0, "a language with no name was listed");
    }

    function test_aReadingThatCannotBeAskedForIsMarkedFailedRatherThanLeftWaiting() {
        const notebook = openNotebook(newNotebookPath());
        sound.notebook = notebook;
        const id = notebook.beginRecording();
        notebook.keepRecording(id, soundOfFourBytes(), 1000);
        notebook.shownRecording = id;

        sound.readWhatWasSaid(id, "");

        // Either the reader takes it on, or it refuses with a reason. What it may never do is
        // leave the recording waiting to be read with nobody reading it.
        tryVerify(() => sound.readingNow === "", 8000, "the reading was never let go of");
        tryVerify(() => notebook.shownReading === 2 || notebook.shownReading === 3, 8000);
        if (notebook.shownReading === 3) {
            verify(notebook.shownTrouble !== "", "nothing was said about why it could not be read");
        }
        sound.notebook = null;
    }

    height: 400
    name: "Recordings"
    visible: true
    when: windowShown
    width: 400

    RecordingViewModel {
        id: sound
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

    Component {
        id: spyComponent

        SignalSpy {
        }
    }
}
