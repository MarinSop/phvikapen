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
    icon.height: Theme.glyph
    icon.width: Theme.glyph
    implicitHeight: Theme.quickTap
    implicitWidth: Theme.quickTap

    background: Rectangle {
        color: root.hovered ? Theme.base : "transparent"
        radius: 8

        Rectangle {
            anchors.centerIn: parent
            color: root.active ? Theme.accent : Theme.line
            height: Theme.quickTap - Theme.gap
            radius: 6
            width: Theme.quickTap - Theme.gap
        }
    }
}
