pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property AppActions actions
    property int draggedPage: -1
    property int draggedSection: -1
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property bool ready: root.notebook !== null && root.notebook.loaded
    property int renamingPage: -1
    property int renamingSection: -1
    readonly property SettingsViewModel settings: root.actions.settings

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

    SplitView {
        id: split

        anchors.fill: parent
        orientation: Qt.Vertical

        handle: Item {
            id: listHandle

            implicitHeight: 7

            Rectangle {
                anchors.centerIn: parent
                color: listHandle.SplitHandle.pressed || listHandle.SplitHandle.hovered ? Theme.accent : Theme.line
                height: listHandle.SplitHandle.pressed || listHandle.SplitHandle.hovered ? 3 : 1
                width: parent.width
            }
        }

        ColumnLayout {
            id: sectionsPart

            SplitView.minimumHeight: 80
            SplitView.preferredHeight: root.settings.sectionsHeight
            spacing: 4
            visible: root.settings.showSections

            // Only what the reader drags is kept: laying out must not rewrite it.
            onHeightChanged: {
                if (split.resizing) {
                    root.settings.sectionsHeight = sectionsPart.height;
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Label {
                    Layout.fillWidth: true
                    font.bold: true
                    text: qsTr("Sections")
                }

                QuickButton {
                    Layout.rightMargin: 4
                    action: root.actions.addSection
                    display: AbstractButton.IconOnly
                    icon.source: Icons.plus
                    label: qsTr("New section")
                    objectName: "addSectionButton"
                }
            }

            ListView {
                id: sectionList

                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true
                model: root.ready ? root.notebook.sections : null

                delegate: ItemDelegate {
                    id: sectionDelegate

                    required property int index
                    readonly property bool renaming: root.renamingSection === sectionDelegate.index
                    required property string title

                    Drag.active: sectionDrag.active
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2
                    Drag.source: sectionDelegate
                    highlighted: root.ready && sectionDelegate.index === root.notebook.currentSection
                    rightPadding: 4
                    width: sectionList.width
                    z: sectionDrag.active ? 2 : 1

                    contentItem: RowLayout {
                        spacing: 4

                        Label {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            text: sectionDelegate.title
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

                        QuickButton {
                            enabled: root.ready && root.notebook.sectionCount > 1
                            icon.source: Icons.close
                            label: qsTr("Delete section")
                            objectName: "deleteSectionButton"
                            // Always there, so the row does not jump when the pointer crosses it.
                            opacity: sectionDelegate.hovered || sectionDelegate.highlighted ? 1 : 0

                            onClicked: root.askToDelete(true, sectionDelegate.index, sectionDelegate.title)
                        }
                    }

                    onClicked: root.notebook.currentSection = sectionDelegate.index
                    onPressAndHold: sectionMenu.popup()

                    DragHandler {
                        id: sectionDrag

                        target: null
                        yAxis.enabled: true

                        onActiveChanged: {
                            if (active) {
                                root.draggedSection = sectionDelegate.index;
                                sectionDelegate.grabToImage(function (result) {
                                    sectionDelegate.Drag.imageSource = result.url;
                                });
                            } else {
                                sectionDelegate.Drag.drop();
                                root.draggedSection = -1;
                            }
                        }
                    }

                    DropArea {
                        anchors.fill: parent

                        onDropped: {
                            if (root.draggedSection >= 0 && root.draggedSection !== sectionDelegate.index) {
                                root.notebook.moveSection(root.draggedSection, sectionDelegate.index);
                            }
                        }
                    }

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
        }

        ColumnLayout {
            id: pagesPart

            SplitView.fillHeight: true
            SplitView.minimumHeight: 80
            spacing: 4
            visible: root.settings.showPages

            RowLayout {
                Layout.fillWidth: true

                Label {
                    Layout.fillWidth: true
                    font.bold: true
                    text: qsTr("Pages")
                }

                QuickButton {
                    Layout.rightMargin: 4
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
                    rightPadding: 4
                    width: pageList.width
                    z: pageDrag.active ? 2 : 1

                    contentItem: RowLayout {
                        spacing: 8

                        Rectangle {
                            Layout.preferredHeight: 56
                            Layout.preferredWidth: 44
                            border.color: palette.mid
                            border.width: 1
                            color: "white"

                            Image {
                                anchors.fill: parent
                                anchors.margins: 1
                                cache: false
                                fillMode: Image.PreserveAspectFit
                                source: pageDelegate.thumbnail
                            }
                        }

                        Label {
                            Layout.fillHeight: true
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

                        QuickButton {
                            enabled: root.ready && root.notebook.pageCount > 1
                            icon.source: Icons.close
                            label: qsTr("Delete page")
                            objectName: "deletePageButton"
                            opacity: pageDelegate.hovered || pageDelegate.highlighted ? 1 : 0

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
    }

    ConfirmDialog {
        id: deleteDialog

        property int index: 0
        property string itemTitle: ""
        property bool sectionScope: true

        objectName: "deleteDialog"
        question: deleteDialog.sectionScope ? qsTr("Move “%1” and every page in it to the deleted pages?").arg(deleteDialog.itemTitle) : qsTr("Move “%1” to the deleted pages?").arg(deleteDialog.itemTitle)
        title: deleteDialog.sectionScope ? qsTr("Delete section") : qsTr("Delete page")

        onAccepted: {
            if (deleteDialog.sectionScope) {
                root.notebook.deleteSection(deleteDialog.index);
            } else {
                root.notebook.deletePage(deleteDialog.index);
            }
        }
    }
}
