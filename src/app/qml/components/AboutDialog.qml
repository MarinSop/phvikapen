import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

AppDialog {
    id: root

    objectName: "aboutDialog"
    standardButtons: Dialog.Close
    title: qsTr("About PhvikaPen")
    width: 460

    RowLayout {
        anchors.fill: parent
        spacing: 20

        Image {
            Layout.alignment: Qt.AlignTop
            Layout.preferredHeight: 96
            Layout.preferredWidth: 96
            fillMode: Image.PreserveAspectFit
            mipmap: true
            objectName: "aboutLogo"
            source: Theme.logo
            sourceSize.height: 192
            sourceSize.width: 192
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                font.bold: true
                font.pixelSize: 20
                text: qsTr("PhvikaPen")
            }

            Label {
                color: palette.placeholderText
                text: qsTr("Version %1").arg(AppInfo.version)
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("A pen-first note-taking application. Notebooks are kept on this computer alone.")
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                color: palette.placeholderText
                text: qsTr("Icons from Boxicons, under the MIT licence.")
                wrapMode: Text.WordWrap
            }
        }
    }
}
