pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

Item {
    id: root

    default property alias middle: middleHolder.data
    required property AppActions actions
    required property WorkspaceViewModel workspace
    property Item middleSlot: null
    readonly property var layout: root.workspace.layout
    readonly property bool dragging: drag.dragging

    function settleDrop() {
        const carried = drag.panelId;
        const kind = drag.kind;
        const path = drag.path;
        const edge = drag.edge;
        const at = drag.at2;
        drag.letGo();
        if (carried === "") {
            return;
        }
        if (kind === "close") {
            root.workspace.closePanel(carried);
        } else if (kind === "edge") {
            root.workspace.dropBeside(carried, path, edge);
        } else if (kind === "tab") {
            root.workspace.dropAsTab(carried, path, at);
        }
    }

    objectName: "workspaceView"

    PanelDragState {
        id: drag

        onDropped: root.settleDrop()
    }

    ToolPalette {
        id: tools

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.top: parent.top
        actions: root.actions
    }

    PanelArea {
        anchors.bottom: parent.bottom
        anchors.left: tools.right
        anchors.right: parent.right
        anchors.top: parent.top
        actions: root.actions
        drag: drag
        node: root.layout
        view: root
        workspace: root.workspace
    }

    Item {
        id: middleHolder

        height: root.middleSlot === null ? 0 : root.middleSlot.height
        parent: root.middleSlot
        width: root.middleSlot === null ? 0 : root.middleSlot.width
    }

    Rectangle {
        readonly property point corner: root.mapFromItem(null, drag.hint.x, drag.hint.y)

        color: Theme.accent
        height: drag.hint.height
        radius: 2
        visible: drag.dragging && drag.kind !== "" && drag.kind !== "close"
        width: drag.hint.width
        x: corner.x
        y: corner.y
        z: 50
    }

    PanelCloseTarget {
        anchors.horizontalCenter: parent.horizontalCenter
        drag: drag
        y: drag.dragging ? Theme.gap * 2 : -height
        z: 60

        Behavior on y {
            NumberAnimation {
                duration: Theme.calm
                easing.type: Theme.ease
            }
        }
    }

    Rectangle {
        id: ghost

        readonly property point here: root.mapFromItem(null, drag.at)

        border.color: Theme.accent
        border.width: 1
        color: Theme.surface
        height: Theme.rowHeight
        opacity: 0.92
        radius: 6
        visible: drag.dragging
        width: ghostName.implicitWidth + (Theme.gap * 4)
        x: Math.max(0, Math.min(root.width - width, ghost.here.x + Theme.gap))
        y: Math.max(0, Math.min(root.height - height, ghost.here.y + Theme.gap))
        z: 70

        Label {
            id: ghostName

            anchors.centerIn: parent
            color: palette.windowText
            text: drag.dragging ? Panels.titleOf(drag.panelId, Languages.spoken) : ""
        }
    }
}
