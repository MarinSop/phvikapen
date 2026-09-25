pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property UpdateViewModel updates
    property bool putAside: false
    readonly property bool getting: root.updates.state === UpdateViewModel.Getting
    readonly property bool ready: root.updates.state === UpdateViewModel.Ready
    readonly property bool worthShowing: root.ready || root.getting || (root.updates.worthOffering && !root.putAside)

    signal openWanted

    objectName: "updateBar"
    padding: 8
    visible: root.worthShowing

    background: Rectangle {
        color: Theme.accentSoft

        Rectangle {
            anchors.bottom: parent.bottom
            color: Theme.line
            height: 1
            width: parent.width
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            Layout.fillWidth: true
            elide: Text.ElideRight
            text: root.ready ? qsTr("Version %1 is ready to install").arg(root.updates.version) : root.getting ? qsTr("Getting version %1… %2%").arg(root.updates.version).arg(root.updates.howFarAlong) : qsTr("Version %1 is available").arg(root.updates.version)
        }

        ProgressBar {
            Layout.preferredWidth: Math.round(120 * Theme.scale)
            from: 0
            objectName: "updateBarProgress"
            to: 100
            value: root.updates.howFarAlong
            visible: root.getting
        }

        Button {
            objectName: "updateBarButton"
            text: root.ready ? qsTr("Restart now") : root.getting ? qsTr("Show") : qsTr("Update")

            onClicked: {
                if (root.ready) {
                    root.updates.restartNow();
                } else if (root.getting) {
                    root.openWanted();
                } else {
                    root.openWanted();
                }
            }
        }

        ShapeButton {
            icon.source: Icons.close
            label: qsTr("Put this aside")
            objectName: "updateBarAsideButton"
            visible: !root.getting && !root.ready

            onClicked: root.putAside = true
        }
    }
}
