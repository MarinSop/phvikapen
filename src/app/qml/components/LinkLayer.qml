pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

// Every link on the page, drawn over the ink. A link answers a tap whatever tool is in hand, so a
// reader writing with the pen never has to put it down to follow one; a drag that starts on a link
// is still a stroke, because giving up on the drag is what a tap means.
Item {
    id: root

    required property InkCanvas canvas
    required property NotebookViewModel notebook
    readonly property point origin: root.canvas === null ? Qt.point(0, 0) : root.canvas.viewOrigin
    readonly property real zoom: root.canvas === null ? 1 : root.canvas.zoom
    readonly property var links: root.notebook === null ? [] : root.notebook.links

    signal followed(string linkId)

    anchors.fill: parent
    objectName: "linkLayer"

    Repeater {
        model: root.links

        Item {
            id: patch

            required property var modelData
            readonly property bool reachable: patch.modelData.reachable && patch.modelData.open

            Accessible.name: patch.modelData.label !== "" ? patch.modelData.label : patch.modelData.toPage ? qsTr("Goes to another page") : patch.modelData.where
            Accessible.role: Accessible.Link
            height: Math.max(1, patch.modelData.height * root.zoom)
            objectName: "link_" + patch.modelData.linkId
            visible: patch.modelData.open
            width: Math.max(1, patch.modelData.width * root.zoom)
            x: (patch.modelData.columnX - root.origin.x) * root.zoom
            y: (patch.modelData.columnY - root.origin.y) * root.zoom

            Rectangle {
                anchors.fill: parent
                border.color: Theme.accent
                border.width: 1
                color: over.hovered ? Theme.accentSoft : "transparent"
                opacity: over.hovered ? 1 : 0.45
                radius: 3

                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.quick
                        easing.type: Theme.ease
                    }
                }
            }

            HoverHandler {
                id: over

                cursorShape: patch.reachable ? Qt.PointingHandCursor : Qt.ForbiddenCursor
            }

            TapHandler {
                enabled: patch.reachable
                gesturePolicy: TapHandler.ReleaseWithinBounds
                objectName: "linkTap_" + patch.modelData.linkId

                onTapped: root.followed(patch.modelData.linkId)
            }
        }
    }
}
