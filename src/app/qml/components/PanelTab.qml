pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

TabButton {
    id: root

    required property bool current
    required property string panelId
    readonly property string title: Panels.titleOf(root.panelId, Languages.spoken)
    readonly property bool carried: tabDrag.active

    // A tab picked up on its own, rather than the whole group it stands in.
    signal takenUp(point at)
    signal carriedTo(point at)
    signal putDown

    Accessible.name: root.title
    ToolTip.delay: 600
    ToolTip.text: root.title
    ToolTip.visible: root.hovered && label.truncated
    height: Theme.rowHeight
    implicitWidth: Math.min(140, Math.max(58, label.implicitWidth + leftPadding + rightPadding))
    leftPadding: Theme.gap
    objectName: "panelTab_" + root.panelId
    rightPadding: Theme.gap

    // A carried tab leaves the strip: what stays behind is the space it came out of, not a second
    // tab. The name following the pointer is the only one there is, so a tab carried a long way is
    // never in two places.
    background: Rectangle {
        color: root.carried ? "transparent" : root.current ? Theme.surface : root.hovered ? Theme.hover : "transparent"
        radius: 6

        Behavior on color {
            ColorAnimation {
                duration: Theme.quick
                easing.type: Theme.ease
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            color: Theme.accent
            height: 2
            radius: 1
            visible: root.current && !root.carried
            width: parent.width - (Theme.gap * 2)
        }
    }
    contentItem: Label {
        id: label

        color: root.current ? palette.windowText : palette.placeholderText
        elide: Text.ElideRight
        font.bold: root.current
        horizontalAlignment: Text.AlignHCenter
        opacity: root.carried ? 0 : 1
        text: root.title
        verticalAlignment: Text.AlignVCenter
    }

    DragHandler {
        id: tabDrag

        // Nothing may take this grab away, or the header would carry the whole group instead of
        // the one tab the hand is on.
        // A shorter threshold than the header's, so a hand on a tab carries the tab and not the
        // whole group, and nothing may take the grab away once it has it.
        dragThreshold: 4
        grabPermissions: PointerHandler.CanTakeOverFromAnything
        target: null

        onActiveChanged: {
            if (tabDrag.active) {
                root.takenUp(tabDrag.centroid.scenePosition);
                return;
            }
            root.putDown();
        }
        onCentroidChanged: {
            if (tabDrag.active) {
                root.carriedTo(tabDrag.centroid.scenePosition);
            }
        }
    }
}
