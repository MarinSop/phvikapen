import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

// One of the shapes the shape tool can draw, marked when it is the one in hand.
ToolButton {
    id: root

    property bool active: false
    required property string label

    ToolTip.delay: 600
    ToolTip.text: root.label
    ToolTip.visible: root.hovered
    display: AbstractButton.IconOnly
    icon.color: root.active ? Theme.accentText : palette.buttonText
    icon.height: 24
    icon.width: 24
    implicitHeight: 36
    implicitWidth: 36

    background: Rectangle {
        color: root.hovered ? Theme.base : "transparent"
        radius: 8

        Rectangle {
            anchors.centerIn: parent
            color: root.active ? Theme.accent : Theme.line
            height: 26
            radius: 6
            width: 26
        }
    }
}
