import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

// Every window of the application that sits over the page: one surface, one line around it, and
// the same dimming behind it.
Dialog {
    id: root

    anchors.centerIn: Overlay.overlay
    modal: true
    padding: 12
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
        bottomPadding: 12
        leftPadding: 12
        rightPadding: 12
        spacing: 8
        topPadding: 6
        visible: count > 0

        delegate: Button {
            id: answer

            highlighted: answer.DialogButtonBox.buttonRole === DialogButtonBox.AcceptRole

            background: Rectangle {
                border.color: Theme.line
                border.width: 1
                color: answer.pressed ? Theme.accent : answer.hovered ? Theme.line : answer.highlighted ? Theme.base : "transparent"
                radius: 6
            }
        }
    }
}
