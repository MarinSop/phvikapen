import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function test_a_worksOutASumWithNothingUnknownInIt() {
        maths.ask("2 + 3 * 4");

        compare(maths.state, MathViewModel.Answered);
        compare(maths.answer, "14");
        compare(maths.letters, "");
        verify(!maths.drawable);
    }

    function test_b_solvesAStraightLine() {
        maths.ask("2x + 5 = 15");

        compare(maths.state, MathViewModel.Solved);
        compare(maths.letter, "x");
        compare(maths.answer, "x = 5");
        verify(maths.working.length >= 2, "the working was not shown");
        compare(maths.working[0].reason, MathViewModel.Gathered);
        compare(maths.working[maths.working.length - 1].reason, MathViewModel.Reached);
    }

    function test_c_solvesASquareWithTwoAnswers() {
        maths.ask("x^2 - 5x + 6 = 0");

        compare(maths.state, MathViewModel.Solved);
        compare(maths.answer, "x = 2, x = 3");
    }

    function test_d_saysWhyItCouldNotWorkSomethingOut() {
        maths.ask("2 +");

        compare(maths.state, MathViewModel.Refused);
        verify(maths.message !== "", "nothing was said about why");
        compare(maths.answer, "");
    }

    function test_e_saysWhenAStatementHoldsForEveryNumber() {
        maths.ask("2x + x = 3x");

        compare(maths.state, MathViewModel.Always);
    }

    function test_f_drawsAStraightLine() {
        maths.resetFrame();
        maths.ask("y = 2x + 1");

        verify(maths.drawable, "the line cannot be drawn");
        compare(maths.runs.length, 1);
        verify(maths.runs[0].length > 2, "the line has no points on it");
        compare(maths.curveMessage, "");
    }

    function test_g_drawsACircleAsTwoRuns() {
        maths.resetFrame();
        maths.ask("x^2 + y^2 = 25");

        verify(maths.drawable);
        compare(maths.runs.length, 2);
    }

    function test_h_theFrameMovesAndZooms() {
        maths.resetFrame();
        maths.ask("y = x");
        const was = maths.frame.left;

        maths.moveBy(5, 0);

        compare(maths.frame.left, was + 5);

        maths.zoomBy(0.5, 0, 0);

        verify(maths.frame.right - maths.frame.left < 20, "zooming in did not make the frame smaller");

        maths.resetFrame();

        compare(maths.frame.left, -10);
        compare(maths.frame.top, 10);
    }

    function test_i_aFrameWithNoRoomInItIsRefused() {
        maths.resetFrame();
        maths.ask("y = x");

        maths.look(1, 1, -1, 1);

        compare(maths.frame.left, -10, "a frame with no width was taken");
    }

    function test_j_anAnswerGoesBackOnThePageTheWayItIsRead() {
        maths.ask("50 * 4");
        compare(maths.written(), "50 * 4 = 200");

        maths.ask("2x = 8");
        compare(maths.written(), "x = 4");
    }

    function test_k_aSumEndingInAnEqualsSignIsWorkedOut() {
        compare(maths.quickAnswer("50*4="), "200");
        compare(maths.quickAnswer(" 12 + 30 = "), "42");
        compare(maths.quickAnswer("(2+3)*4="), "20");
        compare(maths.quickAnswer("7/2="), "3.5");
    }

    function test_l_ordinaryWritingIsNeverWorkedOut() {
        compare(maths.quickAnswer("what is this="), "");
        compare(maths.quickAnswer("Monday="), "");
        compare(maths.quickAnswer("50*4"), "", "there is no equals sign to ask with");
        compare(maths.quickAnswer("2x + 1 ="), "", "a letter is not a number");
        compare(maths.quickAnswer("="), "");
        compare(maths.quickAnswer("1/0="), "", "nothing can be divided by nothing");
    }

    function test_m_aStatementWithTwoLettersIsDrawnRatherThanAnswered() {
        maths.resetFrame();
        maths.ask("y = 2x");

        compare(maths.letters, "xy");
        compare(maths.state, MathViewModel.Drawn);
        compare(maths.answer, "");
        verify(maths.drawable, "a statement with two letters has a curve");
        verify(maths.runs.length > 0);
    }

    function test_n_forgettingClearsEverything() {
        maths.ask("2 + 2");
        maths.forget();

        compare(maths.state, MathViewModel.Nothing);
        compare(maths.answer, "");
        compare(maths.said, "");
        compare(maths.runs.length, 0);
    }

    name: "MathViewModel"

    MathViewModel {
        id: maths
    }
}
