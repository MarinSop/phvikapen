pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import PhvikaPen.Ui

Rectangle {
    id: root

    required property AppActions actions
    required property PanelDragState drag
    required property var panelIds
    required property string path
    required property WorkspaceViewModel workspace
    property int current: 0
    readonly property string currentPanel: root.current >= 0 && root.current < root.panelIds.length ? root.panelIds[root.current] : ""
    readonly property real edgeReach: Math.round(26 * Theme.scale)
    readonly property bool holdsOnlyTheDragged: root.panelIds.length === 1 && root.panelIds[0] === root.drag.panelId

    function reportAim() {
        if (!root.drag.dragging || root.holdsOnlyTheDragged) {
            return;
        }
        const local = root.mapFromItem(null, root.drag.at);
        if (local.x < 0 || local.y < 0 || local.x > root.width || local.y > root.height) {
            return;
        }
        const corner = root.mapToItem(null, 0, 0);
        if (local.y >= header.y && local.y <= header.y + header.height) {
            const at = root.tabAt(local.x - header.x - tabs.x);
            root.drag.report(root.drag.tabRank, "tab", root.path, -1, at, Qt.rect(corner.x + header.x + tabs.x + root.tabLine(at) - 1, corner.y + header.y, 2, header.height));
            return;
        }
        const left = local.x;
        const right = root.width - local.x;
        const top = local.y;
        const bottom = root.height - local.y;
        const nearest = Math.min(left, right, top, bottom);
        if (nearest > root.edgeReach) {
            root.drag.report(root.drag.intoRank, "edge", root.path, WorkspaceViewModel.Bottom, 0, Qt.rect(corner.x, corner.y + (root.height / 2) - 1, root.width, 3));
            return;
        }
        if (nearest === left) {
            root.drag.report(root.drag.edgeRank, "edge", root.path, WorkspaceViewModel.Left, 0, Qt.rect(corner.x, corner.y, 3, root.height));
        } else if (nearest === right) {
            root.drag.report(root.drag.edgeRank, "edge", root.path, WorkspaceViewModel.Right, 0, Qt.rect(corner.x + root.width - 3, corner.y, 3, root.height));
        } else if (nearest === top) {
            root.drag.report(root.drag.edgeRank, "edge", root.path, WorkspaceViewModel.Top, 0, Qt.rect(corner.x, corner.y, root.width, 3));
        } else {
            root.drag.report(root.drag.edgeRank, "edge", root.path, WorkspaceViewModel.Bottom, 0, Qt.rect(corner.x, corner.y + root.height - 3, root.width, 3));
        }
    }

    function tabAt(along) {
        let at = 0;
        for (let step = 0; step < tabs.count; ++step) {
            const tab = tabs.itemAtIndex(step);
            if (tab !== null && along > tab.x + (tab.width / 2)) {
                at = step + 1;
            }
        }
        return at;
    }

    function tabLine(at) {
        const last = Math.min(at, tabs.count - 1);
        const tab = tabs.itemAtIndex(last);
        if (tab === null) {
            return 0;
        }
        return at > tabs.count - 1 ? tab.x + tab.width : tab.x;
    }

    color: Theme.surface
    objectName: "panelFrame_" + root.path
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
        border.color: Theme.line
        border.width: 1
        color: "transparent"
        radius: parent.radius
        z: 4
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

                onActiveChanged: {
                    if (headerDrag.active) {
                        root.drag.take(root.currentPanel, headerDrag.centroid.scenePosition);
                        return;
                    }
                    root.drag.dropped();
                }
                onCentroidChanged: {
                    if (headerDrag.active) {
                        root.drag.at = headerDrag.centroid.scenePosition;
                    }
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
                            root.workspace.choosePanel(root.path, index);
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
}
