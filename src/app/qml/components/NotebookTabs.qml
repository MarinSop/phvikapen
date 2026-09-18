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

        Image {
            Layout.leftMargin: 8
            Layout.preferredHeight: 22
            Layout.preferredWidth: 22
            fillMode: Image.PreserveAspectFit
            mipmap: true
            objectName: "brandMark"
            source: Theme.logo
            sourceSize.height: 64
            sourceSize.width: 64
        }

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

                    implicitHeight: 28
                    text: tab.modelData
                    width: Math.min(160, implicitContentWidth + 24)

                    onPressAndHold: tabMenu.popup()

                    TapHandler {
                        acceptedButtons: Qt.RightButton

                        onTapped: tabMenu.popup()
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
            implicitHeight: 28
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
