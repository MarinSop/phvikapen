pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

Item {
    id: root

    required property NotebooksViewModel notebooks
    required property SettingsViewModel settings
    required property ToolViewModel tools
    readonly property InkCanvas canvas: root.notebooks.canvas
    readonly property NotebookViewModel notebook: root.notebooks.current
    readonly property bool hasNotebook: root.notebook !== null && root.notebook.loaded
    readonly property bool hasSelection: root.canvas !== null && root.canvas.selectedCount > 0
    readonly property Action selectTool: Action {
        checked: root.tools.currentTool === ToolViewModel.Selection
        icon.source: Icons.select
        shortcut: root.keysFor("selectTool", "V")
        text: qsTr("Select")

        onTriggered: root.tools.currentTool = ToolViewModel.Selection
    }
    readonly property Action handTool: Action {
        checked: root.tools.currentTool === ToolViewModel.Hand
        icon.source: Icons.hand
        shortcut: root.keysFor("handTool", "H")
        text: qsTr("Hand")

        onTriggered: root.tools.currentTool = ToolViewModel.Hand
    }
    readonly property Action penTool: Action {
        checked: root.tools.currentTool === ToolViewModel.Pen
        icon.source: Icons.pen
        shortcut: root.keysFor("penTool", "P")
        text: qsTr("Pen")

        onTriggered: root.tools.currentTool = ToolViewModel.Pen
    }
    readonly property Action highlighterTool: Action {
        checked: root.tools.currentTool === ToolViewModel.Highlighter
        icon.source: Icons.highlighter
        shortcut: root.keysFor("highlighterTool", "M")
        text: qsTr("Highlighter")

        onTriggered: root.tools.currentTool = ToolViewModel.Highlighter
    }
    readonly property Action shapeTool: Action {
        checked: root.tools.currentTool === ToolViewModel.Shape
        icon.source: Icons.shape
        shortcut: root.keysFor("shapeTool", "U")
        text: qsTr("Shape")

        onTriggered: root.tools.currentTool = ToolViewModel.Shape
    }
    readonly property Action eraserTool: Action {
        checked: root.tools.currentTool === ToolViewModel.Eraser
        icon.source: Icons.eraser
        shortcut: root.keysFor("eraserTool", "E")
        text: qsTr("Eraser")

        onTriggered: root.tools.currentTool = ToolViewModel.Eraser
    }
    readonly property Action colourTool: Action {
        checked: root.tools.currentTool === ToolViewModel.ColourPicker
        icon.source: Icons.colourPicker
        shortcut: root.keysFor("colourTool", "K")
        text: qsTr("Color Picker")

        onTriggered: root.tools.currentTool = ToolViewModel.ColourPicker
    }
    readonly property list<Action> toolActions: [root.selectTool, root.handTool, root.penTool, root.highlighterTool, root.shapeTool, root.eraserTool, root.colourTool]
    readonly property Action undo: Action {
        enabled: root.notebook !== null && root.notebook.canUndo
        icon.source: Icons.undo
        shortcut: root.keysFor("undo", AppInfo.shortcutText(StandardKey.Undo))
        text: qsTr("Undo")

        onTriggered: root.notebook.undo()
    }
    readonly property Action redo: Action {
        enabled: root.notebook !== null && root.notebook.canRedo
        icon.source: Icons.redo
        shortcut: root.keysFor("redo", AppInfo.shortcutText(StandardKey.Redo))
        text: qsTr("Redo")

        onTriggered: root.notebook.redo()
    }
    readonly property Action copy: Action {
        enabled: root.hasSelection
        icon.source: Icons.copy
        shortcut: root.keysFor("copy", AppInfo.shortcutText(StandardKey.Copy))
        text: qsTr("Copy")

        onTriggered: root.notebook.copySelection()
    }
    readonly property Action paste: Action {
        enabled: root.notebook !== null && root.notebook.hasCopiedStrokes
        icon.source: Icons.paste
        shortcut: root.keysFor("paste", AppInfo.shortcutText(StandardKey.Paste))
        text: qsTr("Paste")

        onTriggered: root.notebook.pasteStrokes()
    }
    readonly property Action remove: Action {
        enabled: root.hasSelection
        icon.source: Icons.trash
        shortcut: root.keysFor("delete", AppInfo.shortcutText(StandardKey.Delete))
        text: qsTr("Delete")

        onTriggered: root.notebook.deleteSelection()
    }
    readonly property Action clearPage: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("clearPage", "Ctrl+Shift+Del")
        text: qsTr("Clear Page")

        onTriggered: root.notebook.clearPage()
    }
    readonly property Action zoomIn: Action {
        enabled: root.canvas !== null
        icon.source: Icons.zoomIn
        shortcut: root.keysFor("zoomIn", AppInfo.shortcutText(StandardKey.ZoomIn))
        text: qsTr("Zoom In")

        onTriggered: root.canvas.zoomIn()
    }
    readonly property Action zoomOut: Action {
        enabled: root.canvas !== null
        icon.source: Icons.zoomOut
        shortcut: root.keysFor("zoomOut", AppInfo.shortcutText(StandardKey.ZoomOut))
        text: qsTr("Zoom Out")

        onTriggered: root.canvas.zoomOut()
    }
    readonly property Action fitPage: Action {
        enabled: root.canvas !== null
        icon.source: Icons.fit
        shortcut: root.keysFor("fitPage", "Ctrl+0")
        text: qsTr("Fit Page")

        onTriggered: root.canvas.fitPage()
    }
    readonly property Action previousPage: Action {
        enabled: root.notebook !== null && root.notebook.hasPreviousPage
        icon.source: Icons.chevronLeft
        shortcut: root.keysFor("previousPage", AppInfo.shortcutText(StandardKey.MoveToPreviousPage))
        text: qsTr("Previous Page")

        onTriggered: root.notebook.previousPage()
    }
    readonly property Action nextPage: Action {
        enabled: root.notebook !== null && root.notebook.hasNextPage
        icon.source: Icons.chevronRight
        shortcut: root.keysFor("nextPage", AppInfo.shortcutText(StandardKey.MoveToNextPage))
        text: qsTr("Next Page")

        onTriggered: root.notebook.nextPage()
    }
    readonly property Action addPage: Action {
        enabled: root.hasNotebook
        icon.source: Icons.plus
        shortcut: root.keysFor("addPage", "Ctrl+Shift+P")
        text: qsTr("Page")

        onTriggered: root.notebook.addPage()
    }
    readonly property Action addSection: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("addSection", "Ctrl+Shift+N")
        text: qsTr("Section")

        onTriggered: root.notebook.addSection()
    }
    readonly property Action duplicatePage: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("duplicatePage", "Ctrl+D")
        text: qsTr("Duplicate Page")

        onTriggered: root.notebook.duplicatePage(root.notebook.currentPage)
    }
    readonly property Action newNotebook: Action {
        shortcut: root.keysFor("newNotebook", AppInfo.shortcutText(StandardKey.New))
        text: qsTr("New Notebook")

        onTriggered: root.newNotebookWanted()
    }
    readonly property Action closeNotebook: Action {
        enabled: root.notebook !== null
        shortcut: root.keysFor("closeNotebook", AppInfo.shortcutText(StandardKey.Close))
        text: qsTr("Close Notebook")

        onTriggered: root.askToClose(root.notebooks.currentIndex)
    }
    readonly property Action importDocument: Action {
        enabled: root.hasNotebook
        icon.source: Icons.importDocument
        shortcut: root.keysFor("import", "Ctrl+I")
        text: qsTr("PDF or Picture…")

        onTriggered: root.importWanted()
    }
    readonly property Action exportEverything: Action {
        enabled: root.hasNotebook && !root.notebook.exporting
        icon.source: Icons.exportDocument
        shortcut: root.keysFor("exportPdf", "Ctrl+E")
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
        shortcut: root.keysFor("save", AppInfo.shortcutText(StandardKey.Save))
        text: qsTr("Save")

        onTriggered: {
            if (!root.notebook.save()) {
                root.saveWanted();
            }
        }
    }
    readonly property Action saveCopy: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("saveCopy", AppInfo.shortcutText(StandardKey.SaveAs))
        text: qsTr("Save As…")

        onTriggered: root.saveWanted()
    }
    readonly property Action pageSetup: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("pageSetup", "Ctrl+Shift+U")
        text: qsTr("Page Setup…")

        onTriggered: root.pageSetupWanted()
    }
    readonly property Action showTrash: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("trash", "Ctrl+Shift+T")
        text: qsTr("Deleted Pages…")

        onTriggered: root.trashWanted()
    }
    readonly property Action showSettings: Action {
        shortcut: root.keysFor("settings", AppInfo.shortcutText(StandardKey.Preferences))
        text: qsTr("Settings…")

        onTriggered: root.settingsWanted()
    }
    readonly property Action showHints: Action {
        shortcut: root.keysFor("hints", "F1")
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
        shortcut: root.keysFor("continuousPages", "Ctrl+Shift+C")
        text: qsTr("Pages One Below the Other")

        onTriggered: root.settings.continuousPages = !root.settings.continuousPages
    }
    readonly property Action sectionsList: Action {
        checkable: true
        checked: root.settings.showSections
        shortcut: root.keysFor("sectionsList", "Ctrl+1")
        text: qsTr("Sections")

        onTriggered: root.settings.showSections = !root.settings.showSections
    }
    readonly property Action pagesList: Action {
        checkable: true
        checked: root.settings.showPages
        shortcut: root.keysFor("pagesList", "Ctrl+2")
        text: qsTr("Pages")

        onTriggered: root.settings.showPages = !root.settings.showPages
    }
    readonly property Action pagePanel: Action {
        checkable: true
        checked: root.settings.showPagePanel
        shortcut: root.keysFor("pagePanel", "Ctrl+3")
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
    signal trashWanted

    // Nothing is closed over the top of changes nobody has kept.
    function askToClose(index) {
        if (root.notebooks.isEdited(index)) {
            root.closeAsked(index);
        } else {
            root.notebooks.closeNotebook(index);
        }
    }

    function keysFor(commandId, fallback) {
        const kept = root.settings.shortcuts[commandId];
        return kept === undefined || kept === "" ? fallback : kept;
    }
}
