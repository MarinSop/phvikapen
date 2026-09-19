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
        color: root.hovered ? Theme.base : "transparent"
        radius: 8
    }
    contentItem: Item {
        Rectangle {
            anchors.centerIn: parent
            color: Theme.line
            height: 24
            radius: 6
            width: 24

            Rectangle {
                anchors.centerIn: parent
                color: root.color
                // The thicker the pen, the thicker the mark it leaves here.
                height: Math.max(3, Math.min(18, root.penWidth * 1.6))
                radius: height / 2
                width: 16
            }
        }
    }
}
