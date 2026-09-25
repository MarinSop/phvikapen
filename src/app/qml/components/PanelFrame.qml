pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Rectangle {
    id: root

    required property AppActions actions
    required property PanelDragState drag
    required property int group
    required property var panelIds
    required property int side
    required property WorkspaceViewModel workspace
    property int current: 0
    readonly property string currentPanel: root.current >= 0 && root.current < root.panelIds.length ? root.panelIds[root.current] : ""
    readonly property bool aimedAt: root.drag.dragging && root.drag.kind === "stack" && root.drag.side === root.side && root.drag.group === root.group
    readonly property bool holdsTheDragged: root.panelIds.indexOf(root.drag.panelId) >= 0

    function pointInside(scenePosition) {
        const point = root.mapFromItem(null, scenePosition);
        return point.x >= 0 && point.y >= 0 && point.x <= root.width && point.y <= root.height;
    }

    function reportAim() {
        const worthIt = root.drag.dragging && !(root.holdsTheDragged && root.panelIds.length === 1);
        root.drag.report(root.drag.stackRank, worthIt && root.pointInside(root.drag.at) ? "stack" : "", root.side, root.group);
    }

    color: Theme.surface
    objectName: "panelFrame_" + root.side + "_" + root.group
    radius: 8

    onCurrentChanged: tabs.positionViewAtIndex(root.current, ListView.Contain)

    Connections {
        function onAtChanged() {
            root.reportAim();
        }

        target: root.drag
    }

    Connections {
        function onPanelShown(panelId) {
            const at = root.panelIds.indexOf(panelId);
            if (at >= 0) {
                root.current = at;
            }
        }

        target: root.workspace
    }

    Rectangle {
        anchors.fill: parent
        border.color: root.aimedAt ? Theme.accent : Theme.line
        border.width: root.aimedAt ? 2 : 1
        color: "transparent"
        radius: parent.radius
        z: 4

        Behavior on border.color {
            ColorAnimation {
                duration: Theme.quick
                easing.type: Theme.ease
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        Rectangle {
            id: header

            Layout.fillWidth: true
            Layout.preferredHeight: Theme.rowHeight + (Theme.gap * 2)
            color: headerHover.hovered || headerDrag.active ? Theme.hover : "transparent"
            radius: root.radius

            Behavior on color {
                ColorAnimation {
                    duration: Theme.quick
                    easing.type: Theme.ease
                }
            }

            HoverHandler {
                id: headerHover

                cursorShape: headerDrag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
            }

            DragHandler {
                id: headerDrag

                dragThreshold: 8
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                target: null

                onCentroidChanged: {
                    if (headerDrag.active) {
                        root.drag.at = headerDrag.centroid.scenePosition;
                    }
                }
                onActiveChanged: {
                    if (headerDrag.active) {
                        root.drag.take(root.currentPanel, headerDrag.centroid.scenePosition);
                        return;
                    }
                    root.drag.dropped();
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.gap
                anchors.rightMargin: Theme.gap
                spacing: Theme.gap

                ListView {
                    id: tabs

                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.rowHeight
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true
                    interactive: contentWidth > width
                    model: root.panelIds
                    objectName: "panelTabs"
                    orientation: ListView.Horizontal
                    spacing: 2

                    delegate: PanelTab {
                        required property int index
                        required property string modelData

                        current: index === root.current
                        panelId: modelData

                        onClicked: {
                            root.current = index;
                            root.workspace.choosePanel(root.side, root.group, index);
                        }
                    }

                    Component.onCompleted: tabs.positionViewAtIndex(root.current, ListView.Contain)
                }

                ShapeButton {
                    icon.source: Icons.close
                    label: qsTr("Close this panel")
                    objectName: "closePanelButton"

                    onClicked: root.workspace.closePanel(root.currentPanel)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.line
        }

        StackLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            currentIndex: root.current

            Repeater {
                model: root.panelIds

                PanelBody {
                    required property string modelData

                    actions: root.actions
                    panelId: modelData
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.accentSoft
        opacity: root.aimedAt ? 1 : 0
        radius: parent.radius
        visible: opacity > 0
        z: 3

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.quick
                easing.type: Theme.ease
            }
        }

        Label {
            anchors.centerIn: parent
            color: palette.windowText
            font.bold: true
            text: qsTr("Add as a tab")
        }
    }
}
