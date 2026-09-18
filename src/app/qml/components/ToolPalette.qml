pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property AppActions actions

    objectName: "toolPalette"
    padding: 4

    background: Rectangle {
        color: Theme.surfaceStrong

        Rectangle {
            anchors.right: parent.right
            color: Theme.line
            height: parent.height
            width: 1
        }
    }

    ColumnLayout {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        spacing: 2

        Repeater {
            model: root.actions.toolActions

            ToolPaletteButton {
                required property Action modelData

                action: modelData
                enabled: modelData.enabled
                objectName: modelData.text.toLowerCase() + "Tool"
                shortcutText: AppInfo.shortcutText(modelData.shortcut)
                toolName: modelData.text
            }
        }
    }
}
