import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

// Every window of the application that sits over the page: one surface, one line around it, and
// the same dimming behind it.
Dialog {
    id: root

    anchors.centerIn: Overlay.overlay
    modal: true
    padding: 20
    parent: Overlay.overlay

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, Theme.dark ? 0.55 : 0.35)
    }
    background: Rectangle {
        border.color: Theme.line
        border.width: 1
        color: Theme.surface
        radius: 10
    }
}
