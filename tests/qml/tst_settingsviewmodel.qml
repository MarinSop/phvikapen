import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function test_a_looksForUpdatesUntilToldOtherwise() {
        const settings = createTemporaryObject(settingsComponent, testCase);

        compare(settings.lookForUpdates, true);
    }

    function test_b_whatWasSetIsRememberedForTheNextTime() {
        const settings = createTemporaryObject(settingsComponent, testCase);
        settings.lookForUpdates = false;

        const later = createTemporaryObject(settingsComponent, testCase);

        compare(later.lookForUpdates, false);
        later.lookForUpdates = true;
    }

    function test_c_saysWhereNotebooksAreKept() {
        const settings = createTemporaryObject(settingsComponent, testCase);

        verify(settings.notebookFolder.length > 0);
        verify(settings.notebookFolder.endsWith("notebooks"));
    }

    function test_d_newNotebooksStartOnA4LinedPaper() {
        const settings = createTemporaryObject(settingsComponent, testCase);

        compare(settings.paper, PageOptions.A4);
        compare(settings.background, PageOptions.Lined);
        compare(settings.landscape, false);
    }

    function test_e_thePaperForNewNotebooksIsRemembered() {
        const settings = createTemporaryObject(settingsComponent, testCase);
        settings.paper = PageOptions.A5;
        settings.background = PageOptions.Dotted;
        settings.landscape = true;

        const later = createTemporaryObject(settingsComponent, testCase);

        compare(later.paper, PageOptions.A5);
        compare(later.background, PageOptions.Dotted);
        compare(later.landscape, true);
    }

    name: "SettingsViewModel"

    Component {
        id: settingsComponent

        SettingsViewModel {
        }
    }
}
