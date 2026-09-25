pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

Rectangle {
    id: root

    required property PanelDragState drag
    required property string label
    required property int side
    readonly property bool aimedAt: root.drag.kind === "edge" && root.drag.side === root.side

    function pointInside(scenePosition) {
        const point = root.mapFromItem(null, scenePosition);
        return point.x >= 0 && point.y >= 0 && point.x <= root.width && point.y <= root.height;
    }

    function reportAim() {
        root.drag.report(root.drag.edgeRank, root.drag.dragging && root.pointInside(root.drag.at) ? "edge" : "", root.side, -1);
    }

    border.color: Theme.accent
    border.width: root.aimedAt ? 2 : 1
    color: root.aimedAt ? Theme.accentSoft : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.08)
    objectName: "panelDropZone" + root.side
    radius: 8
    visible: root.drag.dragging

    Behavior on color {
        ColorAnimation {
            duration: Theme.quick
            easing.type: Theme.ease
        }
    }

    Connections {
        function onAtChanged() {
            root.reportAim();
        }

        target: root.drag
    }

    Label {
        anchors.centerIn: parent
        color: palette.windowText
        font.bold: root.aimedAt
        opacity: root.aimedAt ? 1 : 0.65
        text: root.label
    }
}
