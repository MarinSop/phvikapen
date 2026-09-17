pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property NotebookViewModel notebook

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
                text: root.notebook.title
            }

            ToolButton {
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
            model: root.notebook.sections

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
            model: root.notebook.pages

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
        }
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
