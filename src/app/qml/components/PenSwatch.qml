import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

AbstractButton {
    id: root

    property color color: "black"
    property real penWidth: 2

    implicitHeight: 34
    implicitWidth: 34

    background: Rectangle {
        border.color: root.checked ? Theme.accent : "transparent"
        border.width: 2
        color: root.checked || root.hovered ? Theme.line : "transparent"
        radius: 6
    }
    contentItem: Item {
        Rectangle {
            anchors.centerIn: parent
            color: root.color
            // The thicker the pen, the thicker the mark it leaves here.
            height: Math.max(4, Math.min(20, root.penWidth * 1.6))
            radius: height / 2
            width: 22
        }
    }
}
