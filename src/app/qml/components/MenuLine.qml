import QtQuick.Controls

// A line drawn between groups of commands. One that is not shown takes up no room, so a menu whose
// groups come and go is never left with a gap where they were.
MenuSeparator {
    id: root

    height: root.visible ? root.implicitHeight : 0
}
