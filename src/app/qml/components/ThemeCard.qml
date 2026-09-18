import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

// A small picture of what the theme does to the window: the bars, the desk and a sheet on it.
AbstractButton {
    id: root

    required property int mode
    readonly property var colours: Theme.coloursOf(root.mode)

    ToolTip.delay: 600
    ToolTip.text: Theme.noteOf(root.mode)
    ToolTip.visible: root.hovered
    checkable: true
    implicitHeight: 112
    implicitWidth: 132
    padding: 6

    background: Rectangle {
        border.color: root.checked ? Theme.accent : Theme.line
        border.width: root.checked ? 2 : 1
        color: root.hovered && !root.checked ? Theme.hover : "transparent"
        radius: 10
    }
    contentItem: Column {
        spacing: 6

        Rectangle {
            id: preview

            border.color: Theme.line
            border.width: 1
            clip: true
            color: root.colours.window
            height: 64
            radius: 6
            width: parent.width

            Rectangle {
                color: root.colours.surfaceStrong
                height: 12
                width: parent.width
            }

            Rectangle {
                color: root.colours.surfaceStrong
                height: parent.height - 12
                width: 14
                y: 12
            }

            Rectangle {
                color: root.colours.desk
                height: parent.height - 12
                width: parent.width - 14
                x: 14
                y: 12

                Rectangle {
                    anchors.centerIn: parent
                    color: "#ffffff"
                    height: parent.height - 10
                    width: parent.width - 24

                    Rectangle {
                        color: root.colours.accent
                        height: 3
                        radius: 1.5
                        width: parent.width - 16
                        x: 8
                        y: 8
                    }

                    Rectangle {
                        color: root.colours.line
                        height: 2
                        width: parent.width - 24
                        x: 8
                        y: 18
                    }
                }
            }

            Rectangle {
                color: root.colours.accent
                height: 6
                radius: 3
                width: 6
                x: 4
                y: 20
            }
        }

        Label {
            color: Theme.text
            elide: Text.ElideRight
            font.bold: root.checked
            text: Theme.nameOf(root.mode)
            width: parent.width
        }
    }
}
