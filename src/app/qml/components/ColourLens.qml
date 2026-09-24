pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

// The glass that follows the color picker. It shows the page many times over around the point
// that would be taken, with a cross on the very spot, so that a color can be picked exactly.
Item {
    id: root

    required property InkCanvas canvas
    required property ToolViewModel tools
    // How many times over the page is shown inside the glass.
    property real reach: 6
    readonly property int bubbleSize: Math.round(132 * Theme.scale)
    readonly property bool picking: root.tools !== null && root.tools.currentTool === ToolViewModel.ColourPicker
    readonly property bool shown: root.picking && root.canvas !== null && root.canvas.enabled && root.canvas.pointerInside
    readonly property point at: root.canvas === null ? Qt.point(0, 0) : root.mapFromItem(root.canvas, root.canvas.pointerAt.x, root.canvas.pointerAt.y)
    readonly property real window: root.bubbleSize / root.reach

    objectName: "colourLens"
    visible: root.shown

    Item {
        id: bubble

        height: root.bubbleSize + footer.height
        objectName: "colourLensBubble"
        width: root.bubbleSize
        // Up and to the left of the pen, so the hand holding it does not cover the glass, and
        // never over the edge of the window.
        x: Math.max(0, Math.min(root.width - width, root.at.x - width - Theme.gap))
        y: Math.max(0, Math.min(root.height - height, root.at.y - height - Theme.gap))

        Rectangle {
            anchors.fill: parent
            border.color: Theme.line
            border.width: 1
            color: Theme.surface
            radius: 10
        }

        Item {
            id: glassRoom

            clip: true
            height: root.bubbleSize
            width: root.bubbleSize

            ShaderEffectSource {
                id: glass

                anchors.centerIn: parent
                height: root.bubbleSize - 8
                live: true
                // Nearest neighbour, so that the pixel that would be taken is a square with an
                // edge rather than a blur.
                smooth: false
                sourceItem: root.canvas
                sourceRect: root.canvas === null ? Qt.rect(0, 0, 1, 1) : Qt.rect(root.canvas.pointerAt.x - (root.window / 2), root.canvas.pointerAt.y - (root.window / 2), root.window, root.window)
                width: root.bubbleSize - 8
            }

            Rectangle {
                anchors.centerIn: glass
                color: "transparent"
                border.color: Theme.accent
                border.width: 1
                height: Math.max(3, root.reach)
                width: Math.max(3, root.reach)
            }

            Rectangle {
                anchors.horizontalCenter: glass.horizontalCenter
                anchors.top: glass.top
                color: Theme.accent
                height: (glass.height / 2) - Math.max(3, root.reach)
                opacity: 0.55
                width: 1
            }

            Rectangle {
                anchors.bottom: glass.bottom
                anchors.horizontalCenter: glass.horizontalCenter
                color: Theme.accent
                height: (glass.height / 2) - Math.max(3, root.reach)
                opacity: 0.55
                width: 1
            }

            Rectangle {
                anchors.left: glass.left
                anchors.verticalCenter: glass.verticalCenter
                color: Theme.accent
                height: 1
                opacity: 0.55
                width: (glass.width / 2) - Math.max(3, root.reach)
            }

            Rectangle {
                anchors.right: glass.right
                anchors.verticalCenter: glass.verticalCenter
                color: Theme.accent
                height: 1
                opacity: 0.55
                width: (glass.width / 2) - Math.max(3, root.reach)
            }
        }

        Row {
            id: footer

            anchors.top: glassRoom.bottom
            height: Theme.rowHeight
            leftPadding: Theme.gap
            spacing: Theme.gap

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                border.color: Theme.line
                border.width: 1
                color: root.tools === null ? "transparent" : root.tools.strokeColor
                height: Theme.grip
                radius: 4
                width: Theme.grip
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                color: Theme.subtleText
                font.pixelSize: 12
                objectName: "colourLensName"
                text: root.tools === null ? "" : root.tools.strokeColor.toString()
            }
        }
    }
}
