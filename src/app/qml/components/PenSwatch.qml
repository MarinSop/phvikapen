import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

AbstractButton {
    id: root

    property color color: "black"
    property real penWidth: 2

    implicitHeight: 32
    implicitWidth: 32

    background: Rectangle {
        color: root.checked ? Theme.accent : root.hovered ? Theme.line : "transparent"
        radius: 6
    }
    contentItem: Item {
        Rectangle {
            anchors.centerIn: parent
            color: root.color
            height: Math.max(3, Math.min(14, root.penWidth * 1.5))
            radius: height / 2
            width: 18
        }
    }
}
