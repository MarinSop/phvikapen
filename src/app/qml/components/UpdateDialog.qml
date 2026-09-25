pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

AppDialog {
    id: root

    required property UpdateViewModel updates
    readonly property bool offering: root.updates.state === UpdateViewModel.Available
    readonly property bool getting: root.updates.state === UpdateViewModel.Getting
    readonly property bool ready: root.updates.state === UpdateViewModel.Ready

    function offer() {
        askAgain.checked = false;
        root.open();
    }

    closePolicy: root.getting ? Popup.CloseOnEscape | Popup.CloseOnPressOutside : Popup.NoAutoClose
    objectName: "updateDialog"
    title: root.ready ? qsTr("Ready to install") : root.getting ? qsTr("Getting the update") : qsTr("An update is ready")
    width: 400

    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        background: null
        bottomPadding: 12
        leftPadding: 12
        rightPadding: 12
        spacing: 8
        topPadding: 6

        Button {
            objectName: "updateLaterButton"
            text: root.getting ? qsTr("Keep going in the background") : qsTr("Not now")

            onClicked: {
                if (root.offering && askAgain.checked) {
                    root.updates.skipThisVersion();
                }
                root.close();
            }
        }

        Button {
            highlighted: true
            objectName: "updateNowButton"
            text: root.ready ? qsTr("Restart now") : qsTr("Update now")
            visible: !root.getting

            onClicked: {
                if (root.ready) {
                    root.updates.restartNow();
                } else {
                    root.updates.get();
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label {
            Layout.fillWidth: true
            text: root.ready ? qsTr("Version %1 is on this machine. PhvikaPen has to start again to use it.").arg(root.updates.version) : root.getting ? qsTr("Getting version %1…").arg(root.updates.version) : qsTr("Version %1 is ready to install. Your work is not touched.").arg(root.updates.version)
            wrapMode: Text.WordWrap
        }

        ProgressBar {
            Layout.fillWidth: true
            from: 0
            objectName: "updateProgress"
            to: 100
            value: root.updates.howFarAlong
            visible: root.getting || root.ready
        }

        Label {
            Layout.fillWidth: true
            color: palette.placeholderText
            text: qsTr("%1%").arg(root.updates.howFarAlong)
            visible: root.getting
        }

        CheckBox {
            id: askAgain

            objectName: "skipVersionBox"
            text: qsTr("Do not ask about this version again")
            visible: root.offering
        }

        Label {
            Layout.fillWidth: true
            color: palette.placeholderText
            text: qsTr("Closing this window leaves the update getting itself in the background.")
            visible: root.getting
            wrapMode: Text.WordWrap
        }
    }
}
