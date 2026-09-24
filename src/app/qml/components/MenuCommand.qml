import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

MenuItem {
    id: root

    readonly property string shortcutText: root.action === null ? "" : AppInfo.shortcutText(root.action.shortcut)

    implicitHeight: Math.max(Theme.rowHeight, implicitContentHeight + topPadding + bottomPadding)
    implicitWidth: Math.max(Theme.menuWidth, implicitContentWidth + leftPadding + rightPadding)
    indicator: null

    contentItem: Item {
        implicitHeight: label.implicitHeight

        Label {
            id: tick

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            color: palette.windowText
            text: "✓"
            visible: root.checkable && root.checked
            width: 18
        }

        Label {
            id: label

            anchors.left: parent.left
            anchors.leftMargin: root.checkable ? 18 : 0
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
