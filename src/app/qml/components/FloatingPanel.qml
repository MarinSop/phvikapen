pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

Item {
    id: root

    required property AppActions actions
    required property PanelDragState drag
    required property var place
    required property Item view
    required property WorkspaceViewModel workspace
    readonly property bool carried: root.drag.dragging && root.drag.homePath === root.place.path
    readonly property real snapReach: Math.round(18 * Theme.scale)
    property real heldWidth: root.place.width
    property real heldHeight: root.place.height

    function snapped(along, span, room) {
        if (along < root.snapReach) {
            return 0;
        }
        if (along + span > room - root.snapReach) {
            return Math.max(0, room - span);
        }
        return Math.max(0, Math.min(Math.max(0, room - span), along));
    }

    function followPointer() {
        if (!root.carried) {
            return;
        }
        const wanted = root.view.mapFromItem(null, root.drag.at);
        const from = root.view.mapFromItem(null, root.drag.startAt);
        root.x = root.snapped(root.place.x + wanted.x - from.x, root.width, root.view.width);
        root.y = root.snapped(root.place.y + wanted.y - from.y, root.height, root.view.height);
        root.drag.windowRect = Qt.rect(root.x, root.y, root.width, root.height);
    }

    function held() {
        root.workspace.sizePanelWindow(root.place.path, Math.round(root.heldWidth), Math.round(root.heldHeight));
    }

    height: root.heldHeight
    objectName: "panelWindow_" + root.place.path
    width: root.heldWidth
    x: Math.max(0, Math.min(Math.max(0, root.view.width - root.width), root.place.x))
    y: Math.max(0, Math.min(Math.max(0, root.view.height - root.height), root.place.y))

    Connections {
        function onAtChanged() {
            root.followPointer();
        }

        target: root.drag
    }

    Rectangle {
        anchors.fill: parent
        border.color: root.carried ? Theme.accent : Theme.line
        border.width: 1
        color: Theme.surface
        radius: 8
        z: -1
    }

    PanelArea {
        anchors.fill: parent
        actions: root.actions
        drag: root.drag
        node: root.place.node
        view: root.view
        workspace: root.workspace
    }

    Item {
        anchors.right: parent.right
        anchors.top: parent.top
        height: parent.height - corner.height
        width: 6

        HoverHandler {
            cursorShape: Qt.SizeHorCursor
        }

        DragHandler {
            target: null
            yAxis.enabled: false

            onActiveChanged: {
                if (!active) {
                    root.held();
                }
            }
            onTranslationChanged: {
                if (active) {
                    root.heldWidth = Math.max(root.workspace.leastExtent, root.place.width + translation.x);
                }
            }
        }
    }

    Item {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        height: 6
        width: parent.width - corner.width

        HoverHandler {
            cursorShape: Qt.SizeVerCursor
        }

        DragHandler {
            target: null
            xAxis.enabled: false

            onActiveChanged: {
                if (!active) {
                    root.held();
                }
            }
            onTranslationChanged: {
                if (active) {
                    root.heldHeight = Math.max(root.workspace.leastExtent, root.place.height + translation.y);
                }
            }
        }
    }

    Item {
        id: corner

        anchors.bottom: parent.bottom
        anchors.right: parent.right
        height: 14
        objectName: "panelWindowGrip_" + root.place.path
        width: 14

        HoverHandler {
            cursorShape: Qt.SizeFDiagCursor
        }

        DragHandler {
            target: null

            onActiveChanged: {
                if (!active) {
                    root.held();
                }
            }
            onTranslationChanged: {
                if (active) {
                    root.heldWidth = Math.max(root.workspace.leastExtent, root.place.width + translation.x);
                    root.heldHeight = Math.max(root.workspace.leastExtent, root.place.height + translation.y);
                }
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.margins: 3
            anchors.right: parent.right
            color: Theme.line
            height: 2
            radius: 1
            width: 8
        }
    }
}
