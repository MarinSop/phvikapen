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

        // A tool is chosen from the palette down the side, so the menus do not offer them again.
        compare(titles, ["File", "Edit", "View", "Insert", "Help"]);
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
        const sections = findChild(menuBar, "sectionsPanelItem");
        const pages = findChild(menuBar, "pagesPanelItem");
        verify(sections !== null);
        verify(pages !== null);
        verify(workspace.isOpen("sections"));

        sections.action.trigger();
        verify(!workspace.isOpen("sections"));
        pages.action.trigger();
        verify(!workspace.isOpen("pages"));

        workspace.resetWorkspace();
    }

    function test_x1_aLineOfAMenuThatIsNotShownTakesUpNoRoom() {
        const command = createTemporaryObject(commandComponent, testCase, {
            text: "Something"
        });

        verify(command.height > 0);

        command.visible = false;

        compare(command.height, 0);
    }

    function test_x2_aRuleBetweenGroupsThatIsNotShownTakesUpNoRoom() {
        const line = createTemporaryObject(lineComponent, testCase);

        verify(line.height > 0);

        line.visible = false;

        compare(line.height, 0);
    }

    function test_y1_theMenuUnderThePointerIsAsTallAsWhatItOffers() {
        const menu = createTemporaryObject(contextComponent, testCase, {
            actions: actions
        });

        menu.popup(0, 0);
        tryVerify(() => menu.contentItem.contentHeight > 0);

        let shown = 0;
        let tall = 0;
        for (let i = 0; i < menu.count; ++i) {
            const line = menu.itemAt(i);
            if (line.visible) {
                shown += 1;
                tall += line.height;
            }
        }
        verify(shown > 0);
        verify(shown < menu.count, "nothing was left out, so there is nothing to prove");
        compare(menu.contentItem.contentHeight, tall);
        menu.close();
    }

    function test_y2_theMenuIsNoWiderThanTheWidestLineItIsShowing() {
        const menu = createTemporaryObject(contextComponent, testCase, {
            actions: actions
        });

        menu.popup(0, 0);
        tryVerify(() => menu.implicitWidth > 0);

        let widest = 0;
        let leftOut = 0;
        for (let i = 0; i < menu.count; ++i) {
            const line = menu.itemAt(i);
            if (line.visible) {
                widest = Math.max(widest, line.implicitWidth);
            } else {
                leftOut += 1;
            }
        }
        verify(widest > 0);
        verify(leftOut > 0, "every line is shown, so there is nothing to prove");
        verify(menu.implicitWidth <= Math.ceil(widest) + 1, "the menu is " + menu.implicitWidth + " wide for a line of " + widest);
        menu.close();
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

    Component {
        id: commandComponent

        MenuCommand {
        }
    }

    Component {
        id: lineComponent

        MenuLine {
        }
    }

    Component {
        id: contextComponent

        ContextMenu {
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

    MathViewModel {
        id: maths
    }

    TimeKeeperViewModel {
        id: timeKeeper
    }

    AppActions {
        id: actions

        maths: maths
        timeKeeper: timeKeeper
        notebooks: notebooks
        settings: settings
        tools: tools
        workspace: workspace
    }

    WorkspaceViewModel {
        id: workspace
    }

    AppMenuBar {
        id: menuBar

        actions: actions
        anchors.fill: parent
        notebooks: notebooks
    }
}
