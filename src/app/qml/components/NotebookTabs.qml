pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Item {
    id: root

    required property NotebooksViewModel notebooks

    implicitHeight: tabRow.implicitHeight

    Rectangle {
        anchors.fill: parent
        color: Theme.surfaceStrong
        z: -1
    }

    RowLayout {
        id: tabRow

        anchors.fill: parent
        spacing: 4

        TabBar {
            id: tabBar

            Layout.fillWidth: true
            currentIndex: root.notebooks.currentIndex

            onCurrentIndexChanged: root.notebooks.currentIndex = tabBar.currentIndex

            Repeater {
                model: root.notebooks.openNotebooks

                TabButton {
                    id: tab

                    required property int index
                    required property string modelData

                    text: tab.modelData
                    width: Math.min(200, Math.max(120, implicitWidth + 32))

                    onPressAndHold: tabMenu.popup()

                    TapHandler {
                        acceptedButtons: Qt.RightButton

                        onTapped: tabMenu.popup()
                    }

                    ToolButton {
                        anchors.right: parent.right
                        anchors.rightMargin: 2
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("×")
                        visible: tabBar.currentIndex === tab.index
                        width: 24

                        onClicked: root.notebooks.closeNotebook(tab.index)
                    }

                    Menu {
                        id: tabMenu

                        MenuItem {
                            text: qsTr("Rename…")

                            onTriggered: {
                                renameDialog.index = tab.index;
                                renameField.text = tab.modelData;
                                renameDialog.open();
                            }
                        }

                        MenuItem {
                            text: qsTr("Close")

                            onTriggered: root.notebooks.closeNotebook(tab.index)
                        }

                        MenuItem {
                            text: qsTr("Delete…")

                            onTriggered: {
                                deleteDialog.notebookName = tab.modelData;
                                deleteDialog.open();
                            }
                        }
                    }
                }
            }
        }

        ToolButton {
            objectName: "notebooksButton"
            text: qsTr("+")
            ToolTip.text: qsTr("Open or create a notebook")
            ToolTip.visible: hovered

            onClicked: libraryMenu.popup()

            Menu {
                id: libraryMenu

                MenuItem {
                    text: qsTr("New notebook…")

                    onTriggered: {
                        newField.text = root.notebooks.suggestedName();
                        newDialog.open();
                    }
                }

                MenuSeparator {
                }

                Repeater {
                    model: root.notebooks.library

                    MenuItem {
                        required property string modelData

                        text: modelData

                        onTriggered: root.notebooks.openNotebook(modelData)
                    }
                }
            }
        }
    }

    AppDialog {
        id: newDialog

        standardButtons: Dialog.Ok | Dialog.Cancel
        title: qsTr("New notebook")

        onAccepted: root.notebooks.createNotebook(newField.text.trim())

        TextField {
            id: newField

            width: 240

            onAccepted: newDialog.accept()
        }
    }

    AppDialog {
        id: renameDialog

        property int index: 0

        standardButtons: Dialog.Ok | Dialog.Cancel
        title: qsTr("Rename notebook")

        onAccepted: root.notebooks.renameNotebook(renameDialog.index, renameField.text)

        TextField {
            id: renameField

            width: 240

            onAccepted: renameDialog.accept()
        }
    }

    AppDialog {
        id: deleteDialog

        property string notebookName: ""

        standardButtons: Dialog.Ok | Dialog.Cancel
        title: qsTr("Delete notebook")

        onAccepted: root.notebooks.deleteNotebook(deleteDialog.notebookName)

        Label {
            text: qsTr("Move “%1” and everything in it to the trash?").arg(deleteDialog.notebookName)
        }
    }
}
