import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

ToolButton {
    id: root

    property string shortcutText: ""
    required property string label
    readonly property string tooltipText: root.shortcutText === "" ? root.label : root.label + "   " + root.shortcutText
    // A command with no glyph of its own says what it is in words rather than standing there as an
    // empty square, and is given the width those words need.
    readonly property bool glyphless: root.icon.source.toString() === ""

    ToolTip.delay: 600
    ToolTip.text: root.tooltipText
    ToolTip.visible: root.hovered
    display: root.glyphless ? AbstractButton.TextOnly : AbstractButton.IconOnly
    icon.color: root.enabled ? palette.buttonText : palette.placeholderText
    bottomPadding: 0
    icon.height: Theme.glyph
    icon.width: Theme.glyph
    implicitHeight: Theme.quickTap
    implicitWidth: root.glyphless ? Math.max(Theme.quickTap, root.implicitContentWidth + (2 * Theme.gap)) : Theme.quickTap
    leftPadding: root.glyphless ? Theme.gap : 0
    rightPadding: root.glyphless ? Theme.gap : 0
    topPadding: 0

    background: Rectangle {
        color: root.pressed ? Theme.accent : root.hovered ? Theme.line : "transparent"
        radius: 6
    }
}
