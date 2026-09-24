import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

ToolButton {
    id: root

    property bool active: false
    property string shortcutText: ""
    required property string toolName
    readonly property string tooltipText: root.shortcutText === "" ? root.toolName : root.toolName + "   " + root.shortcutText

    ToolTip.delay: 600
    ToolTip.text: root.tooltipText
    ToolTip.visible: root.hovered
    display: AbstractButton.IconOnly
    icon.color: !root.enabled ? palette.placeholderText : root.active ? Theme.text : palette.buttonText
    icon.height: Theme.glyph
    icon.width: Theme.glyph
    implicitHeight: Theme.tap
    implicitWidth: Theme.tap

    background: Rectangle {
        color: root.active ? Theme.base : root.hovered ? Theme.line : "transparent"
        radius: 6
    }

    // The picked tool is marked by a bar as well as by colour.
    Rectangle {
        height: parent.height - (Theme.gap * 2)
        radius: 1.5
        visible: root.active
        width: 3
        x: 0
        y: Theme.gap

        gradient: Gradient {
            GradientStop {
                color: Theme.mode === Theme.Brand ? Theme.brandStart : Theme.accent
                position: 0.0
            }

            GradientStop {
                color: Theme.mode === Theme.Brand ? Theme.brandEnd : Theme.accent
                position: 1.0
            }
        }
    }
}
