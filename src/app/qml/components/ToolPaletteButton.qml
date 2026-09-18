import QtQuick
import QtQuick.Controls

ToolButton {
    id: root

    property string shortcutText: ""
    required property string toolName
    readonly property string tooltipText: root.shortcutText === "" ? root.toolName : root.toolName + "   " + root.shortcutText

    ToolTip.delay: 600
    ToolTip.text: root.tooltipText
    ToolTip.visible: root.hovered
    checkable: true
    display: AbstractButton.IconOnly
    icon.color: root.enabled ? palette.buttonText : palette.placeholderText
    icon.height: 20
    icon.width: 20
    implicitHeight: 40
    implicitWidth: 40

    // The picked tool is marked by a bar as well as by colour.
    Rectangle {
        color: palette.highlight
        height: parent.height - 12
        radius: 1
        visible: root.checked
        width: 3
        x: 0
        y: 6
    }
}
