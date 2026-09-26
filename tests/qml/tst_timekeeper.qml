import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function init() {
        keeper.reset();
        keeper.way = TimeKeeperViewModel.Down;
        keeper.setWantedParts(0, 25, 0);
    }

    function test_a_aCountdownStartsFromTheTimeItWasSet() {
        compare(keeper.way, TimeKeeperViewModel.Down);
        compare(keeper.wanted, 25 * 60 * 1000);
        compare(keeper.said, "25:00");
        verify(!keeper.running);
        verify(!keeper.started);
    }

    function test_b_theTimeIsSetInThePartsAReaderThinksIn() {
        keeper.setWantedParts(1, 2, 3);

        compare(keeper.wantedHours(), 1);
        compare(keeper.wantedMinutes(), 2);
        compare(keeper.wantedSeconds(), 3);
        compare(keeper.said, "1:02:03");
    }

    function test_c_aTimeTooLongOrTooShortIsBroughtIntoRange() {
        keeper.setWantedParts(0, 0, 0);
        compare(keeper.wanted, 1000, "a countdown is never shorter than a second");

        keeper.setWantedParts(48, 0, 0);
        compare(keeper.wanted, 24 * 60 * 60 * 1000, "a countdown is never longer than a day");
    }

    function test_d_aCountdownRunsPausesAndCarriesOn() {
        keeper.setWantedParts(0, 0, 30);

        keeper.start();
        verify(keeper.running);
        verify(keeper.started);
        wait(250);
        keeper.pause();

        verify(!keeper.running);
        verify(keeper.started, "a paused countdown is still under way");
        const held = keeper.gone;
        verify(held > 0, "no time was counted");

        wait(200);
        compare(keeper.gone, held, "a paused countdown went on counting");

        keeper.start();
        verify(keeper.running);
        wait(200);
        keeper.pause();
        verify(keeper.gone > held, "the countdown did not carry on");
    }

    function test_e_theOneButtonStartsAndPauses() {
        keeper.startOrPause();
        verify(keeper.running);

        keeper.startOrPause();
        verify(!keeper.running);
    }

    function test_f_resettingPutsTheTimeBack() {
        keeper.setWantedParts(0, 0, 30);
        keeper.start();
        wait(150);

        keeper.reset();

        verify(!keeper.running);
        verify(!keeper.started);
        compare(keeper.gone, 0);
        compare(keeper.said, "00:30");
    }

    function test_g_aCountdownRingsWhenItReachesNothing() {
        const rang = createTemporaryObject(spyComponent, testCase, {
            target: keeper,
            signalName: "rangOut"
        });
        keeper.setWantedParts(0, 0, 1);

        keeper.start();
        rang.wait(3000);

        verify(keeper.rang, "nothing says the time is up");
        verify(!keeper.running, "the countdown went past nothing");
        compare(keeper.left, 0);
        compare(keeper.said, "00:00");
        compare(keeper.howFar, 1);

        keeper.seen();
        verify(!keeper.rang);
    }

    function test_h_startingAfterItRangCountsAfresh() {
        keeper.setWantedParts(0, 0, 1);
        keeper.start();
        tryVerify(() => keeper.rang, 3000);

        keeper.start();

        verify(keeper.running);
        verify(keeper.left > 0, "the countdown did not start again");
    }

    function test_i_aStopwatchCountsUpFromNothing() {
        keeper.way = TimeKeeperViewModel.Up;

        compare(keeper.said, "00:00");
        compare(keeper.howFar, 0, "a stopwatch is never along at all");

        keeper.start();
        wait(250);
        keeper.pause();

        verify(keeper.gone > 0, "the stopwatch counted nothing");
        compare(keeper.left, keeper.gone);
        verify(!keeper.rang, "a stopwatch never rings");
    }

    function test_j_changingTheWayPutsTheCountBack() {
        keeper.setWantedParts(0, 0, 30);
        keeper.start();
        wait(150);

        keeper.way = TimeKeeperViewModel.Up;

        verify(!keeper.running);
        compare(keeper.gone, 0);
    }

    name: "TimeKeeper"

    TimeKeeperViewModel {
        id: keeper
    }

    Component {
        id: spyComponent

        SignalSpy {
        }
    }
}
