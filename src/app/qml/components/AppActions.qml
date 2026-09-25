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
    readonly property bool hasPicture: root.notebook !== null && root.notebook.pickedPicture !== ""
    readonly property bool hasTable: root.notebook !== null && root.notebook.pickedTable !== ""
    // A table starts as a plain grid; rows and columns are added to it afterwards.
    readonly property int plainTableColumns: 3
    readonly property int plainTableRows: 3
    // Which row and column of the table in hand the reader is working in. The layer that holds the
    // editor keeps this up to date, so that a row is added beside the box being typed in.
    property int rowInHand: 0
    property int columnInHand: 0
    // The stretch of boxes marked out in the table in hand, which is what joining them asks for.
    property int stretchFromColumn: -1
    property int stretchFromRow: -1
    property int stretchToColumn: -1
    property int stretchToRow: -1
    readonly property bool hasStretch: root.hasTable && root.stretchFromRow >= 0 && (root.stretchFromRow !== root.stretchToRow || root.stretchFromColumn !== root.stretchToColumn)
    // The boxes a table command reaches: the stretch that is marked out, or the one box being
    // worked in where nothing is marked. Every command reads these, so one box and many behave
    // alike and nothing has to know which of the two it was given.
    readonly property int reachFromColumn: root.hasStretch ? Math.min(root.stretchFromColumn, root.stretchToColumn) : root.columnInHand
    readonly property int reachFromRow: root.hasStretch ? Math.min(root.stretchFromRow, root.stretchToRow) : root.rowInHand
    readonly property int reachToColumn: root.hasStretch ? Math.max(root.stretchFromColumn, root.stretchToColumn) : root.columnInHand
    readonly property int reachToRow: root.hasStretch ? Math.max(root.stretchFromRow, root.stretchToRow) : root.rowInHand
    // While words are being typed the keyboard belongs to whoever is typing them: a command with
    // a plain letter for a key, and the ones an editor owns itself, stand aside.
    readonly property bool typing: AppInfo.typing
    // How the first box of what is reached is shown, so that bold and slant turn off again when
    // they are already on.
    readonly property var lookInHand: root.hasTable ? root.notebook.cellLook(root.notebook.pickedTable, root.reachFromRow, root.reachFromColumn) : ({})
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
        icon.source: Icons.toText
        shortcut: root.pageKeys("copyAsText")
        text: qsTr("Copy as Text")

        onTriggered: root.notebook.copySelectionAsText()
    }
    readonly property Action convertToText: Action {
        enabled: root.hasSelection && root.notebook !== null && root.notebook.readsHandwriting
        icon.source: Icons.toText
        shortcut: root.pageKeys("convertToText")
        text: qsTr("Convert to Text")

        onTriggered: root.notebook.convertSelectionToText(root.tools.textStyle)
    }
    readonly property Action solve: Action {
        enabled: root.hasTextBox || (root.hasSelection && root.notebook !== null && root.notebook.readsHandwriting)
        icon.source: Icons.solve
        shortcut: root.pageKeys("solve")
        text: qsTr("Solve")

        onTriggered: root.notebook.solveSelection(root.tools.textStyle)
    }
    readonly property Action paste: Action {
        enabled: root.notebook !== null && root.notebook.hasCopiedStrokes
        icon.source: Icons.paste
        shortcut: root.pageKeys("paste")
        text: qsTr("Paste")

        onTriggered: root.notebook.pasteStrokes()
    }
    readonly property Action insertPicture: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("insertPicture")
        text: qsTr("Picture on This Page…")

        onTriggered: root.pictureWanted()
    }
    readonly property Action insertTable: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("insertTable")
        text: qsTr("Table")

        onTriggered: root.notebook.addTable(root.plainTableRows, root.plainTableColumns)
    }
    readonly property Action insertEquation: Action {
        enabled: root.hasNotebook
        shortcut: root.keysFor("insertEquation")
        text: qsTr("Equation")

        onTriggered: root.notebook.addEquation(root.tools.textStyle)
    }
    readonly property Action addRowAbove: Action {
        enabled: root.hasTable
        text: qsTr("Insert Row Above")

        onTriggered: root.notebook.addRow(root.notebook.pickedTable, root.rowInHand)
    }
    readonly property Action addRowBelow: Action {
        enabled: root.hasTable
        text: qsTr("Insert Row Below")

        onTriggered: root.notebook.addRow(root.notebook.pickedTable, root.rowInHand + 1)
    }
    readonly property Action addColumnBefore: Action {
        enabled: root.hasTable
        text: qsTr("Insert Column Left")

        onTriggered: root.notebook.addColumn(root.notebook.pickedTable, root.columnInHand)
    }
    readonly property Action addColumnAfter: Action {
        enabled: root.hasTable
        text: qsTr("Insert Column Right")

        onTriggered: root.notebook.addColumn(root.notebook.pickedTable, root.columnInHand + 1)
    }
    readonly property Action mergeCells: Action {
        enabled: root.hasStretch
        text: qsTr("Merge Boxes")

        onTriggered: root.notebook.mergeCells(root.notebook.pickedTable, root.reachFromRow, root.reachFromColumn, root.reachToRow, root.reachToColumn)
    }
    readonly property Action splitCell: Action {
        enabled: root.hasTable
        text: qsTr("Split Box")

        onTriggered: root.notebook.splitCell(root.notebook.pickedTable, root.rowInHand, root.columnInHand)
    }
    readonly property Action removeRow: Action {
        enabled: root.hasTable
        text: qsTr("Delete Row")

        onTriggered: root.notebook.removeRow(root.notebook.pickedTable, root.rowInHand)
    }
    // How the words of the box in hand line up, counted the way a run of type counts it.
    readonly property Action alignCellLeft: Action {
        enabled: root.hasTable
        icon.source: Icons.alignLeft
        text: qsTr("Align Left")

        onTriggered: root.alignTheBoxesInHand(0)
    }
    readonly property Action alignCellCentre: Action {
        enabled: root.hasTable
        icon.source: Icons.alignCenter
        text: qsTr("Center")

        onTriggered: root.alignTheBoxesInHand(1)
    }
    readonly property Action alignCellRight: Action {
        enabled: root.hasTable
        icon.source: Icons.alignRight
        text: qsTr("Align Right")

        onTriggered: root.alignTheBoxesInHand(2)
    }
    readonly property Action boldCells: Action {
        enabled: root.hasTable
        icon.source: Icons.bold
        text: qsTr("Bold Boxes")

        onTriggered: root.notebook.weighCells(root.notebook.pickedTable, root.reachFromRow, root.reachFromColumn, root.reachToRow, root.reachToColumn, !root.lookInHand.bold)
    }
    readonly property Action italicCells: Action {
        enabled: root.hasTable
        icon.source: Icons.italic
        text: qsTr("Slant Boxes")

        onTriggered: root.notebook.slantCells(root.notebook.pickedTable, root.reachFromRow, root.reachFromColumn, root.reachToRow, root.reachToColumn, !root.lookInHand.italic)
    }
    // Where the words of a box sit between its top and its foot.
    readonly property Action sitAtTop: Action {
        enabled: root.hasTable
        icon.source: Icons.sitAtTop
        text: qsTr("Sit at Top")

        onTriggered: root.raiseTheBoxesInHand(0)
    }
    readonly property Action sitAtMiddle: Action {
        enabled: root.hasTable
        icon.source: Icons.sitAtMiddle
        text: qsTr("Sit at Middle")

        onTriggered: root.raiseTheBoxesInHand(1)
    }
    readonly property Action sitAtFoot: Action {
        enabled: root.hasTable
        icon.source: Icons.sitAtFoot
        text: qsTr("Sit at Foot")

        onTriggered: root.raiseTheBoxesInHand(2)
    }
    readonly property Action clearCellLook: Action {
        enabled: root.hasTable
        text: qsTr("Clear Formatting")

        onTriggered: root.notebook.plainCells(root.notebook.pickedTable, root.reachFromRow, root.reachFromColumn, root.reachToRow, root.reachToColumn)
    }
    readonly property Action duplicateRow: Action {
        enabled: root.hasTable
        icon.source: Icons.duplicate
        text: qsTr("Duplicate Row")

        onTriggered: root.notebook.duplicateRow(root.notebook.pickedTable, root.reachFromRow)
    }
    readonly property Action duplicateColumn: Action {
        enabled: root.hasTable
        icon.source: Icons.duplicate
        text: qsTr("Duplicate Column")

        onTriggered: root.notebook.duplicateColumn(root.notebook.pickedTable, root.reachFromColumn)
    }
    readonly property Action removeColumn: Action {
        enabled: root.hasTable
        text: qsTr("Delete Column")

        onTriggered: root.notebook.removeColumn(root.notebook.pickedTable, root.columnInHand)
    }
    readonly property Action showLayersPanel: Action {
        checkable: true
        checked: root.settings.showLayers
        shortcut: root.settings.keysFor("showLayers")
        text: qsTr("Layers Panel")

        onTriggered: root.settings.showLayers = !root.settings.showLayers
    }
    readonly property Action addLayer: Action {
        enabled: root.hasNotebook
        icon.source: Icons.layerAdd
        text: qsTr("New Layer")

        onTriggered: root.notebook.addLayer()
    }
    readonly property Action duplicateLayer: Action {
        enabled: root.hasNotebook
        icon.source: Icons.layer
        text: qsTr("Duplicate Layer")

        onTriggered: root.notebook.duplicateLayer(root.notebook.activeLayer)
    }
    readonly property Action removeLayer: Action {
        enabled: root.hasNotebook
        icon.source: Icons.layerRemove
        text: qsTr("Delete Layer")

        onTriggered: root.notebook.removeLayer(root.notebook.activeLayer)
    }
    readonly property Action layerUp: Action {
        enabled: root.hasNotebook
        shortcut: root.pageKeys("layerUp")
        text: qsTr("Move Layer Up")

        onTriggered: root.moveLayerBy(1)
    }
    readonly property Action layerDown: Action {
        enabled: root.hasNotebook
        shortcut: root.pageKeys("layerDown")
        text: qsTr("Move Layer Down")

        onTriggered: root.moveLayerBy(-1)
    }
    readonly property Action layerToFront: Action {
        enabled: root.hasNotebook
        text: qsTr("Bring Layer to Front")

        onTriggered: root.moveLayerTo(root.notebook.layers.count - 1)
    }
    readonly property Action layerToBack: Action {
        enabled: root.hasNotebook
        text: qsTr("Send Layer to Back")

        onTriggered: root.moveLayerTo(0)
    }
    readonly property Action remove: Action {
        enabled: root.hasSelection || root.hasTextBox || root.hasPicture || root.hasTable
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
        text: qsTr("Document as New Pages…")

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
    signal pictureWanted
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

    // Where the layer in hand stands in the order the layers are drawn in, counted from the
    // bottom, or nothing at all where there is no notebook open.
    function placeOfLayerInHand() {
        if (root.notebook === null) {
            return -1;
        }
        const layers = root.notebook.layers;
        for (let step = 0; step < layers.count; ++step) {
            if (layers.data(layers.index(step, 0), Qt.UserRole + 1) === root.notebook.activeLayer) {
                // The panel lists them top first, so counting from the bottom turns it round.
                return layers.count - 1 - step;
            }
        }
        return -1;
    }

    function moveLayerBy(steps) {
        root.moveLayerTo(root.placeOfLayerInHand() + steps);
    }

    function moveLayerTo(place) {
        if (root.notebook !== null && root.placeOfLayerInHand() >= 0) {
            root.notebook.moveLayer(root.notebook.activeLayer, place);
        }
    }

    function alignTheBoxesInHand(align) {
        if (root.hasTable) {
            root.notebook.alignCells(root.notebook.pickedTable, root.reachFromRow, root.reachFromColumn, root.reachToRow, root.reachToColumn, align);
        }
    }

    function fillTheBoxesInHand(colour) {
        if (root.hasTable) {
            root.notebook.fillCells(root.notebook.pickedTable, root.reachFromRow, root.reachFromColumn, root.reachToRow, root.reachToColumn, colour);
        }
    }

    function inkTheBoxesInHand(colour) {
        if (root.hasTable) {
            root.notebook.inkCells(root.notebook.pickedTable, root.reachFromRow, root.reachFromColumn, root.reachToRow, root.reachToColumn, colour);
        }
    }

    function raiseTheBoxesInHand(rise) {
        if (root.hasTable) {
            root.notebook.riseCells(root.notebook.pickedTable, root.reachFromRow, root.reachFromColumn, root.reachToRow, root.reachToColumn, rise);
        }
    }

    // One key removes whatever is picked up, whether that is ink, a box of words, a picture or a
    // table. Where a stretch of a table is marked out, it is what is typed in those boxes that
    // goes rather than the table itself, which is what a reader marking them out is asking for.
    function deleteWhatIsPicked() {
        if (root.hasSelection) {
            root.notebook.deleteSelection();
        } else if (root.hasTextBox) {
            root.notebook.removeText(root.notebook.pickedText);
        } else if (root.hasPicture) {
            root.notebook.removePicture(root.notebook.pickedPicture);
        } else if (root.hasStretch) {
            root.notebook.emptyCells(root.notebook.pickedTable, root.reachFromRow, root.reachFromColumn, root.reachToRow, root.reachToColumn);
        } else if (root.hasTable) {
            root.notebook.removeTable(root.notebook.pickedTable);
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
        enabled: !root.typing && (root.hasSelection || root.hasTextBox || root.hasPicture || root.hasTable)
        sequences: ["Backspace"]

        onActivated: root.deleteWhatIsPicked()
    }
}
