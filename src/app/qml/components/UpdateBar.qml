import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property UpdateViewModel updates

    objectName: "updateBar"
    visible: root.updates.state === UpdateViewModel.Available || root.updates.state === UpdateViewModel.Installing

    RowLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            Layout.fillWidth: true
            elide: Text.ElideRight
            text: root.updates.state === UpdateViewModel.Installing ? qsTr("Getting version %1…").arg(root.updates.version) : qsTr("Version %1 is ready to install").arg(root.updates.version)
        }

        Button {
            enabled: !root.updates.busy
            objectName: "installUpdateButton"
            text: qsTr("Install and restart")

            onClicked: root.updates.install()
        }
    }
}
