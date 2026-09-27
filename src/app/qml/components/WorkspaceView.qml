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
    readonly property rect dropHint: drag.hint
    readonly property string dropKind: drag.kind

    function windowSpan() {
        return Qt.size(Math.round(Math.max(220, Math.min(480, drag.homeWidth))), Math.round(Math.max(200, Math.min(560, drag.homeHeight))));
    }

    function sheetRect() {
        if (root.middleSlot === null) {
            return Qt.rect(0, 0, root.width, root.height);
        }
        const at = root.mapFromItem(root.middleSlot, 0, 0);
        return Qt.rect(at.x, at.y, root.middleSlot.width, root.middleSlot.height);
    }

    function cornerRect(which) {
        const span = root.windowSpan();
        const sheet = root.sheetRect();
        const left = which % 2 === 0;
        const top = which < 2;
        return Qt.rect(Math.round(left ? sheet.x : sheet.x + sheet.width - span.width), Math.round(top ? sheet.y : sheet.y + sheet.height - span.height), span.width, span.height);
    }

    function looseRect() {
        if (drag.windowRect.width > 0) {
            return drag.windowRect;
        }
        const span = root.windowSpan();
        const here = root.mapFromItem(null, drag.at);
        return Qt.rect(Math.round(Math.max(0, Math.min(root.width - span.width, here.x - (span.width / 2)))), Math.round(Math.max(0, Math.min(root.height - span.height, here.y - (Theme.rowHeight / 2)))), span.width, span.height);
    }

    function letPanelFloat(panelId, scenePosition, wide, tall) {
        const width = Math.round(Math.max(220, Math.min(480, wide)));
        const height = Math.round(Math.max(200, Math.min(560, tall)));
        const at = root.mapFromItem(null, scenePosition);
        root.workspace.floatPanel(panelId, Math.round(Math.max(0, Math.min(root.width - width, at.x - (width / 2)))), Math.round(Math.max(0, Math.min(root.height - height, at.y - (height / 2)))), width, height);
    }

    function settleDrop() {
        const carried = drag.panelId;
        const kind = drag.kind;
        const path = drag.path;
        const edge = drag.edge;
        const at = drag.at2;
        const loose = kind === "corner" ? root.cornerRect(at) : root.looseRect();
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
        } else {
            root.workspace.floatPanel(carried, loose.x, loose.y, loose.width, loose.height);
        }
    }

    objectName: "workspaceView"

    PanelDragState {
        id: drag

        onDropped: root.settleDrop()
        onLooseWanted: (panelId, at, wide, tall) => root.letPanelFloat(panelId, at, wide, tall)
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

    Repeater {
        model: root.workspace.floating

        FloatingPanel {
            required property var modelData

            actions: root.actions
            drag: drag
            place: modelData
            view: root
            workspace: root.workspace
            z: 40
        }
    }

    Repeater {
        model: root.workspace.looseWindows

        Item {
            id: freeHolder

            required property var modelData
            readonly property var own: ownWindow

            objectName: "loosePanelHolder_" + freeHolder.modelData.path

            PanelWindow {
                id: ownWindow

                actions: root.actions
                drag: drag
                place: freeHolder.modelData
                workspace: root.workspace
            }
        }
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

    Rectangle {
        readonly property point corner: root.mapFromItem(null, drag.hint2.x, drag.hint2.y)

        color: Theme.accent
        height: drag.hint2.height
        radius: 2
        visible: drag.dragging && drag.hint2.width > 0
        width: drag.hint2.width
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
        visible: drag.dragging && drag.windowRect.width <= 0
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
