import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

// A colour to pick from the bar: the glyph of whatever it colours, with a band under it showing
// what is set. A band with nothing in it means nothing of its own has been asked for.
ToolButton {
    id: root

    required property color colour
    required property string label
    readonly property bool unset: root.colour.a === 0

    ToolTip.delay: 600
    ToolTip.text: root.label
    ToolTip.visible: root.hovered
    bottomPadding: 0
    display: AbstractButton.IconOnly
    icon.color: root.enabled ? palette.buttonText : palette.placeholderText
    icon.height: Theme.glyph
    icon.width: Theme.glyph
    implicitHeight: Theme.quickTap
    implicitWidth: Theme.quickTap
    leftPadding: 0
    rightPadding: 0
    topPadding: 0

    background: Rectangle {
        color: root.pressed ? Theme.accent : root.hovered ? Theme.line : "transparent"
        radius: 6

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 3
            anchors.horizontalCenter: parent.horizontalCenter
            border.color: Theme.line
            border.width: 1
            color: root.unset ? "transparent" : root.colour
            height: 4
            radius: 2
            width: parent.width - 10
        }
    }
}
