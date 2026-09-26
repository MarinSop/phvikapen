pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

// What can be done right here, opened under the pointer. Every line is one of the application's
// commands, the same objects the menus and the keyboard reach, so nothing can drift apart.
Menu {
    id: root

    required property AppActions actions
    readonly property bool onSelection: root.actions.hasSelection
    readonly property bool onText: !root.actions.hasSelection && root.actions.hasTextBox
    readonly property bool onTable: !root.actions.hasSelection && !root.actions.hasTextBox && root.actions.hasTable
    readonly property bool onPage: !root.actions.hasSelection && !root.actions.hasTextBox && !root.actions.hasTable

    // Opened where the reader asked, and never off the edge of the window.
    function openAt(at) {
        const room = root.parent;
        const x = room === null ? at.x : Math.min(at.x, room.width - root.width);
        const y = room === null ? at.y : Math.min(at.y, room.height - root.height);
        root.popup(Math.max(0, x), Math.max(0, y));
    }

    objectName: "contextMenu"

    // The view the lines stand in leaves no room between them. A line that does not apply is not
    // shown and takes up no height, but the room between one line and the next was still kept for
    // it, which is what left a band of nothing under the last command the menu was offering.
    contentItem: ListView {
        clip: true
        currentIndex: root.currentIndex
        implicitHeight: contentHeight
        interactive: Window.window !== null && contentHeight > Window.window.height
        keyNavigationEnabled: true
        keyNavigationWraps: true
        model: root.contentModel
        spacing: 0

        ScrollIndicator.vertical: ScrollIndicator {
        }
    }

    MenuCommand {
        action: root.actions.cut
        objectName: "contextCut"
        visible: root.onSelection
    }

    MenuCommand {
        action: root.actions.copy
        visible: root.onSelection
    }

    MenuCommand {
        action: root.actions.duplicate
        objectName: "contextDuplicate"
        visible: root.onSelection
    }

    MenuLine {
        visible: root.onSelection
    }

    MenuCommand {
        action: root.actions.convertToText
        objectName: "contextConvertToText"
        visible: root.onSelection
    }

    MenuCommand {
        action: root.actions.copyAsText
        visible: root.onSelection
    }

    MenuCommand {
        action: root.actions.solve
        objectName: "contextSolve"
        visible: root.onSelection
    }

    MenuLine {
        visible: root.onSelection
    }

    MenuCommand {
        action: root.actions.rotateLeft
        objectName: "contextRotateLeft"
        visible: root.onSelection
    }

    MenuCommand {
        action: root.actions.rotateRight
        visible: root.onSelection
    }

    MenuCommand {
        action: root.actions.paste
        visible: root.onPage
    }

    MenuCommand {
        action: root.actions.selectAll
        objectName: "contextSelectAll"
        visible: root.onPage
    }

    MenuLine {
        visible: root.onPage
    }

    MenuCommand {
        action: root.actions.undo
        visible: root.onPage
    }

    MenuCommand {
        action: root.actions.redo
        visible: root.onPage
    }

    MenuLine {
        visible: root.onPage
    }

    MenuCommand {
        action: root.actions.textTool
        visible: root.onPage
    }

    MenuCommand {
        action: root.actions.addPage
        visible: root.onPage
    }

    MenuCommand {
        action: root.actions.clearPage
        visible: root.onPage
    }

    MenuCommand {
        action: root.actions.mergeCells
        objectName: "contextMergeCells"
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.splitCell
        objectName: "contextSplitCell"
        visible: root.onTable
    }

    MenuLine {
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.boldCells
        objectName: "contextBoldCells"
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.italicCells
        visible: root.onTable
    }

    MenuLine {
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.alignCellLeft
        objectName: "contextAlignCellLeft"
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.alignCellCentre
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.alignCellRight
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.sitAtTop
        objectName: "contextSitAtTop"
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.sitAtMiddle
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.sitAtFoot
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.clearCellLook
        objectName: "contextClearCellLook"
        visible: root.onTable
    }

    MenuLine {
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.addRowAbove
        objectName: "contextAddRowAbove"
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.addRowBelow
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.addColumnBefore
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.addColumnAfter
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.duplicateRow
        objectName: "contextDuplicateRow"
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.duplicateColumn
        visible: root.onTable
    }

    MenuLine {
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.removeRow
        objectName: "contextRemoveRow"
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.removeColumn
        visible: root.onTable
    }

    MenuLine {
        visible: root.onTable
    }

    MenuCommand {
        action: root.actions.remove
        objectName: "contextDelete"
        visible: root.onSelection || root.onText || root.onTable
    }

    MenuLine {
    }

    MenuCommand {
        action: root.actions.readPicture
        objectName: "contextReadPicture"
        visible: root.actions.hasPicture
    }

    MenuCommand {
        action: root.actions.keepAsElement
        objectName: "contextKeepAsElement"
        visible: root.actions.keepAsElement.enabled
    }

    MenuCommand {
        action: root.actions.addLink
        objectName: "contextAddLink"
        visible: root.actions.linkInHand === ""
    }

    MenuCommand {
        action: root.actions.changeLink
        objectName: "contextChangeLink"
        visible: root.actions.linkInHand !== ""
    }

    MenuCommand {
        action: root.actions.removeLink
        objectName: "contextRemoveLink"
        visible: root.actions.linkInHand !== ""
    }

    MenuLine {
        visible: root.actions.playFromHere.enabled
    }

    MenuCommand {
        action: root.actions.playFromHere
        objectName: "contextPlayFromHere"
        visible: root.actions.playFromHere.enabled
    }
}
