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

        compare(titles, ["File", "Edit", "View", "Insert", "Tools", "Window", "Help"]);
    }

    function test_b_commandsCarryTheirShortcut() {
        verify(AppInfo.shortcutText(actions.undo.shortcut).length > 0);
        verify(AppInfo.shortcutText(actions.copy.shortcut).length > 0);
        verify(AppInfo.shortcutText(actions.importDocument.shortcut).endsWith("I"));
        compare(AppInfo.shortcutText(actions.selectTool.shortcut), "V");
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
        const wanted = [["importWanted", actions.importDocument], ["exportWanted", actions.exportPdf], ["copyWanted", actions.saveCopy], ["trashWanted", actions.showTrash], ["aboutWanted", actions.showAbout], ["newNotebookWanted", actions.newNotebook]];
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
        const toggled = createTemporaryObject(spyComponent, testCase, {
            target: menuBar,
            signalName: "pagesPanelToggled"
        });
        const item = findChild(menuBar, "pagesPanelItem");
        verify(item !== null);

        item.triggered();

        compare(toggled.count, 1);
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

    AppActions {
        id: actions

        notebooks: notebooks
        tools: tools
    }

    AppMenuBar {
        id: menuBar

        actions: actions
        anchors.fill: parent
        notebooks: notebooks
        pagePanelShown: true
        pagesPanelShown: true
    }
}
