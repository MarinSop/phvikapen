import QtQuick
import QtQuick.Controls

AbstractButton {
    id: root

    property color color: "black"
    property real penWidth: 2

    implicitHeight: 32
    implicitWidth: 32

    background: Rectangle {
        border.color: root.checked ? root.palette.highlight : "transparent"
        border.width: 2
        color: root.hovered ? Qt.rgba(0.5, 0.5, 0.5, 0.15) : "transparent"
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
