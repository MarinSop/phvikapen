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

    function test_f_keysComeWithTheirUsualValues() {
        const settings = createTemporaryObject(settingsComponent, testCase);

        verify(settings.shortcutList.count > 10);
        compare(settings.shortcuts["undo"], undefined);
        const row = settings.shortcutList.index(0, 0);
        compare(settings.shortcutList.data(row, Qt.UserRole + 1), "undo");
        compare(settings.shortcutList.data(row, Qt.UserRole + 3), "Ctrl+Z");
        compare(settings.shortcutList.data(row, Qt.UserRole + 4), false);
    }

    function test_g_aKeyCanBeChangedAndPutBack() {
        const settings = createTemporaryObject(settingsComponent, testCase);

        verify(settings.changeShortcut("undo", "Ctrl+Alt+Z"));

        compare(settings.shortcuts["undo"], "Ctrl+Alt+Z");
        const later = createTemporaryObject(settingsComponent, testCase);
        compare(later.shortcuts["undo"], "Ctrl+Alt+Z");

        settings.resetShortcut("undo");
        compare(settings.shortcuts["undo"], undefined);
    }

    function test_h_aKeyThatIsTakenIsRefused() {
        const settings = createTemporaryObject(settingsComponent, testCase);

        compare(settings.conflictWith("undo", "Ctrl+C"), "Copy");
        verify(!settings.changeShortcut("undo", "Ctrl+C"));
        compare(settings.shortcuts["undo"], undefined);
        compare(settings.conflictWith("undo", "Ctrl+Alt+Q"), "");
    }

    name: "SettingsViewModel"

    Component {
        id: settingsComponent

        SettingsViewModel {
        }
    }
}
