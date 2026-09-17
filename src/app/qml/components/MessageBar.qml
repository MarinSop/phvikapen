import QtQuick
import QtQuick.Controls

Pane {
    id: root

    property string message: ""

    function show(text) {
        root.message = text;
        hideTimer.restart();
    }

    opacity: root.message === "" ? 0 : 1
    visible: opacity > 0

    Behavior on opacity {
        NumberAnimation {
            duration: 150
        }
    }

    Label {
        objectName: "messageLabel"
        text: root.message
    }

    Timer {
        id: hideTimer

        interval: 6000

        onTriggered: root.message = ""
    }
}
