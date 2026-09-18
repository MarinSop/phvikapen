import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

Pane {
    id: root

    property string message: ""

    function show(text) {
        root.message = text;
        hideTimer.restart();
    }

    opacity: root.message === "" ? 0 : 1
    padding: 12
    visible: opacity > 0

    Behavior on opacity {
        NumberAnimation {
            duration: 150
        }
    }
    background: Rectangle {
        border.color: Theme.line
        border.width: 1
        color: Theme.surface
        radius: 8
    }

    Label {
        color: Theme.text
        objectName: "messageLabel"
        text: root.message
    }

    Timer {
        id: hideTimer

        interval: 6000

        onTriggered: root.message = ""
    }
}
