import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import InkRecorder

ApplicationWindow {
    id: root

    height: 700
    title: qsTr("Ink Recorder")
    visible: true
    width: 900

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 8

            Button {
                text: recorder.recording ? qsTr("Stop") : qsTr("Start")

                onClicked: recorder.recording ? recorder.stop() : recorder.start()
            }

            Label {
                Layout.fillWidth: true
                elide: Text.ElideMiddle
                text: recorder.filePath === "" ? qsTr("No recording yet") : recorder.filePath
            }

            Label {
                text: qsTr("%1 samples").arg(recorder.sampleCount)
            }
        }
    }

    RecorderItem {
        id: recorder

        anchors.fill: parent

        onFailed: message => errorLabel.text = message
    }

    Label {
        id: errorLabel

        anchors.centerIn: parent
        color: "#b00020"
        text: ""
        visible: errorLabel.text !== ""
    }

    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 16
        color: "#808080"
        text: qsTr("Press Start, then write across this area with the pen or the mouse.")
    }
}
