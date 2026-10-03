import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function freshBar() {
        return createTemporaryObject(barComponent, testCase);
    }

    function test_a_aNoteIsShownAndGoesAgainOnItsOwn() {
        const bar = freshBar();
        compare(bar.noteCount, 0, "a bar with nothing to say should carry no note");

        bar.say("Something happened");

        compare(bar.noteCount, 1);
        tryVerify(() => bar.noteCount === 0, bar.staysFor + 2000, "the note never went away again");
    }

    function test_b_severalNotesStandOneAboveTheOther() {
        const bar = freshBar();

        bar.wellDone("First");
        bar.wentWrong("Second");
        bar.say("Third");

        compare(bar.noteCount, 3, "the notes did not stand one above the other");
    }

    function test_c_onlySoManyNotesAreKept() {
        const bar = freshBar();

        for (let step = 0; step < bar.mostAtOnce + 3; ++step) {
            bar.say("Note " + step);
        }

        compare(bar.noteCount, bar.mostAtOnce, "the bar grew without end");
    }

    // Something being worked on stays until it is settled, so no spinner is left turning over work
    // that is already finished.
    function test_d_whatIsBeingWorkedOnStaysUntilItIsSettled() {
        const bar = freshBar();

        bar.busy("Reading");

        compare(bar.noteCount, 1);
        wait(300);
        compare(bar.noteCount, 1, "what is being worked on went away on its own");

        bar.settled();

        compare(bar.noteCount, 0);
    }

    function test_e_onlyOneThingIsEverBeingWorkedOn() {
        const bar = freshBar();

        bar.busy("Reading");
        bar.busy("Reading again");

        compare(bar.noteCount, 1, "two spinners were left turning at once");
    }

    function test_f_whatANoteIsAboutIsSaidWhereItIsShortEnough() {
        const bar = freshBar();

        compare(bar.about("Copied", "Hello world"), "Copied: Hello world");
        compare(bar.about("Copied", "  Hello   world\n"), "Copied: Hello world", "the words were not tidied");
        compare(bar.about("Copied", ""), "Copied", "nothing to say about was still said");

        const long = "x".repeat(bar.longest + 40);
        const said = bar.about("Copied", long);
        verify(said.length < long.length, "a long saying was not cut short");
        verify(said.endsWith("…"), "a saying cut short does not say so");
    }

    name: "MessageBar"

    Component {
        id: barComponent

        MessageBar {
        }
    }
}
