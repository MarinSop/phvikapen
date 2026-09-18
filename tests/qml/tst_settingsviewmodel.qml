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

    name: "SettingsViewModel"

    Component {
        id: settingsComponent

        SettingsViewModel {
        }
    }
}
