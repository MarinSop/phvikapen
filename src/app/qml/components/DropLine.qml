pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

// The line that shows where a row being carried would land.
Rectangle {
    id: root

    required property ListView list
    required property int place

    readonly property Item row: root.place >= 0 ? root.list.itemAtIndex(root.place) : null

    color: Theme.accent
    height: 2
    radius: 1
    visible: root.row !== null
    width: root.list.width
    x: 0
    y: root.row === null ? 0 : root.row.y + root.row.height - 1
    z: 3
}
