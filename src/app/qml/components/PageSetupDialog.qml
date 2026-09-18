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
            text: qsTr("The setup belongs to this page. New pages start the way the page they were added after does.")
            wrapMode: Text.WordWrap
        }

        Button {
            objectName: "applyToSectionButton"
            text: qsTr("Give every page in the section this setup")

            onClicked: root.notebook.applyStyleToSection()
        }
    }
}
