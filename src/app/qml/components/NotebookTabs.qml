pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Item {
    id: root

    property int draggedTab: -1
    required property AppActions actions
    required property NotebooksViewModel notebooks

    implicitHeight: 36

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
            Layout.leftMargin: 7
            Layout.preferredHeight: 26
            Layout.preferredWidth: 26
            fillMode: Image.PreserveAspectFit
            mipmap: true
            objectName: "brandMark"
            source: Theme.logo
            sourceSize.height: 64
            sourceSize.width: 64
        }

        TabBar {
            id: tabBar

            Layout.fillHeight: true
            Layout.fillWidth: true
            background: null
            currentIndex: root.notebooks.currentIndex

            onCurrentIndexChanged: root.notebooks.currentIndex = tabBar.currentIndex

            Repeater {
                model: root.notebooks.openNotebooks

                TabButton {
                    id: tab

                    required property int index
                    required property string modelData
                    readonly property bool open: tabBar.currentIndex === tab.index

                    height: tabBar.height
                    width: Math.min(200, name.implicitWidth + 52)
                    Drag.active: tabDrag.active
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2
                    Drag.source: tab

                    background: Rectangle {
                        color: tab.open ? Theme.surface : "transparent"
                        radius: 5
                    }
                    contentItem: Label {
                        id: name

                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        leftPadding: 10
                        rightPadding: 26
                        text: tab.modelData
                        verticalAlignment: Text.AlignVCenter
                    }

                    onPressAndHold: tabMenu.popup()

                    DragHandler {
                        id: tabDrag

                        target: null
                        xAxis.enabled: true
                        yAxis.enabled: false

                        onActiveChanged: {
                            if (active) {
                                root.draggedTab = tab.index;
                                tab.grabToImage(function (result) {
                                    tab.Drag.imageSource = result.url;
                                });
                            } else {
                                tab.Drag.drop();
                                root.draggedTab = -1;
                            }
                        }
                    }

                    DropArea {
                        anchors.fill: parent

                        onDropped: {
                            if (root.draggedTab >= 0 && root.draggedTab !== tab.index) {
                                root.notebooks.moveNotebook(root.draggedTab, tab.index);
                            }
                        }
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton

                        onTapped: tabMenu.popup()
                    }

                    ToolButton {
                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        bottomPadding: 0
                        display: AbstractButton.IconOnly
                        icon.color: palette.buttonText
                        icon.height: 16
                        icon.source: Icons.close
                        icon.width: 16
                        implicitHeight: 20
                        implicitWidth: 20
                        leftPadding: 0
                        objectName: "closeNotebookButton"
                        opacity: tab.open || tab.hovered ? 1 : 0
                        rightPadding: 0
                        topPadding: 0

                        onClicked: root.actions.askToClose(tab.index)
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

                            onTriggered: root.actions.askToClose(tab.index)
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

        QuickButton {
            Layout.rightMargin: 6
            icon.source: Icons.plus
            label: qsTr("Open or create a notebook")
            objectName: "notebooksButton"

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

    ConfirmDialog {
        id: deleteDialog

        property string notebookName: ""

        question: qsTr("Move “%1” and everything in it to the trash?").arg(deleteDialog.notebookName)
        title: qsTr("Delete notebook")

        onAccepted: root.notebooks.deleteNotebook(deleteDialog.notebookName)
    }
}
