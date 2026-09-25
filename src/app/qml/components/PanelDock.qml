pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

SplitView {
    id: root

    required property AppActions actions
    required property PanelDragState drag
    required property var groups
    required property int side
    required property WorkspaceViewModel workspace
    readonly property bool standing: root.groups.length > 0

    objectName: "panelDock" + root.side
    orientation: root.side === WorkspaceViewModel.Left || root.side === WorkspaceViewModel.Right ? Qt.Vertical : Qt.Horizontal
    visible: root.standing

    handle: Item {
        id: dockHandle

        readonly property bool lit: dockHandle.SplitHandle.pressed || dockHandle.SplitHandle.hovered

        implicitHeight: root.orientation === Qt.Vertical ? 7 : 0
        implicitWidth: root.orientation === Qt.Horizontal ? 7 : 0

        Rectangle {
            anchors.centerIn: parent
            color: dockHandle.lit ? Theme.accent : Theme.line
            height: root.orientation === Qt.Vertical ? (dockHandle.lit ? 3 : 1) : parent.height
            width: root.orientation === Qt.Horizontal ? (dockHandle.lit ? 3 : 1) : parent.width

            Behavior on color {
                ColorAnimation {
                    duration: Theme.quick
                    easing.type: Theme.ease
                }
            }
        }
    }

    Repeater {
        model: root.groups

        PanelFrame {
            id: frame

            required property int index
            required property var modelData

            SplitView.fillHeight: root.orientation === Qt.Vertical && frame.index === root.groups.length - 1
            SplitView.fillWidth: root.orientation === Qt.Horizontal && frame.index === root.groups.length - 1
            SplitView.minimumHeight: root.orientation === Qt.Vertical ? root.workspace.leastGroupExtent : 0
            SplitView.minimumWidth: root.orientation === Qt.Horizontal ? root.workspace.leastGroupExtent : 0
            actions: root.actions
            drag: root.drag
            group: frame.index
            panelIds: frame.modelData.panels
            side: root.side
            workspace: root.workspace

            Component.onCompleted: {
                frame.current = frame.modelData.current;
                const kept = frame.modelData.extent;
                if (kept > 0) {
                    if (root.orientation === Qt.Vertical) {
                        frame.SplitView.preferredHeight = kept;
                    } else {
                        frame.SplitView.preferredWidth = kept;
                    }
                }
            }
            onHeightChanged: {
                if (root.resizing && root.orientation === Qt.Vertical) {
                    root.workspace.setGroupExtent(root.side, frame.index, frame.height);
                }
            }
            onWidthChanged: {
                if (root.resizing && root.orientation === Qt.Horizontal) {
                    root.workspace.setGroupExtent(root.side, frame.index, frame.width);
                }
            }
        }
    }
}
