import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

// A short question with two answers, no wider than the question needs.
AppDialog {
    id: root

    property alias question: message.text

    standardButtons: Dialog.Ok | Dialog.Cancel
    width: 330

    Label {
        id: message

        width: parent.width
        wrapMode: Text.WordWrap
    }
}
