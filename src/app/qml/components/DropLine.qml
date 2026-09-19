pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

// The line that marks the gap a row being carried would drop into.
Rectangle {
    id: root

    required property ListView list
    // 0 is above the first row, `count` is under the last one; -1 while nothing is carried.
    required property int place
    readonly property real step: root.list.count === 0 ? 0 : root.list.contentHeight / root.list.count

    color: Theme.accent
    height: 2
    radius: 1
    visible: root.place >= 0
    width: root.list.width
    x: 0
    y: Math.min(root.list.contentHeight - root.height, root.place * root.step)
    z: 3
}
