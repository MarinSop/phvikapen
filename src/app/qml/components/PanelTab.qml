pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

TabButton {
    id: root

    required property bool current
    required property string panelId
    readonly property string title: Panels.titleOf(root.panelId, Languages.spoken)

    Accessible.name: root.title
    ToolTip.delay: 600
    ToolTip.text: root.title
    ToolTip.visible: root.hovered && label.truncated
    height: Theme.rowHeight
    implicitWidth: Math.min(140, Math.max(58, label.implicitWidth + leftPadding + rightPadding))
    leftPadding: Theme.gap
    objectName: "panelTab_" + root.panelId
    rightPadding: Theme.gap

    background: Rectangle {
        color: root.current ? Theme.surface : root.hovered ? Theme.hover : "transparent"
        radius: 6

        Behavior on color {
            ColorAnimation {
                duration: Theme.quick
                easing.type: Theme.ease
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            color: Theme.accent
            height: 2
            radius: 1
            visible: root.current
            width: parent.width - (Theme.gap * 2)
        }
    }
    contentItem: Label {
        id: label

        color: root.current ? palette.windowText : palette.placeholderText
        elide: Text.ElideRight
        font.bold: root.current
        horizontalAlignment: Text.AlignHCenter
        text: root.title
        verticalAlignment: Text.AlignVCenter
    }
}
