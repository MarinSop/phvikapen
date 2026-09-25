pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

Rectangle {
    id: root

    required property PanelDragState drag
    readonly property bool aimedAt: root.drag.kind === "close"

    function pointInside(scenePosition) {
        const point = root.mapFromItem(null, scenePosition);
        return point.x >= 0 && point.y >= 0 && point.x <= root.width && point.y <= root.height;
    }

    function reportAim() {
        if (root.drag.dragging && root.pointInside(root.drag.at)) {
            root.drag.report(root.drag.closeRank, "close", "", -1, 0, Qt.rect(0, 0, 0, 0));
        }
    }

    border.color: root.aimedAt ? Theme.accent : Theme.line
    border.width: 1
    color: root.aimedAt ? Theme.accent : Theme.surface
    implicitHeight: Theme.tap
    implicitWidth: words.implicitWidth + (Theme.gap * 6)
    objectName: "panelCloseTarget"
    opacity: root.drag.dragging ? 1 : 0
    radius: height / 2
    scale: root.aimedAt ? 1.08 : root.drag.dragging ? 1 : 0.9
    visible: opacity > 0

    Behavior on color {
        ColorAnimation {
            duration: Theme.quick
            easing.type: Theme.ease
        }
    }
    Behavior on opacity {
        NumberAnimation {
            duration: Theme.calm
            easing.type: Theme.ease
        }
    }
    Behavior on scale {
        NumberAnimation {
            duration: Theme.calm
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
        id: words

        anchors.centerIn: parent
        color: root.aimedAt ? Theme.accentText : palette.windowText
        font.bold: root.aimedAt
        text: qsTr("Drop here to close")
    }
}
