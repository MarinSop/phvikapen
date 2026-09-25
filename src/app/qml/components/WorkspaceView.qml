pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

Item {
    id: root

    default property alias middle: middleHolder.data
    required property AppActions actions
    required property WorkspaceViewModel workspace
    readonly property var docks: root.workspace.docks
    readonly property int edgeReach: Math.round(72 * Theme.scale)
    readonly property bool dragging: drag.dragging

    function groupsOn(side) {
        return root.docks[side].groups;
    }

    function extentOn(side) {
        return root.docks[side].extent;
    }

    function settleDrop() {
        const carried = drag.panelId;
        const kind = drag.kind;
        const side = drag.side;
        const group = drag.group;
        drag.letGo();
        if (carried === "") {
            return;
        }
        if (kind === "close") {
            root.workspace.closePanel(carried);
        } else if (kind === "edge") {
            root.workspace.dockPanel(carried, side, -1);
        } else if (kind === "stack") {
            root.workspace.dockPanel(carried, side, group);
        }
    }

    objectName: "workspaceView"

    PanelDragState {
        id: drag

        onDropped: root.settleDrop()
    }

    SplitView {
        id: across

        anchors.fill: parent
        orientation: Qt.Horizontal

        handle: Item {
            id: acrossHandle

            readonly property bool lit: acrossHandle.SplitHandle.pressed || acrossHandle.SplitHandle.hovered

            implicitWidth: 7

            Rectangle {
                anchors.centerIn: parent
                color: acrossHandle.lit ? Theme.accent : Theme.line
                height: parent.height
                width: acrossHandle.lit ? 3 : 1

                Behavior on color {
                    ColorAnimation {
                        duration: Theme.quick
                        easing.type: Theme.ease
                    }
                }
            }
        }

        ToolPalette {
            SplitView.maximumWidth: implicitWidth
            SplitView.minimumWidth: implicitWidth
            actions: root.actions
        }

        PanelDock {
            id: leftDock

            SplitView.maximumWidth: 640
            SplitView.minimumWidth: 140
            actions: root.actions
            drag: drag
            groups: root.groupsOn(WorkspaceViewModel.Left)
            side: WorkspaceViewModel.Left
            workspace: root.workspace

            Component.onCompleted: SplitView.preferredWidth = root.extentOn(WorkspaceViewModel.Left)
            onWidthChanged: {
                if (across.resizing) {
                    root.workspace.setSideExtent(WorkspaceViewModel.Left, leftDock.width);
                }
            }
        }

        SplitView {
            id: down

            SplitView.fillWidth: true
            orientation: Qt.Vertical

            handle: Item {
                id: downHandle

                readonly property bool lit: downHandle.SplitHandle.pressed || downHandle.SplitHandle.hovered

                implicitHeight: 7

                Rectangle {
                    anchors.centerIn: parent
                    color: downHandle.lit ? Theme.accent : Theme.line
                    height: downHandle.lit ? 3 : 1
                    width: parent.width

                    Behavior on color {
                        ColorAnimation {
                            duration: Theme.quick
                            easing.type: Theme.ease
                        }
                    }
                }
            }

            PanelDock {
                id: topDock

                SplitView.maximumHeight: 640
                SplitView.minimumHeight: 140
                actions: root.actions
                drag: drag
                groups: root.groupsOn(WorkspaceViewModel.Top)
                side: WorkspaceViewModel.Top
                workspace: root.workspace

                Component.onCompleted: SplitView.preferredHeight = root.extentOn(WorkspaceViewModel.Top)
                onHeightChanged: {
                    if (down.resizing) {
                        root.workspace.setSideExtent(WorkspaceViewModel.Top, topDock.height);
                    }
                }
            }

            Item {
                id: middleHolder

                SplitView.fillHeight: true
                SplitView.minimumHeight: 120
            }

            PanelDock {
                id: bottomDock

                SplitView.maximumHeight: 640
                SplitView.minimumHeight: 140
                actions: root.actions
                drag: drag
                groups: root.groupsOn(WorkspaceViewModel.Bottom)
                side: WorkspaceViewModel.Bottom
                workspace: root.workspace

                Component.onCompleted: SplitView.preferredHeight = root.extentOn(WorkspaceViewModel.Bottom)
                onHeightChanged: {
                    if (down.resizing) {
                        root.workspace.setSideExtent(WorkspaceViewModel.Bottom, bottomDock.height);
                    }
                }
            }
        }

        PanelDock {
            id: rightDock

            SplitView.maximumWidth: 640
            SplitView.minimumWidth: 140
            actions: root.actions
            drag: drag
            groups: root.groupsOn(WorkspaceViewModel.Right)
            side: WorkspaceViewModel.Right
            workspace: root.workspace

            Component.onCompleted: SplitView.preferredWidth = root.extentOn(WorkspaceViewModel.Right)
            onWidthChanged: {
                if (across.resizing) {
                    root.workspace.setSideExtent(WorkspaceViewModel.Right, rightDock.width);
                }
            }
        }
    }

    Item {
        anchors.fill: parent
        visible: drag.dragging
        z: 50

        PanelDropZone {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.margins: Theme.gap
            anchors.top: parent.top
            drag: drag
            label: qsTr("Left")
            side: WorkspaceViewModel.Left
            width: root.edgeReach
        }

        PanelDropZone {
            anchors.bottom: parent.bottom
            anchors.margins: Theme.gap
            anchors.right: parent.right
            anchors.top: parent.top
            drag: drag
            label: qsTr("Right")
            side: WorkspaceViewModel.Right
            width: root.edgeReach
        }

        PanelDropZone {
            anchors.left: parent.left
            anchors.leftMargin: root.edgeReach + (Theme.gap * 2)
            anchors.margins: Theme.gap
            anchors.right: parent.right
            anchors.rightMargin: root.edgeReach + (Theme.gap * 2)
            anchors.top: parent.top
            anchors.topMargin: Theme.tap + (Theme.gap * 5)
            drag: drag
            height: root.edgeReach
            label: qsTr("Top")
            side: WorkspaceViewModel.Top
        }

        PanelDropZone {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: root.edgeReach + (Theme.gap * 2)
            anchors.margins: Theme.gap
            anchors.right: parent.right
            anchors.rightMargin: root.edgeReach + (Theme.gap * 2)
            drag: drag
            height: root.edgeReach
            label: qsTr("Bottom")
            side: WorkspaceViewModel.Bottom
        }
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
