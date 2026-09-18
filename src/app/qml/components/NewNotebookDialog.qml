pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Dialog {
    id: root

    required property NotebooksViewModel notebooks
    required property SettingsViewModel settings
    readonly property list<int> backgrounds: [PageOptions.Blank, PageOptions.Lined, PageOptions.Grid, PageOptions.Dotted]
    readonly property list<string> backgroundNames: [qsTr("Blank"), qsTr("Lined"), qsTr("Grid"), qsTr("Dotted")]
    readonly property list<int> papers: [PageOptions.Infinite, PageOptions.A3, PageOptions.A4, PageOptions.A5, PageOptions.Letter, PageOptions.Legal]
    readonly property list<string> paperNames: [qsTr("Infinite"), qsTr("A3"), qsTr("A4"), qsTr("A5"), qsTr("Letter"), qsTr("Legal")]

    anchors.centerIn: parent
    modal: true
    objectName: "newNotebookDialog"
    standardButtons: Dialog.Ok | Dialog.Cancel
    title: qsTr("New notebook")
    width: 380

    onAccepted: root.notebooks.createNotebookWithSetup(nameField.text.trim(), root.papers[paperBox.currentIndex], root.backgrounds[backgroundBox.currentIndex], landscapeSwitch.checked)
    onOpened: {
        nameField.text = root.notebooks.suggestedName();
        paperBox.currentIndex = root.papers.indexOf(root.settings.paper);
        backgroundBox.currentIndex = root.backgrounds.indexOf(root.settings.background);
        landscapeSwitch.checked = root.settings.landscape;
        nameField.selectAll();
        nameField.forceActiveFocus();
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: qsTr("Name")
        }

        TextField {
            id: nameField

            Layout.fillWidth: true
            objectName: "newNotebookName"

            onAccepted: root.accept()
        }

        Label {
            text: qsTr("Pages start as")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ComboBox {
                id: paperBox

                Layout.fillWidth: true
                model: root.paperNames
                objectName: "newNotebookPaper"
            }

            ComboBox {
                id: backgroundBox

                Layout.fillWidth: true
                model: root.backgroundNames
                objectName: "newNotebookBackground"
            }
        }

        Switch {
            id: landscapeSwitch

            enabled: root.papers[paperBox.currentIndex] !== PageOptions.Infinite
            objectName: "newNotebookLandscape"
            text: qsTr("Landscape")
        }
    }
}
