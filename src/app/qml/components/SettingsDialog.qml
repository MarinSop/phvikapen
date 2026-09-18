pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Dialog {
    id: root

    required property SettingsViewModel settings
    required property UpdateViewModel updates

    anchors.centerIn: parent
    modal: true
    objectName: "settingsDialog"
    standardButtons: Dialog.Close
    title: qsTr("Settings")
    width: 460
    height: 560

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            font.bold: true
            text: qsTr("Updates")
        }

        Switch {
            checked: root.settings.lookForUpdates
            objectName: "lookForUpdatesSwitch"
            text: qsTr("Look for a newer version at start")

            onToggled: root.settings.lookForUpdates = checked
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: {
                    switch (root.updates.state) {
                    case UpdateViewModel.Looking:
                        return qsTr("Looking…");
                    case UpdateViewModel.Available:
                        return qsTr("Version %1 is ready to install").arg(root.updates.version);
                    case UpdateViewModel.Installing:
                        return qsTr("Getting version %1…").arg(root.updates.version);
                    case UpdateViewModel.UpToDate:
                        return qsTr("This is the newest version");
                    case UpdateViewModel.Unavailable:
                        return qsTr("No updates from here");
                    default:
                        return qsTr("Version %1").arg(AppInfo.version);
                    }
                }
            }

            Button {
                enabled: !root.updates.busy
                objectName: "lookNowButton"
                text: qsTr("Look now")

                onClicked: root.updates.check()
            }
        }

        MenuSeparator {
            Layout.fillWidth: true
        }

        Label {
            font.bold: true
            text: qsTr("New notebooks")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ComboBox {
                Layout.fillWidth: true
                currentIndex: root.settings.paper
                model: [qsTr("Infinite"), qsTr("A3"), qsTr("A4"), qsTr("A5"), qsTr("Letter"), qsTr("Legal")]
                objectName: "defaultPaperBox"

                onActivated: root.settings.paper = currentIndex
            }

            ComboBox {
                Layout.fillWidth: true
                currentIndex: root.settings.background
                model: [qsTr("Blank"), qsTr("Lined"), qsTr("Squares"), qsTr("Dots")]
                objectName: "defaultBackgroundBox"

                onActivated: root.settings.background = currentIndex
            }
        }

        Switch {
            checked: root.settings.landscape
            objectName: "defaultLandscapeSwitch"
            text: qsTr("Landscape")

            onToggled: root.settings.landscape = checked
        }

        MenuSeparator {
            Layout.fillWidth: true
        }

        Label {
            font.bold: true
            text: qsTr("Notebooks")
        }

        Label {
            Layout.fillWidth: true
            elide: Text.ElideMiddle
            text: root.settings.notebookFolder
        }

        Button {
            objectName: "openFolderButton"
            text: qsTr("Open the folder")

            onClicked: root.settings.showNotebookFolder()
        }

        MenuSeparator {
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("PhvikaPen %1").arg(AppInfo.version)
        }
    }
}
