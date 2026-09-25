pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

Item {
    id: root

    required property AppActions actions
    required property PanelDragState drag
    required property var node
    required property Item view
    required property WorkspaceViewModel workspace
    readonly property int kind: root.node.kind

    function holdsTheSheet(node) {
        if (node.kind === 0) {
            return true;
        }
        for (const child of node.children) {
            if (root.holdsTheSheet(child)) {
                return true;
            }
        }
        return false;
    }

    function whichOneStretches(children) {
        for (let step = 0; step < children.length; ++step) {
            if (root.holdsTheSheet(children[step])) {
                return step;
            }
        }
        return children.length - 1;
    }

    Loader {
        id: shown

        anchors.fill: parent
        sourceComponent: root.kind === 0 ? middleLeaf : root.kind === 1 ? stackLeaf : splitBranch
    }

    Component {
        id: middleLeaf

        Item {
            id: slot

            readonly property real edgeReach: Math.round(26 * Theme.scale)

            function reportAim() {
                if (!root.drag.dragging) {
                    return;
                }
                const local = slot.mapFromItem(null, root.drag.at);
                if (local.x < 0 || local.y < 0 || local.x > slot.width || local.y > slot.height) {
                    return;
                }
                const corner = slot.mapToItem(null, 0, 0);
                const left = local.x;
                const right = slot.width - local.x;
                const top = local.y;
                const bottom = slot.height - local.y;
                const nearest = Math.min(left, right, top, bottom);
                if (nearest === left) {
                    root.drag.report(root.drag.edgeRank, "edge", root.node.path, WorkspaceViewModel.Left, 0, Qt.rect(corner.x, corner.y, 3, slot.height));
                } else if (nearest === right) {
                    root.drag.report(root.drag.edgeRank, "edge", root.node.path, WorkspaceViewModel.Right, 0, Qt.rect(corner.x + slot.width - 3, corner.y, 3, slot.height));
                } else if (nearest === top) {
                    root.drag.report(root.drag.edgeRank, "edge", root.node.path, WorkspaceViewModel.Top, 0, Qt.rect(corner.x, corner.y, slot.width, 3));
                } else {
                    root.drag.report(root.drag.edgeRank, "edge", root.node.path, WorkspaceViewModel.Bottom, 0, Qt.rect(corner.x, corner.y + slot.height - 3, slot.width, 3));
                }
            }

            Component.onCompleted: root.view.middleSlot = slot

            Connections {
                function onAtChanged() {
                    slot.reportAim();
                }

                target: root.drag
            }
        }
    }

    Component {
        id: stackLeaf

        PanelFrame {
            actions: root.actions
            drag: root.drag
            panelIds: root.node.panels
            path: root.node.path
            workspace: root.workspace

            Component.onCompleted: current = root.node.current
        }
    }

    Component {
        id: splitBranch

        SplitView {
            id: split

            readonly property int stretching: root.whichOneStretches(root.node.children)

            orientation: root.node.across ? Qt.Horizontal : Qt.Vertical

            handle: Item {
                id: bar

                readonly property bool lit: bar.SplitHandle.pressed || bar.SplitHandle.hovered

                implicitHeight: split.orientation === Qt.Vertical ? 7 : 0
                implicitWidth: split.orientation === Qt.Horizontal ? 7 : 0

                Rectangle {
                    anchors.centerIn: parent
                    color: bar.lit ? Theme.accent : Theme.line
                    height: split.orientation === Qt.Vertical ? (bar.lit ? 3 : 1) : parent.height
                    width: split.orientation === Qt.Horizontal ? (bar.lit ? 3 : 1) : parent.width

                    Behavior on color {
                        ColorAnimation {
                            duration: Theme.quick
                            easing.type: Theme.ease
                        }
                    }
                }
            }

            Repeater {
                model: root.node.children

                // A branch of the tree holds another branch, and QML will not let a file name
                // itself. It is loaded by name instead, which is settled when it is needed.
                Loader {
                    id: branch

                    required property int index
                    required property var modelData

                    readonly property bool stretches: branch.index === split.stretching

                    SplitView.fillHeight: split.orientation === Qt.Vertical && branch.stretches
                    SplitView.fillWidth: split.orientation === Qt.Horizontal && branch.stretches
                    SplitView.minimumHeight: split.orientation === Qt.Vertical ? root.workspace.leastExtent : 0
                    SplitView.minimumWidth: split.orientation === Qt.Horizontal ? root.workspace.leastExtent : 0

                    Component.onCompleted: {
                        branch.setSource("PanelArea.qml", {
                            "actions": root.actions,
                            "drag": root.drag,
                            "node": branch.modelData,
                            "view": root.view,
                            "workspace": root.workspace
                        });
                        if (branch.stretches) {
                            return;
                        }
                        const kept = branch.modelData.extent > 0 ? branch.modelData.extent : root.workspace.leastExtent * 2;
                        if (split.orientation === Qt.Vertical) {
                            branch.SplitView.preferredHeight = kept;
                        } else {
                            branch.SplitView.preferredWidth = kept;
                        }
                    }
                    onHeightChanged: {
                        if (split.resizing && split.orientation === Qt.Vertical) {
                            root.workspace.setExtent(branch.modelData.path, branch.height);
                        }
                    }
                    onWidthChanged: {
                        if (split.resizing && split.orientation === Qt.Horizontal) {
                            root.workspace.setExtent(branch.modelData.path, branch.width);
                        }
                    }
                }
            }
        }
    }
}
