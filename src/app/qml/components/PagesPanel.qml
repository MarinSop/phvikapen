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
    // Where the row in hand would land, so that a line can show it.
    property int pageLanding: -1
    property int sectionLanding: -1
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property bool ready: root.notebook !== null && root.notebook.loaded
    property int renamingPage: -1
    property int renamingSection: -1
    readonly property SettingsViewModel settings: root.actions.settings

    // The gap a carried row would drop into: 0 is above the first row, count is under the last.
    // It is worked out from where the rows rest, so the row in hand does not move the answer.
    function gapIn(list, scenePosition) {
        if (list.count === 0) {
            return 0;
        }
        const step = list.contentHeight / list.count;
        const point = list.mapFromItem(null, scenePosition);
        const inContent = point.y + list.contentY;
        return Math.max(0, Math.min(list.count, Math.round(inContent / step)));
    }

    // Where a row taken from `from` ends up when it is dropped into `gap`.
    function landingOf(from, gap) {
        return gap > from ? gap - 1 : gap;
    }

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

    // Clicking anywhere else in the panel settles a name that is being typed.
    TapHandler {
        onTapped: root.forceActiveFocus()
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

                objectName: "sectionList"
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true
                model: root.ready ? root.notebook.sections : null

                delegate: ItemDelegate {
                    id: sectionDelegate

                    required property int index
                    readonly property bool renaming: root.renamingSection === sectionDelegate.index
                    property real restingY: 0
                    required property string title

                    highlighted: root.ready && sectionDelegate.index === root.notebook.currentSection
                    rightPadding: 4
                    width: sectionList.width
                    z: sectionDrag.active ? 2 : 1

                    background: Rectangle {
                        color: sectionDelegate.highlighted ? Theme.base : sectionDelegate.hovered ? Theme.line : "transparent"
                        radius: 6
                    }
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

                        grabPermissions: PointerHandler.CanTakeOverFromAnything
                        target: sectionDelegate
                        xAxis.enabled: false
                        yAxis.enabled: true

                        onCentroidChanged: {
                            if (sectionDrag.active) {
                                root.sectionLanding = root.gapIn(sectionList, sectionDrag.centroid.scenePosition);
                            }
                        }
                        onActiveChanged: {
                            if (sectionDrag.active) {
                                sectionDelegate.restingY = sectionDelegate.y;
                                root.draggedSection = sectionDelegate.index;
                                root.sectionLanding = sectionDelegate.index;
                                return;
                            }
                            const carried = root.draggedSection;
                            const gap = root.sectionLanding;
                            root.draggedSection = -1;
                            root.sectionLanding = -1;
                            sectionDelegate.y = sectionDelegate.restingY;
                            sectionList.forceLayout();
                            const landed = root.landingOf(carried, gap);
                            if (carried >= 0 && landed !== carried) {
                                root.notebook.moveSection(carried, landed);
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

                DropLine {
                    list: sectionList
                    place: root.sectionLanding
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

                objectName: "pageList"
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true
                model: root.ready ? root.notebook.pages : null

                delegate: ItemDelegate {
                    id: pageDelegate

                    required property int index
                    readonly property bool panelReady: root.ready
                    property real restingY: 0
                    readonly property bool renaming: root.renamingPage === pageDelegate.index
                    required property string thumbnail
                    required property string title

                    function askForThumbnail() {
                        if (root.ready && pageDelegate.thumbnail === "") {
                            root.notebook.wantThumbnail(pageDelegate.index);
                        }
                    }

                    height: 64
                    highlighted: root.ready && pageDelegate.index === root.notebook.currentPage
                    rightPadding: 4
                    width: pageList.width
                    z: pageDrag.active ? 2 : 1

                    background: Rectangle {
                        color: pageDelegate.highlighted ? Theme.base : pageDelegate.hovered ? Theme.line : "transparent"
                        radius: 6
                    }
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

                    // Picking a page up and carrying it: the row follows the pointer and the
                    // page lands where it is let go.
                    DragHandler {
                        id: pageDrag

                        // The list would rather scroll; carrying a page has to win that argument.
                        grabPermissions: PointerHandler.CanTakeOverFromAnything
                        target: pageDelegate
                        xAxis.enabled: false
                        yAxis.enabled: true

                        onCentroidChanged: {
                            if (pageDrag.active) {
                                root.pageLanding = root.gapIn(pageList, pageDrag.centroid.scenePosition);
                            }
                        }
                        onActiveChanged: {
                            if (pageDrag.active) {
                                pageDelegate.restingY = pageDelegate.y;
                                root.draggedPage = pageDelegate.index;
                                root.pageLanding = pageDelegate.index;
                                return;
                            }
                            const carried = root.draggedPage;
                            const gap = root.pageLanding;
                            root.draggedPage = -1;
                            root.pageLanding = -1;
                            // The carried row goes back in line; the list puts it where it belongs.
                            pageDelegate.y = pageDelegate.restingY;
                            pageList.forceLayout();
                            const landed = root.landingOf(carried, gap);
                            if (carried >= 0 && landed !== carried) {
                                root.notebook.movePage(carried, landed);
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

                DropLine {
                    list: pageList
                    place: root.pageLanding
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
