import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

MenuItem {
    id: root

    readonly property string shortcutText: root.action === null ? "" : AppInfo.shortcutText(root.action.shortcut)

    // A line that is not shown takes up no room, neither down the menu nor across it, so a menu is
    // as large as what it offers and no larger.
    height: root.visible ? root.implicitHeight : 0
    implicitHeight: Math.max(Theme.rowHeight, implicitContentHeight + topPadding + bottomPadding)
    implicitWidth: root.visible ? implicitContentWidth + leftPadding + rightPadding : 0
    indicator: null

    contentItem: Item {
        readonly property int tickRoom: root.checkable ? 18 : 0

        implicitHeight: label.implicitHeight
        implicitWidth: tickRoom + label.implicitWidth + (keys.visible ? Theme.rowHeight + keys.implicitWidth : 0)

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
            id: keys

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            color: palette.placeholderText
            text: root.shortcutText
            visible: root.shortcutText !== ""
        }
    }
}
