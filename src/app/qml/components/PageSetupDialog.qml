import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

AppDialog {
    id: root

    property NotebookViewModel notebook: null

    objectName: "pageSetupDialog"
    standardButtons: Dialog.Close
    title: qsTr("Page setup")
    width: 380

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        PageSetup {
            Layout.fillWidth: true
            notebook: root.notebook
        }

        Label {
            Layout.fillWidth: true
            color: palette.placeholderText
            text: qsTr("The setup belongs to the whole section: every page in it is written on the same paper.")
            wrapMode: Text.WordWrap
        }
    }
}
