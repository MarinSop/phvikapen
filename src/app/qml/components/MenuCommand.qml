import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

MenuItem {
    id: root

    readonly property string shortcutText: root.action === null ? "" : AppInfo.shortcutText(root.action.shortcut)

    implicitWidth: Math.max(240, implicitContentWidth + leftPadding + rightPadding)

    contentItem: Item {
        implicitHeight: label.implicitHeight

        Label {
            id: label

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            color: root.enabled ? palette.windowText : palette.placeholderText
            text: root.text
        }

        Label {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            color: palette.placeholderText
            text: root.shortcutText
            visible: root.shortcutText !== ""
        }
    }
}
