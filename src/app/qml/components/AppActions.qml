pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

// Every command of the application, named once. The menus, the palette, the bar of options, the
// menu that opens under the pointer and the keyboard all reach the same objects, so a command
// cannot behave one way in one place and another way somewhere else.
Item {
    id: root

    required property NotebooksViewModel notebooks
    required property SettingsViewModel settings
    required property ToolViewModel tools
    readonly property InkCanvas canvas: root.notebooks.canvas
    readonly property NotebookViewModel notebook: root.notebooks.current
    readonly property bool hasNotebook: root.notebook !== null && root.notebook.loaded
    readonly property bool hasSelection: root.canvas !== null && root.canvas.selectedCount > 0
    readonly property bool hasTextBox: root.notebook !== null && root.notebook.pickedText !== ""
    // While words are being typed the keyboard belongs to whoever is typing them: a command with
    // a plain letter for a key, and the ones an editor owns itself, stand aside.
    readonly property bool typing: AppInfo.typing
    readonly property Action selectTool: Action {
        checked: root.tools.currentTool === ToolViewModel.Selection
        icon.source: Icons.select
        shortcut: root.pageKeys("selectTool")
        text: qsTr("Select")

        onTriggered: root.tools.currentTool = ToolViewModel.Selection
    }
    readonly property Action handTool: Action {
        checked: root.tools.currentTool === ToolViewModel.Hand
        icon.source: Icons.hand
        shortcut: root.pageKeys("handTool")
        text: qsTr("Hand")

        onTriggered: root.tools.currentTool = ToolViewModel.Hand
    }
    readonly property Action penTool: Action {
        checked: root.tools.currentTool === ToolViewModel.Pen
        icon.source: Icons.pen
        shortcut: root.pageKeys("penTool")
        text: qsTr("Pen")

        onTriggered: root.tools.currentTool = ToolViewModel.Pen
    }
    readonly property Action highlighterTool: Action {
        checked: root.tools.currentTool === ToolViewModel.Highlighter
        icon.source: Icons.highlighter
        shortcut: root.pageKeys("highlighterTool")
        text: qsTr("Highlighter")

        onTriggered: root.tools.currentTool = ToolViewModel.Highlighter
    }
    readonly property Action shapeTool: Action {
        checked: root.tools.currentTool === ToolViewModel.Shape
        icon.source: Icons.shape
        shortcut: root.pageKeys("shapeTool")
        text: qsTr("Shape")

        onTriggered: root.tools.currentTool = ToolViewModel.Shape
    }
    readonly property Action eraserTool: Action {
        checked: root.tools.currentTool === ToolViewModel.Eraser
        icon.source: Icons.eraser
        shortcut: root.pageKeys("eraserTool")
        text: qsTr("Eraser")

        onTriggered: root.tools.currentTool = ToolViewModel.Eraser
    }
    readonly property Action colourTool: Action {
        checked: root.tools.currentTool === ToolViewModel.ColourPicker
        icon.source: Icons.colourPicker
        shortcut: root.pageKeys("colourTool")
        text: qsTr("Color Picker")

        onTriggered: root.tools.currentTool = ToolViewModel.ColourPicker
    }
    readonly property Action textTool: Action {
        checked: root.tools.currentTool === ToolViewModel.Text
        icon.source: Icons.text
        shortcut: root.pageKeys("textTool")
        text: qsTr("Text")

        onTriggered: root.tools.currentTool = ToolViewModel.Text
    }
    readonly property list<Action> toolActions: [root.selectTool, root.handTool, root.penTool, root.highlighterTool, root.shapeTool, root.eraserTool, root.textTool, root.colourTool]
    readonly property Action eraserMode: Action {
        enabled: root.tools.currentTool === ToolViewModel.Eraser
        icon.source: root.tools.eraserMode === ToolViewModel.WholeStroke ? Icons.eraseWhole : Icons.eraser
        shortcut: root.pageKeys("eraserMode")
        text: root.tools.eraserMode === ToolViewModel.WholeStroke ? qsTr("Erase Part of a Line") : qsTr("Erase a Whole Line")

        onTriggered: root.tools.eraserMode = root.tools.eraserMode === ToolViewModel.WholeStroke ? ToolViewModel.Touched : ToolViewModel.WholeStroke
    }
    readonly property Action undo: Action {
        enabled: root.notebook !== null && root.notebook.canUndo
        icon.source: Icons.undo
        shortcut: root.pageKeys("undo")
        text: qsTr("Undo")

        onTriggered: root.notebook.undo()
    }
    readonly property Action redo: Action {
        enabled: root.notebook !== null && root.notebook.canRedo
        icon.source: Icons.redo
        shortcut: root.pageKeys("redo")
        text: qsTr("Redo")

        onTriggered: root.notebook.redo()
    }
    readonly property Action copy: Action {
        enabled: root.hasSelection
        icon.source: Icons.copy
        shortcut: root.pageKeys("copy")
        text: qsTr("Copy")

        onTriggered: root.notebook.copySelection()
    }
    readonly property Action cut: Action {
        enabled: root.hasSelection
        icon.source: Icons.cut
        shortcut: root.pageKeys("cut")
        text: qsTr("Cut")

        onTriggered: root.notebook.cutSelection()
    }
    readonly property Action duplicate: Action {
        enabled: root.hasSelection
        icon.source: Icons.duplicate
        shortcut: root.pageKeys("duplicate")
        text: qsTr("Duplicate")

        onTriggered: root.notebook.duplicateSelection()
    }
    readonly property Action selectAll: Action {
        enabled: root.hasNotebook
        icon.source: Icons.selectAll
        shortcut: root.pageKeys("selectAll")
        text: qsTr("Select All on the Page")

        onTriggered: {
            root.tools.currentTool = ToolViewModel.Selection;
            root.canvas.selectEverything();
        }
    }
    readonly property Action copyAsText: Action {
        enabled: root.hasSelection && root.notebook !== null && root.notebook.readsHandwriting
        shortcut: root.pageKeys("copyAsText")
        text: qsTr("Copy as Text")

        onTriggered: root.notebook.copySelectionAsText()
    }
    readonly property Action convertToText: Action {
        enabled: root.hasSelection && root.notebook !== null && root.notebook.readsHandwriting
        shortcut: root.pageKeys("convertToText")
        text: qsTr("Convert to Text")

        onTriggered: root.notebook.convertSelectionToText(root.tools.textStyle)
    }
    readonly property Action paste: Action {
        enabled: root.notebook !== null && root.notebook.hasCopiedStrokes
        icon.source: Icons.paste
        shortcut: root.pageKeys("paste")
        text: qsTr("Paste")

        onTriggered: root.notebook.pasteStrokes()
    }
    readonly property Action remove: Action {
        enabled: root.hasSelection || root.hasTextBox
        icon.source: Icons.trash
        shortcut: root.pageKeys("delete")
        text: qsTr("Delete")

        onTriggered: root.deleteWhatIsPicked()
    }
    readonly property Action rotateLeft: Action {
        enabled: root.hasSelection
        icon.source: Icons.turnLeft
        shortcut: root.pageKeys("rotateLeft")
        text: qsTr("Turn Left")

        onTriggered: root.notebook.turnSelection(-root.quarterTurn)
    }
    readonly property Action rotateRight: Action {
        enabled: root.hasSelection
        icon.source: Icons.turnRight
        shortcut: root.pageKeys("rotateRight")
        text: qsTr("Turn Right")

        onTriggered: root.notebook.turnSelection(root.quarterTurn)
    }
    readonly property real quarterTurn: 90
    readonly property Action clearPage: Action {
        enabled: root.hasNotebook
        shortcut: root.pageKeys("clearPage")
        text: qsTr("Clear Page")

        onTriggered: root.notebook.clearPage()
    }
    readonly property Action zoomIn: Action {
        enabled: root.canvas !== null
        icon.source: Icons.zoomIn
        shortcut: root.keysFor("zoomIn")
        text: qsTr("Zoom In")

        onTriggered: root.canvas.zoomIn()
    }
    readonly property Action zoomOut: Action {
        enabled: root.canvas !== null
        icon.source: Icons.zoomOut
        shortcut: root.keysFor("zoomOut")
        text: qsTr("Zoom Out")

        onTriggered: root.canvas.zoomOut()
    }
    readonly property Action fitPage: Action {
        enabled: root.canvas !== null
        icon.source: Icons.fit
        shortcut: root.keysFor("fitPage")
        text: qsTr("Fit Page")

        onTriggered: root.canvas.fitPage()
    }
    readonly property Action previousPage: Action {
        enabled: root.notebook !== null && root.notebook.hasPreviousPage
        icon.source: Icons.chevronLeft
        shortcut: root.keysFor("previousPage")
        text: qsTr("Previous Page")

        onTriggered: root.notebook.previousPage()
    }
    readonly property Action nextPage: Action {
        enabled: root.notebook !== null && root.notebook.hasNextPage
        icon.source: Icons.chevronRight
        shortcut: root.keysFor("nextPage")
        text: qsTr("Next Page")

        onTriggered: root.notebook.nextPage()
    }
    readonly property Action addPage: Action {
        enabled: root.hasNotebook
        icon.source: Icons.plus
        shortcut: root.keysFor("addPage")
        text: qsTr("Page")

        onTriggered: root.notebook.addPage()
    }
    readonly property Action addSection: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("addSection")
        text: qsTr("Section")

        onTriggered: root.notebook.addSection()
    }
    readonly property Action duplicatePage: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("duplicatePage")
        text: qsTr("Duplicate Page")

        onTriggered: root.notebook.duplicatePage(root.notebook.currentPage)
    }
    readonly property Action newNotebook: Action {
        shortcut: root.keysFor("newNotebook")
        text: qsTr("New Notebook")

        onTriggered: root.newNotebookWanted()
    }
    readonly property Action closeNotebook: Action {
        enabled: root.notebook !== null
        shortcut: root.keysFor("closeNotebook")
        text: qsTr("Close Notebook")

        onTriggered: root.askToClose(root.notebooks.currentIndex)
    }
    readonly property Action importDocument: Action {
        enabled: root.hasNotebook
        icon.source: Icons.importDocument
        shortcut: root.keysFor("import")
        text: qsTr("PDF or Picture…")

        onTriggered: root.importWanted()
    }
    readonly property Action exportEverything: Action {
        enabled: root.hasNotebook && !root.notebook.exporting
        icon.source: Icons.exportDocument
        shortcut: root.keysFor("exportPdf")
        text: qsTr("Sheets and Everything Around Them…")

        onTriggered: root.exportWanted(0)
    }
    readonly property Action exportSheets: Action {
        enabled: root.hasNotebook && !root.notebook.exporting
        text: qsTr("Only the Sheets…")

        onTriggered: root.exportWanted(1)
    }
    readonly property Action exportImported: Action {
        enabled: root.hasNotebook && !root.notebook.exporting
        text: qsTr("Only the Imported Document…")

        onTriggered: root.exportWanted(2)
    }
    readonly property Action save: Action {
        enabled: root.hasNotebook
        icon.source: Icons.save
        shortcut: root.keysFor("save")
        text: qsTr("Save")

        onTriggered: {
            if (!root.notebook.save()) {
                root.saveWanted();
            }
        }
    }
    readonly property Action saveCopy: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("saveCopy")
        text: qsTr("Save As…")

        onTriggered: root.saveWanted()
    }
    readonly property Action pageSetup: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("pageSetup")
        text: qsTr("Page Setup…")

        onTriggered: root.pageSetupWanted()
    }
    readonly property Action findWriting: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("find")
        text: qsTr("Find in the Notebook…")

        onTriggered: root.findWanted()
    }
    readonly property Action showTrash: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("trash")
        text: qsTr("Deleted Pages…")

        onTriggered: root.trashWanted()
    }
    readonly property Action showSettings: Action {
        shortcut: root.keysFor("settings")
        text: qsTr("Settings…")

        onTriggered: root.settingsWanted()
    }
    readonly property Action showHints: Action {
        shortcut: root.keysFor("hints")
        text: qsTr("Keys and Hints…")

        onTriggered: root.hintsWanted()
    }
    readonly property Action showAbout: Action {
        text: qsTr("About PhvikaPen")

        onTriggered: root.aboutWanted()
    }
    readonly property Action continuousPages: Action {
        checkable: true
        checked: root.settings.continuousPages
        shortcut: root.keysFor("continuousPages")
        text: qsTr("Pages One Below the Other")

        onTriggered: root.settings.continuousPages = !root.settings.continuousPages
    }
    readonly property Action sectionsList: Action {
        checkable: true
        checked: root.settings.showSections
        shortcut: root.keysFor("sectionsList")
        text: qsTr("Sections")

        onTriggered: root.settings.showSections = !root.settings.showSections
    }
    readonly property Action pagesList: Action {
        checkable: true
        checked: root.settings.showPages
        shortcut: root.keysFor("pagesList")
        text: qsTr("Pages")

        onTriggered: root.settings.showPages = !root.settings.showPages
    }
    readonly property Action pagePanel: Action {
        checkable: true
        checked: root.settings.showPagePanel
        shortcut: root.keysFor("pagePanel")
        text: qsTr("Page Setup Panel")

        onTriggered: root.settings.showPagePanel = !root.settings.showPagePanel
    }
    readonly property Action quit: Action {
        shortcut: StandardKey.Quit
        text: qsTr("Quit")

        onTriggered: root.leaveWanted()
    }

    signal aboutWanted
    signal closeAsked(int index)
    signal leaveWanted
    signal saveWanted
    signal exportWanted(int scope)
    signal hintsWanted
    signal importWanted
    signal newNotebookWanted
    signal pageSetupWanted
    signal settingsWanted
    signal findWanted
    signal trashWanted

    // Nothing is closed over the top of changes nobody has kept.
    function askToClose(index) {
        if (root.notebooks.isEdited(index)) {
            root.closeAsked(index);
        } else {
            root.notebooks.closeNotebook(index);
        }
    }

    // One key removes whatever is picked up, whether that is ink or a box of words.
    function deleteWhatIsPicked() {
        if (root.hasSelection) {
            root.notebook.deleteSelection();
        } else if (root.hasTextBox) {
            root.notebook.removeText(root.notebook.pickedText);
        }
    }

    // Read through the map of what the reader changed, so that changing a key in the settings
    // takes hold at once rather than the next time the application starts.
    function keysFor(commandId) {
        const kept = root.settings.shortcuts[commandId];
        return kept === undefined || kept === "" ? root.settings.defaultKeys(commandId) : kept;
    }

    // A key that belongs to the page rather than to the window: it is given up while words are
    // being typed, so that typing a letter types it.
    function pageKeys(commandId) {
        return root.typing ? "" : root.settings.keysFor(commandId);
    }

    // The key a Mac keyboard sends for Delete, beside the one the command came with.
    Shortcut {
        enabled: !root.typing && (root.hasSelection || root.hasTextBox)
        sequences: ["Backspace"]

        onActivated: root.deleteWhatIsPicked()
    }
}
