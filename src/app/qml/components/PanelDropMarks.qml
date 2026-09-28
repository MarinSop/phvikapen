pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

// The lines that say where a carried panel would land, and the name that follows the hand. Where the
// drag is reckoned is the screens, so every window draws the same marks in its own coordinates and a
// panel carried from one window to another is followed all the way across.
Item {
    id: root

    required property PanelDragState drag

    objectName: "panelDropMarks"

    Rectangle {
        readonly property point corner: root.mapFromGlobal(root.drag.hint.x, root.drag.hint.y)

        color: Theme.accent
        height: root.drag.hint.height
        radius: 2
        visible: root.drag.dragging && root.drag.kind !== "" && root.drag.kind !== "close"
        width: root.drag.hint.width
        x: corner.x
        y: corner.y
        z: 50
    }

    Rectangle {
        readonly property point corner: root.mapFromGlobal(root.drag.hint2.x, root.drag.hint2.y)

        color: Theme.accent
        height: root.drag.hint2.height
        radius: 2
        visible: root.drag.dragging && root.drag.hint2.width > 0
        width: root.drag.hint2.width
        x: corner.x
        y: corner.y
        z: 50
    }

    Rectangle {
        id: ghost

        readonly property point here: root.mapFromGlobal(root.drag.at.x, root.drag.at.y)

        border.color: Theme.accent
        border.width: 1
        color: Theme.surface
        height: Theme.rowHeight
        objectName: "panelGhost"
        opacity: 0.92
        radius: 6
        visible: root.drag.dragging && root.drag.windowRect.width <= 0
        width: ghostName.implicitWidth + (Theme.gap * 4)
        x: Math.max(0, Math.min(root.width - width, ghost.here.x + Theme.gap))
        y: Math.max(0, Math.min(root.height - height, ghost.here.y + Theme.gap))
        z: 70

        Label {
            id: ghostName

            anchors.centerIn: parent
            color: palette.windowText
            text: root.drag.dragging ? Panels.titleOf(root.drag.panelId, Languages.spoken) : ""
        }
    }
}
