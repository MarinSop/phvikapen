pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    property NotebookViewModel notebook: null
    readonly property bool ready: root.notebook !== null && root.notebook.loaded

    function rename(sectionScope, index, current) {
        renameDialog.sectionScope = sectionScope;
        renameDialog.index = index;
        renameField.text = current;
        renameDialog.open();
    }

    padding: 8

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: root.notebook === null ? "" : root.notebook.title
            }

            ToolButton {
                enabled: root.ready && !root.notebook.exporting
                objectName: "exportButton"
                text: qsTr("⤒")
                ToolTip.text: qsTr("Export the notebook as a PDF")
                ToolTip.visible: hovered

                onClicked: {
                    const folder = StandardPaths.writableLocation(StandardPaths.DocumentsLocation);
                    exportDialog.currentFolder = folder;
                    exportDialog.selectedFile = folder + "/" + root.notebook.title + ".pdf";
                    exportDialog.open();
                }
            }

            ToolButton {
                enabled: root.ready
                objectName: "addSectionButton"
                text: qsTr("+")
                ToolTip.text: qsTr("New section")
                ToolTip.visible: hovered

                onClicked: root.notebook.addSection()
            }
        }

        ListView {
            id: sectionList

            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 140)
            clip: true
            model: root.ready ? root.notebook.sections : null

            delegate: ItemDelegate {
                id: sectionDelegate

                required property int count
                required property int index
                required property string title

                highlighted: sectionDelegate.index === root.notebook.currentSection
                text: sectionDelegate.title + " (" + sectionDelegate.count + ")"
                width: sectionList.width

                onClicked: root.notebook.currentSection = sectionDelegate.index
                onPressAndHold: sectionMenu.popup()

                TapHandler {
                    acceptedButtons: Qt.RightButton

                    onTapped: sectionMenu.popup()
                }

                Menu {
                    id: sectionMenu

                    MenuItem {
                        text: qsTr("Rename…")

                        onTriggered: root.rename(true, sectionDelegate.index, sectionDelegate.title)
                    }

                    MenuItem {
                        enabled: sectionDelegate.index > 0
                        text: qsTr("Move up")

                        onTriggered: root.notebook.moveSection(sectionDelegate.index, sectionDelegate.index - 1)
                    }

                    MenuItem {
                        enabled: sectionDelegate.index + 1 < root.notebook.sectionCount
                        text: qsTr("Move down")

                        onTriggered: root.notebook.moveSection(sectionDelegate.index, sectionDelegate.index + 1)
                    }

                    MenuItem {
                        enabled: root.notebook.sectionCount > 1
                        text: qsTr("Delete")

                        onTriggered: root.notebook.deleteSection(sectionDelegate.index)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: qsTr("Pages")
            }

            ToolButton {
                enabled: root.ready
                objectName: "importButton"
                text: qsTr("⤓")
                ToolTip.text: qsTr("Import a PDF or a picture")
                ToolTip.visible: hovered

                onClicked: importDialog.open()
            }

            ToolButton {
                enabled: root.ready
                objectName: "addPageButton"
                text: qsTr("+")
                ToolTip.text: qsTr("New page")
                ToolTip.visible: hovered

                onClicked: root.notebook.addPage()
            }
        }

        ListView {
            id: pageList

            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            model: root.ready ? root.notebook.pages : null

            delegate: ItemDelegate {
                id: pageDelegate

                required property int index
                required property string title

                highlighted: pageDelegate.index === root.notebook.currentPage
                text: pageDelegate.title
                width: pageList.width

                onClicked: root.notebook.currentPage = pageDelegate.index
                onPressAndHold: pageMenu.popup()

                TapHandler {
                    acceptedButtons: Qt.RightButton

                    onTapped: pageMenu.popup()
                }

                Menu {
                    id: pageMenu

                    MenuItem {
                        text: qsTr("Rename…")

                        onTriggered: root.rename(false, pageDelegate.index, pageDelegate.title)
                    }

                    MenuItem {
                        enabled: pageDelegate.index > 0
                        text: qsTr("Move up")

                        onTriggered: root.notebook.movePage(pageDelegate.index, pageDelegate.index - 1)
                    }

                    MenuItem {
                        enabled: pageDelegate.index + 1 < root.notebook.pageCount
                        text: qsTr("Move down")

                        onTriggered: root.notebook.movePage(pageDelegate.index, pageDelegate.index + 1)
                    }

                    MenuItem {
                        enabled: root.notebook.pageCount > 1
                        text: qsTr("Delete")

                        onTriggered: root.notebook.deletePage(pageDelegate.index)
                    }
                }
            }
        }

        PageSetup {
            Layout.fillWidth: true
            notebook: root.notebook
            visible: root.ready
        }
    }

    FileDialog {
        id: exportDialog

        defaultSuffix: "pdf"
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("PDF documents (*.pdf)")]
        title: qsTr("Export as PDF")

        onAccepted: root.notebook.exportToPdf(exportDialog.selectedFile)
    }

    FileDialog {
        id: importDialog

        nameFilters: [qsTr("Documents and pictures (*.pdf *.png *.jpg *.jpeg *.webp)")]
        title: qsTr("Import")

        onAccepted: root.notebook.importDocument(importDialog.selectedFile)
    }

    Dialog {
        id: renameDialog

        property int index: 0
        property bool sectionScope: true

        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        title: renameDialog.sectionScope ? qsTr("Rename section") : qsTr("Rename page")

        onAccepted: {
            if (renameDialog.sectionScope) {
                root.notebook.renameSection(renameDialog.index, renameField.text);
            } else {
                root.notebook.renamePage(renameDialog.index, renameField.text);
            }
        }

        TextField {
            id: renameField

            width: 220

            onAccepted: renameDialog.accept()
        }
    }
}
