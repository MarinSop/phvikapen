import QtQuick
import QtQuick.Controls

// Half the height of a field, for the pair that steps a number up and down.
ToolButton {
    id: root

    display: AbstractButton.IconOnly
    icon.color: root.enabled ? palette.buttonText : palette.placeholderText
    icon.height: 14
    icon.width: 14
    bottomPadding: 0
    implicitHeight: 15
    implicitWidth: 22
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
}
