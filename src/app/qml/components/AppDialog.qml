import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

// Every window of the application that sits over the page: one surface, one line around it, and
// the same dimming behind it.
Dialog {
    id: root

    anchors.centerIn: Overlay.overlay
    modal: true
    padding: 16
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
    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        background: null
        bottomPadding: 16
        leftPadding: 16
        rightPadding: 16
        spacing: 8
        topPadding: 8
        visible: count > 0
    }
}
