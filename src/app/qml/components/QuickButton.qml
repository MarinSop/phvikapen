import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

ToolButton {
    id: root

    property string shortcutText: ""
    required property string label
    readonly property string tooltipText: root.shortcutText === "" ? root.label : root.label + "   " + root.shortcutText

    ToolTip.delay: 600
    ToolTip.text: root.tooltipText
    ToolTip.visible: root.hovered
    display: AbstractButton.IconOnly
    icon.color: root.enabled ? palette.buttonText : palette.placeholderText
    bottomPadding: 0
    icon.height: 24
    icon.width: 24
    implicitHeight: 34
    implicitWidth: 34
    leftPadding: 0
    rightPadding: 0
    topPadding: 0

    background: Rectangle {
        color: root.pressed ? Theme.accent : root.hovered ? Theme.line : "transparent"
        radius: 6
    }
}
