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
    height: 720

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
            text: qsTr("Writing")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: qsTr("Smoothing")
            }

            NumberField {
                maximum: 10
                minimum: 0
                number: Math.round(root.settings.smoothing * 10)
                objectName: "smoothingField"
                step: 1

                onNumberEdited: value => root.settings.smoothing = value / 10
            }
        }

        Label {
            Layout.fillWidth: true
            color: palette.placeholderText
            text: qsTr("0 keeps every wobble of the pen, 10 irons the line out.")
            wrapMode: Text.WordWrap
        }

        MenuSeparator {
            Layout.fillWidth: true
        }

        Label {
            font.bold: true
            text: qsTr("Export")
        }

        Switch {
            checked: root.settings.exportEverything
            objectName: "exportEverythingSwitch"
            text: qsTr("Take in what is written beside the sheet")

            onToggled: root.settings.exportEverything = checked
        }

        Label {
            Layout.fillWidth: true
            color: palette.placeholderText
            text: qsTr("Off, a page of the document holds only the sheet, and anything written around it is left out.")
            wrapMode: Text.WordWrap
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
            font.bold: true
            text: qsTr("Keys")
        }

        Label {
            Layout.fillWidth: true
            color: palette.placeholderText
            text: keysMessage.text === "" ? qsTr("Click a key field and press the keys you want.") : keysMessage.text
            wrapMode: Text.WordWrap
        }

        ListView {
            id: keyList

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 160
            clip: true
            model: root.settings.shortcutList
            objectName: "shortcutList"

            delegate: RowLayout {
                id: keyRow

                required property bool changed
                required property string commandId
                required property string name
                required property string sequence

                spacing: 8
                width: keyList.width

                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: keyRow.name
                }

                ShortcutField {
                    sequence: keyRow.sequence

                    onCaptured: wanted => {
                        const taken = root.settings.conflictWith(keyRow.commandId, wanted);
                        if (taken !== "") {
                            keysMessage.text = qsTr("%1 already uses %2").arg(taken).arg(wanted);
                            return;
                        }
                        if (root.settings.changeShortcut(keyRow.commandId, wanted)) {
                            keysMessage.text = "";
                        }
                    }
                }

                ToolButton {
                    enabled: keyRow.changed
                    text: qsTr("Reset")

                    onClicked: {
                        root.settings.resetShortcut(keyRow.commandId);
                        keysMessage.text = "";
                    }
                }
            }
        }

        MenuSeparator {
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("PhvikaPen %1").arg(AppInfo.version)
        }

        QtObject {
            id: keysMessage

            property string text: ""
        }
    }
}
