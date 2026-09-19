pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

AppDialog {
    id: root

    required property SettingsViewModel settings
    required property UpdateViewModel updates

    height: 540
    objectName: "settingsDialog"
    padding: 10
    standardButtons: Dialog.Close
    title: qsTr("Settings")
    width: 560

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        TabBar {
            id: tabs

            Layout.fillWidth: true
            objectName: "settingsTabs"

            TabButton {
                objectName: "lookTab"
                text: qsTr("Look")
            }

            TabButton {
                objectName: "writingTab"
                text: qsTr("Writing")
            }

            TabButton {
                objectName: "pagesTab"
                text: qsTr("Pages")
            }

            TabButton {
                objectName: "keysTab"
                text: qsTr("Keys")
            }

            TabButton {
                objectName: "generalTab"
                text: qsTr("General")
            }
        }

        StackLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            currentIndex: tabs.currentIndex

            SettingsPage {
                objectName: "lookPage"

                Label {
                    font.bold: true
                    text: qsTr("Theme")
                }

                RowLayout {
                    Layout.fillWidth: true
                    objectName: "themeRow"
                    spacing: 8

                    Repeater {
                        model: [Theme.Brand, Theme.Dark, Theme.Light]

                        ThemeCard {
                            id: themeCard

                            required property int modelData

                            Layout.fillWidth: true
                            checked: root.settings.theme === themeCard.modelData
                            mode: themeCard.modelData
                            objectName: "themeCard" + themeCard.modelData

                            onClicked: root.settings.theme = themeCard.modelData
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    color: palette.placeholderText
                    text: Theme.noteOf(root.settings.theme)
                    wrapMode: Text.WordWrap
                }
            }

            SettingsPage {
                objectName: "writingPage"

                Label {
                    font.bold: true
                    text: qsTr("The line")
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
                    text: qsTr("Reading")
                }

                Switch {
                    checked: root.settings.continuousPages
                    objectName: "continuousPagesSwitch"
                    text: qsTr("Pages one below the other")

                    onToggled: root.settings.continuousPages = checked
                }

                Label {
                    Layout.fillWidth: true
                    color: palette.placeholderText
                    text: qsTr("On, the pages of a section stand in one column and scrolling carries on into the next one. Off, one page fills the window at a time.")
                    wrapMode: Text.WordWrap
                }

                MenuSeparator {
                    Layout.fillWidth: true
                }

                Label {
                    font.bold: true
                    text: qsTr("Export")
                }

                ComboBox {
                    id: exportScopeBox

                    Layout.fillWidth: true
                    currentIndex: root.settings.exportScope
                    model: [qsTr("Sheets and everything around them"), qsTr("Only the sheets"), qsTr("Only the imported document")]
                    objectName: "exportScopeBox"

                    onActivated: root.settings.exportScope = exportScopeBox.currentIndex
                }

                Label {
                    Layout.fillWidth: true
                    color: palette.placeholderText
                    text: qsTr("What File ▸ Export as PDF offers first. Each of the three is its own item in that menu.")
                    wrapMode: Text.WordWrap
                }
            }

            SettingsPage {
                objectName: "pagesPage"

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
                        model: [qsTr("Infinite"), qsTr("A3"), qsTr("A4"), qsTr("A5"), qsTr("Letter"), qsTr("Legal"), qsTr("Own size")]
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

                SizeFields {
                    Layout.fillWidth: true
                    heightMillimetres: root.settings.customHeight
                    objectName: "defaultSize"
                    visible: root.settings.paper === PageOptions.Custom
                    widthMillimetres: root.settings.customWidth

                    onSizeEdited: (wide, tall) => {
                        root.settings.customWidth = wide;
                        root.settings.customHeight = tall;
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
                    text: qsTr("How new pages are ruled")
                }

                PageLook {
                    Layout.fillWidth: true
                    lineColor: root.settings.lineColor
                    lineSpacing: root.settings.lineSpacing
                    lineWidth: root.settings.lineWidth
                    lined: root.settings.background === PageOptions.Lined
                    marginAt: root.settings.marginAt
                    marginColor: root.settings.marginColor
                    marginShown: root.settings.margin
                    objectName: "defaultLook"
                    paperColor: root.settings.paperColor

                    onLineColorPicked: wanted => root.settings.lineColor = wanted
                    onLineSpacingChosen: wanted => root.settings.lineSpacing = wanted
                    onLineWidthChosen: wanted => root.settings.lineWidth = wanted
                    onMarginAtChosen: wanted => root.settings.marginAt = wanted
                    onMarginColorPicked: wanted => root.settings.marginColor = wanted
                    onMarginShownChosen: wanted => root.settings.margin = wanted
                    onPaperColorPicked: wanted => root.settings.paperColor = wanted
                }

                MenuSeparator {
                    Layout.fillWidth: true
                }

                Label {
                    font.bold: true
                    text: qsTr("Where the notebooks are kept")
                }

                Label {
                    Layout.fillWidth: true
                    color: palette.placeholderText
                    elide: Text.ElideMiddle
                    text: root.settings.notebookFolder
                }

                Button {
                    objectName: "openFolderButton"
                    text: qsTr("Open the folder")

                    onClicked: root.settings.showNotebookFolder()
                }
            }

            SettingsPage {
                objectName: "keysPage"

                Label {
                    Layout.fillWidth: true
                    color: palette.placeholderText
                    text: keysMessage.text === "" ? qsTr("Click a key field and press the keys you want.") : keysMessage.text
                    wrapMode: Text.WordWrap
                }

                ListView {
                    id: keyList

                    Layout.fillWidth: true
                    Layout.preferredHeight: contentHeight
                    clip: true
                    interactive: false
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
            }

            SettingsPage {
                objectName: "updatesPage"

                Label {
                    font.bold: true
                    text: qsTr("Starting up")
                }

                Switch {
                    checked: root.settings.reopenNotebooks
                    objectName: "reopenNotebooksSwitch"
                    text: qsTr("Open the notebooks that were open last time")

                    onToggled: root.settings.reopenNotebooks = checked
                }

                Label {
                    Layout.fillWidth: true
                    color: palette.placeholderText
                    text: qsTr("Off, PhvikaPen starts with nothing open and waits for you to make or open a notebook.")
                    wrapMode: Text.WordWrap
                }

                MenuSeparator {
                    Layout.fillWidth: true
                }

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
                    color: palette.placeholderText
                    text: qsTr("PhvikaPen %1").arg(AppInfo.version)
                }
            }
        }
    }

    QtObject {
        id: keysMessage

        property string text: ""
    }
}
