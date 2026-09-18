import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Dialog {
    id: root

    anchors.centerIn: parent
    modal: true
    objectName: "aboutDialog"
    standardButtons: Dialog.Close
    title: qsTr("About PhvikaPen")

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            font.bold: true
            text: qsTr("PhvikaPen %1").arg(AppInfo.version)
        }

        Label {
            Layout.maximumWidth: 360
            text: qsTr("A pen-first note-taking application. Notebooks are kept on this computer alone.")
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.maximumWidth: 360
            text: qsTr("Icons from Boxicons, under the MIT licence.")
            wrapMode: Text.WordWrap
        }
    }
}
