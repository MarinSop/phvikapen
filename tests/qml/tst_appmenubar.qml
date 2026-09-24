import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function test_a_hasTheUsualMenus() {
        const titles = [];
        for (let i = 0; i < menuBar.count; ++i) {
            titles.push(menuBar.menuAt(i).title.replace("&", ""));
        }

        compare(titles, ["File", "Edit", "Tools", "View", "Insert", "Help"]);
    }

    function test_b_commandsCarryTheirShortcut() {
        verify(AppInfo.shortcutText(actions.undo.shortcut).length > 0);
        verify(AppInfo.shortcutText(actions.copy.shortcut).length > 0);
        verify(AppInfo.shortcutText(actions.importDocument.shortcut).endsWith("I"));
        compare(AppInfo.shortcutText(actions.selectTool.shortcut), "V");
    }

    function test_b2_everyCommandKeepsItsOwnKeys() {
        const seen = {};
        for (const id of settings.shortcutList.ids()) {
            const keys = actions.keysFor(id);
            verify(keys.length > 0, id + " answers to nothing");
            verify(seen[keys] === undefined, keys + " is asked of both " + seen[keys] + " and " + id);
            seen[keys] = id;
        }
    }

    function test_b3_theKeysAreWrittenTheWayTheyAreReadBack() {
        // What is shown beside a command is written for the platform and cannot be read back, so
        // what a command answers to is never taken from there.
        verify(!actions.keysFor("undo").startsWith("\u2318"));
        compare(actions.keysFor("undo"), "Ctrl+Z");
        compare(actions.keysFor("textTool"), "T");
    }

    function test_b4_lettersStandAsideWhileWordsAreTyped() {
        compare(actions.pageKeys("textTool"), "T");
        verify(actions.keysFor("save").length > 0);
    }

    function test_c_commandsThatNeedANotebookStayOff() {
        verify(!actions.undo.enabled);
        verify(!actions.remove.enabled);
        verify(actions.showSettings.enabled);
    }

    function test_d_theMenusAskForTheDialogsInsteadOfOpeningThem() {
        const asked = createTemporaryObject(spyComponent, testCase, {
            target: actions,
            signalName: "settingsWanted"
        });

        actions.showSettings.trigger();

        compare(asked.count, 1);
    }

    function test_d2_everyDialogIsAskedForByItsCommand() {
        const wanted = [["importWanted", actions.importDocument], ["exportWanted", actions.exportEverything], ["saveWanted", actions.saveCopy], ["trashWanted", actions.showTrash], ["aboutWanted", actions.showAbout], ["newNotebookWanted", actions.newNotebook]];
        for (const pair of wanted) {
            const asked = createTemporaryObject(spyComponent, testCase, {
                target: actions,
                signalName: pair[0]
            });
            pair[1].enabled = true;
            pair[1].trigger();
            compare(asked.count, 1, pair[0] + " was not asked for");
        }
    }

    function test_e_thePanelsCanBeTurnedOff() {
        const sections = findChild(menuBar, "sectionsListItem");
        const pages = findChild(menuBar, "pagesListItem");
        verify(sections !== null);
        verify(pages !== null);

        sections.action.trigger();
        compare(settings.showSections, false);
        pages.action.trigger();
        compare(settings.showPages, false);

        settings.showSections = true;
        settings.showPages = true;
    }

    height: 60
    name: "AppMenuBar"
    visible: true
    when: windowShown
    width: 800

    Component {
        id: spyComponent

        SignalSpy {
        }
    }

    ToolViewModel {
        id: tools
    }

    NotebooksViewModel {
        id: notebooks

        directory: temporaryDirectory + "/menus"
    }

    SettingsViewModel {
        id: settings
    }

    AppActions {
        id: actions

        notebooks: notebooks
        settings: settings
        tools: tools
    }

    AppMenuBar {
        id: menuBar

        actions: actions
        anchors.fill: parent
        notebooks: notebooks
    }
}
