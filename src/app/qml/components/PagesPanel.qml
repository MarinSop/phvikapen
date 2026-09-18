pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    property int draggedPage: -1
    required property AppActions actions
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property bool ready: root.notebook !== null && root.notebook.loaded

    function rename(sectionScope, index, current) {
        renameDialog.sectionScope = sectionScope;
        renameDialog.index = index;
        renameField.text = current;
        renameDialog.open();
    }

    objectName: "pagesPanel"
    padding: 8

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                font.bold: true
                text: qsTr("Sections")
            }

            QuickButton {
                action: root.actions.addSection
                display: AbstractButton.IconOnly
                icon.source: Icons.plus
                label: qsTr("New section")
                objectName: "addSectionButton"
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

                highlighted: root.ready && sectionDelegate.index === root.notebook.currentSection
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
                        enabled: root.ready && sectionDelegate.index + 1 < root.notebook.sectionCount
                        text: qsTr("Move down")

                        onTriggered: root.notebook.moveSection(sectionDelegate.index, sectionDelegate.index + 1)
                    }

                    MenuItem {
                        enabled: root.ready && root.notebook.sectionCount > 1
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
                font.bold: true
                text: qsTr("Pages")
            }

            QuickButton {
                action: root.actions.addPage
                display: AbstractButton.IconOnly
                icon.source: Icons.plus
                label: qsTr("New page")
                objectName: "addPageButton"
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
                readonly property bool panelReady: root.ready
                required property string thumbnail
                required property string title

                function askForThumbnail() {
                    if (root.ready && pageDelegate.thumbnail === "") {
                        root.notebook.wantThumbnail(pageDelegate.index);
                    }
                }

                Drag.active: pageDrag.active
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2
                Drag.source: pageDelegate
                height: 64
                highlighted: root.ready && pageDelegate.index === root.notebook.currentPage
                width: pageList.width
                z: pageDrag.active ? 2 : 1

                contentItem: RowLayout {
                    spacing: 8

                    Rectangle {
                        Layout.preferredHeight: 56
                        Layout.preferredWidth: 44
                        border.color: palette.mid
                        border.width: 1
                        color: palette.base

                        Image {
                            anchors.fill: parent
                            anchors.margins: 1
                            cache: false
                            fillMode: Image.PreserveAspectFit
                            source: pageDelegate.thumbnail
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        text: pageDelegate.title
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Component.onCompleted: pageDelegate.askForThumbnail()
                onPanelReadyChanged: pageDelegate.askForThumbnail()
                onThumbnailChanged: pageDelegate.askForThumbnail()
                onClicked: root.notebook.currentPage = pageDelegate.index
                onPressAndHold: pageMenu.popup()

                DragHandler {
                    id: pageDrag

                    target: null
                    yAxis.enabled: true

                    onActiveChanged: {
                        if (active) {
                            root.draggedPage = pageDelegate.index;
                            pageDelegate.grabToImage(function (result) {
                                pageDelegate.Drag.imageSource = result.url;
                            });
                        } else {
                            pageDelegate.Drag.drop();
                            root.draggedPage = -1;
                        }
                    }
                }

                DropArea {
                    anchors.fill: parent

                    onDropped: {
                        if (root.draggedPage >= 0 && root.draggedPage !== pageDelegate.index) {
                            root.notebook.movePage(root.draggedPage, pageDelegate.index);
                        }
                    }
                }

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
                        objectName: "duplicatePageItem"
                        text: qsTr("Duplicate")

                        onTriggered: root.notebook.duplicatePage(pageDelegate.index)
                    }

                    MenuItem {
                        enabled: pageDelegate.index > 0
                        text: qsTr("Move up")

                        onTriggered: root.notebook.movePage(pageDelegate.index, pageDelegate.index - 1)
                    }

                    MenuItem {
                        enabled: root.ready && pageDelegate.index + 1 < root.notebook.pageCount
                        text: qsTr("Move down")

                        onTriggered: root.notebook.movePage(pageDelegate.index, pageDelegate.index + 1)
                    }

                    MenuItem {
                        enabled: root.ready && root.notebook.pageCount > 1
                        text: qsTr("Delete")

                        onTriggered: root.notebook.deletePage(pageDelegate.index)
                    }
                }
            }
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
