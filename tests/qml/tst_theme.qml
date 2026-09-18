import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function cleanupTestCase() {
        Theme.mode = Theme.Brand;
    }

    function test_a_everyThemeHasItsOwnColours() {
        const brand = Theme.coloursOf(Theme.Brand);
        const dark = Theme.coloursOf(Theme.Dark);
        const light = Theme.coloursOf(Theme.Light);

        verify(brand.window !== dark.window);
        verify(dark.window !== light.window);
        verify(brand.accent !== dark.accent);
        verify(Theme.nameOf(Theme.Light) === "Light");
        verify(Theme.noteOf(Theme.Brand).length > 0);
    }

    function test_b_theColoursFollowTheThemeThatIsPicked() {
        Theme.mode = Theme.Light;

        compare(Theme.window.toString(), Qt.color(Theme.lightColours.window).toString());
        verify(!Theme.dark);

        Theme.mode = Theme.Brand;

        compare(Theme.window.toString(), Qt.color(Theme.brandColours.window).toString());
        verify(Theme.dark);
        verify(Theme.logo.toString().length > 0);
    }

    name: "Theme"
}
