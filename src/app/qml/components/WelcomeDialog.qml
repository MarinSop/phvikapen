pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

// The first thing the application asks: which of the three it should wear.
AppDialog {
    id: root

    required property SettingsViewModel settings

    closePolicy: Popup.NoAutoClose
    objectName: "welcomeDialog"
    standardButtons: Dialog.Ok
    title: qsTr("Welcome to PhvikaPen")
    width: 560

    onAccepted: root.settings.themeChosen = true

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Image {
                Layout.preferredHeight: 48
                Layout.preferredWidth: 48
                fillMode: Image.PreserveAspectFit
                mipmap: true
                source: Theme.logo
                sourceSize.height: 128
                sourceSize.width: 128
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("Pick the look you would like. You can change it later in Settings.")
                wrapMode: Text.WordWrap
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: [Theme.Brand, Theme.Dark, Theme.Light]

                ThemeCard {
                    id: themeCard

                    required property int modelData

                    Layout.fillWidth: true
                    checked: root.settings.theme === themeCard.modelData
                    mode: themeCard.modelData
                    objectName: "welcomeTheme" + themeCard.modelData

                    onClicked: root.settings.theme = themeCard.modelData
                }
            }
        }
    }
}
