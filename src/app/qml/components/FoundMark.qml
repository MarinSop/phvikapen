pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

// The word a search was followed to, marked out on the page so that the eye finds it. The page has
// already moved the least it can to bring it into view; this only says which word it was. The mark
// touches nothing on the page and goes of its own accord.
Item {
    id: root

    required property InkCanvas canvas
    required property NotebookViewModel notebook
    readonly property var pointed: root.notebook === null ? ({}) : root.notebook.pointedWord
    readonly property bool pointing: root.pointed.columnLeft !== undefined
    readonly property point origin: root.canvas === null ? Qt.point(0, 0) : root.canvas.viewOrigin
    readonly property real zoom: root.canvas === null ? 1 : root.canvas.zoom
    // Long enough to be seen, short enough that it is not left lying over the page.
    readonly property int staysFor: 2600

    objectName: "foundMark"
    visible: root.pointing

    onPointingChanged: {
        if (root.pointing) {
            staying.restart();
        }
    }

    Timer {
        id: staying

        interval: root.staysFor
        repeat: false

        onTriggered: {
            if (root.notebook !== null) {
                root.notebook.forgetPointedWord();
            }
        }
    }

    Rectangle {
        border.color: Theme.accent
        border.width: 2
        color: Theme.accent
        height: root.pointing ? Math.max(4, (root.pointed.columnBottom - root.pointed.columnTop) * root.zoom) : 0
        objectName: "foundWordMark"
        opacity: 0.28
        radius: 3
        width: root.pointing ? Math.max(4, (root.pointed.columnRight - root.pointed.columnLeft) * root.zoom) : 0
        x: root.pointing ? (root.pointed.columnLeft - root.origin.x) * root.zoom : 0
        y: root.pointing ? (root.pointed.columnTop - root.origin.y) * root.zoom : 0

        SequentialAnimation on opacity {
            loops: 3
            running: root.pointing

            NumberAnimation {
                duration: Theme.calm
                from: 0.12
                to: 0.42
            }

            NumberAnimation {
                duration: Theme.calm
                from: 0.42
                to: 0.12
            }
        }
    }
}
