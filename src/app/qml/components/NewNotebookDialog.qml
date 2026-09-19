pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import PhvikaPen.Ui

AppDialog {
    id: root

    property url document
    required property NotebooksViewModel notebooks
    required property SettingsViewModel settings
    readonly property list<int> backgrounds: [PageOptions.Blank, PageOptions.Lined, PageOptions.Grid, PageOptions.Dotted]
    readonly property list<string> backgroundNames: [qsTr("Blank"), qsTr("Lined"), qsTr("Squares"), qsTr("Dots")]
    readonly property list<int> papers: [PageOptions.Infinite, PageOptions.A3, PageOptions.A4, PageOptions.A5, PageOptions.Letter, PageOptions.Legal, PageOptions.Custom]
    readonly property list<string> paperNames: [qsTr("Infinite"), qsTr("A3"), qsTr("A4"), qsTr("A5"), qsTr("Letter"), qsTr("Legal"), qsTr("Own size")]

    objectName: "newNotebookDialog"
    standardButtons: Dialog.Ok | Dialog.Cancel
    title: qsTr("New notebook")
    width: 420

    onAccepted: {
        root.settings.paper = root.papers[paperBox.currentIndex];
        root.settings.background = root.backgrounds[backgroundBox.currentIndex];
        root.settings.landscape = landscapeSwitch.checked;
        root.notebooks.createNotebookWithSetup(nameField.text.trim(), root.papers[paperBox.currentIndex], root.backgrounds[backgroundBox.currentIndex], landscapeSwitch.checked, root.document);
    }
    onOpened: {
        root.document = "";
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
            text: qsTr("Start from")
        }

        Button {
            Layout.fillWidth: true
            highlighted: root.document.toString() === ""
            objectName: "blankStartButton"
            text: qsTr("A blank notebook")

            onClicked: root.document = ""
        }

        Button {
            Layout.fillWidth: true
            highlighted: root.document.toString() !== ""
            objectName: "documentStartButton"
            text: root.document.toString() === "" ? qsTr("A PDF or picture…") : decodeURIComponent(root.document.toString().split("/").pop())

            onClicked: documentDialog.open()
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

        SizeFields {
            Layout.fillWidth: true
            heightMillimetres: root.settings.customHeight
            objectName: "newNotebookSize"
            visible: root.papers[paperBox.currentIndex] === PageOptions.Custom
            widthMillimetres: root.settings.customWidth

            onSizeEdited: (wide, tall) => {
                root.settings.customWidth = wide;
                root.settings.customHeight = tall;
            }
        }

        Switch {
            id: landscapeSwitch

            enabled: root.papers[paperBox.currentIndex] !== PageOptions.Infinite && root.papers[paperBox.currentIndex] !== PageOptions.Custom
            objectName: "newNotebookLandscape"
            text: qsTr("Landscape")
        }
    }

    FileDialog {
        id: documentDialog

        nameFilters: [qsTr("Documents and pictures (*.pdf *.png *.jpg *.jpeg *.webp)")]
        objectName: "newNotebookDocument"
        title: qsTr("Start from")

        onAccepted: root.document = documentDialog.selectedFile
    }
}
