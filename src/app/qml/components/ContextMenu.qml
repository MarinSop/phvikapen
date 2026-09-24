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
    readonly property bool onPage: !root.actions.hasSelection && !root.actions.hasTextBox

    // Opened where the reader asked, and never off the edge of the window.
    function openAt(at) {
        const room = root.parent;
        const x = room === null ? at.x : Math.min(at.x, room.width - root.width);
        const y = room === null ? at.y : Math.min(at.y, room.height - root.height);
        root.popup(Math.max(0, x), Math.max(0, y));
    }

    objectName: "contextMenu"

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

    MenuSeparator {
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

    MenuSeparator {
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

    MenuSeparator {
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

    MenuSeparator {
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
        action: root.actions.remove
        objectName: "contextDelete"
        visible: root.onSelection || root.onText
    }
}
