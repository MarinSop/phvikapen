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
    icon.height: 20
    icon.width: 20
    implicitHeight: 32
    implicitWidth: 32

    background: Rectangle {
        color: root.active ? Theme.accent : root.hovered ? Theme.line : "transparent"
        radius: 4
    }
}
