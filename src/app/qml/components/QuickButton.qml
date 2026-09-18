import QtQuick
import QtQuick.Controls

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
    icon.height: 18
    icon.width: 18
    implicitHeight: 32
    implicitWidth: 32
}
