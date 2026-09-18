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
    property int renamingPage: -1
    property int renamingSection: -1

    function askToDelete(sectionScope, index, title) {
        deleteDialog.sectionScope = sectionScope;
        deleteDialog.index = index;
        deleteDialog.itemTitle = title;
        deleteDialog.open();
    }

    function renamePage(index, title) {
        if (root.renamingPage === index) {
            root.renamingPage = -1;
            root.notebook.renamePage(index, title.trim());
        }
    }

    function renameSection(index, title) {
        if (root.renamingSection === index) {
            root.renamingSection = -1;
            root.notebook.renameSection(index, title.trim());
        }
    }

    objectName: "pagesPanel"
    padding: 8

    background: Rectangle {
        color: Theme.surface

        Rectangle {
            anchors.right: parent.right
            color: Theme.line
            height: parent.height
            width: 1
        }
    }

    Connections {
        function onPageAdded(index) {
            root.renamingSection = -1;
            root.renamingPage = index;
        }

        function onSectionAdded(index) {
            root.renamingPage = -1;
            root.renamingSection = index;
        }

        target: root.notebook
    }

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
                readonly property bool renaming: root.renamingSection === sectionDelegate.index
                required property string title

                highlighted: root.ready && sectionDelegate.index === root.notebook.currentSection
                width: sectionList.width

                contentItem: RowLayout {
                    spacing: 4

                    Label {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        text: sectionDelegate.title + " (" + sectionDelegate.count + ")"
                        verticalAlignment: Text.AlignVCenter
                        visible: !sectionDelegate.renaming
                    }

                    TextField {
                        id: sectionName

                        Layout.fillWidth: true
                        objectName: "sectionNameField"
                        text: sectionDelegate.title
                        visible: sectionDelegate.renaming

                        onAccepted: root.renameSection(sectionDelegate.index, sectionName.text)
                        onActiveFocusChanged: {
                            if (sectionName.visible && !sectionName.activeFocus) {
                                root.renameSection(sectionDelegate.index, sectionName.text);
                            }
                        }
                        onVisibleChanged: {
                            if (sectionName.visible) {
                                sectionName.selectAll();
                                sectionName.forceActiveFocus();
                            }
                        }
                    }

                    ToolButton {
                        display: AbstractButton.IconOnly
                        enabled: root.ready && root.notebook.sectionCount > 1
                        icon.color: enabled ? palette.buttonText : palette.placeholderText
                        icon.height: 24
                        icon.source: Icons.close
                        icon.width: 24
                        implicitHeight: 34
                        implicitWidth: 34
                        objectName: "deleteSectionButton"
                        ToolTip.delay: 600
                        ToolTip.text: qsTr("Delete section")
                        ToolTip.visible: hovered
                        visible: sectionDelegate.hovered || sectionDelegate.highlighted

                        onClicked: root.askToDelete(true, sectionDelegate.index, sectionDelegate.title)
                    }
                }

                onClicked: root.notebook.currentSection = sectionDelegate.index
                onPressAndHold: sectionMenu.popup()

                TapHandler {
                    acceptedButtons: Qt.RightButton

                    onTapped: sectionMenu.popup()
                }

                Menu {
                    id: sectionMenu

                    MenuItem {
                        text: qsTr("Rename")

                        onTriggered: root.renamingSection = sectionDelegate.index
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
                        text: qsTr("Delete…")

                        onTriggered: root.askToDelete(true, sectionDelegate.index, sectionDelegate.title)
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
                readonly property bool renaming: root.renamingPage === pageDelegate.index
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
                        visible: !pageDelegate.renaming
                    }

                    TextField {
                        id: pageName

                        Layout.fillWidth: true
                        objectName: "pageNameField"
                        text: pageDelegate.title
                        visible: pageDelegate.renaming

                        onAccepted: root.renamePage(pageDelegate.index, pageName.text)
                        onActiveFocusChanged: {
                            if (pageName.visible && !pageName.activeFocus) {
                                root.renamePage(pageDelegate.index, pageName.text);
                            }
                        }
                        onVisibleChanged: {
                            if (pageName.visible) {
                                pageName.selectAll();
                                pageName.forceActiveFocus();
                            }
                        }
                    }

                    ToolButton {
                        display: AbstractButton.IconOnly
                        enabled: root.ready && root.notebook.pageCount > 1
                        icon.color: enabled ? palette.buttonText : palette.placeholderText
                        icon.height: 24
                        icon.source: Icons.close
                        icon.width: 24
                        implicitHeight: 34
                        implicitWidth: 34
                        objectName: "deletePageButton"
                        ToolTip.delay: 600
                        ToolTip.text: qsTr("Delete page")
                        ToolTip.visible: hovered
                        visible: pageDelegate.hovered || pageDelegate.highlighted

                        onClicked: root.askToDelete(false, pageDelegate.index, pageDelegate.title)
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
                        text: qsTr("Rename")

                        onTriggered: root.renamingPage = pageDelegate.index
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
                        text: qsTr("Delete…")

                        onTriggered: root.askToDelete(false, pageDelegate.index, pageDelegate.title)
                    }
                }
            }
        }
    }

    AppDialog {
        id: deleteDialog

        property int index: 0
        property string itemTitle: ""
        property bool sectionScope: true

        objectName: "deleteDialog"
        standardButtons: Dialog.Ok | Dialog.Cancel
        title: deleteDialog.sectionScope ? qsTr("Delete section") : qsTr("Delete page")
        width: 380

        onAccepted: {
            if (deleteDialog.sectionScope) {
                root.notebook.deleteSection(deleteDialog.index);
            } else {
                root.notebook.deletePage(deleteDialog.index);
            }
        }

        Label {
            anchors.fill: parent
            text: deleteDialog.sectionScope ? qsTr("Move “%1” and every page in it to the deleted pages?").arg(deleteDialog.itemTitle) : qsTr("Move “%1” to the deleted pages?").arg(deleteDialog.itemTitle)
            wrapMode: Text.WordWrap
        }
    }
}
