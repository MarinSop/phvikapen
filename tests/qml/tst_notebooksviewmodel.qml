import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function test_a_startsWithOneNotebook() {
        compare(notebooks.titles.length, 1);
    }

    function test_b_opensAndClosesNotebooks() {
        notebooks.addNotebook();
        compare(notebooks.titles.length, 2);

        notebooks.closeNotebook(0);
        compare(notebooks.titles.length, 1);
    }

    function test_c_ignoresAnIndexOutsideTheList() {
        const before = notebooks.titles.length;

        notebooks.closeNotebook(before + 5);
        notebooks.closeNotebook(-1);

        compare(notebooks.titles.length, before);
    }

    name: "NotebooksViewModel"

    NotebooksViewModel {
        id: notebooks
    }
}
