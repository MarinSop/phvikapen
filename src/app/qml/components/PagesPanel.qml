pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property AppActions actions
    property int landing: -1
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property bool ready: root.notebook !== null && root.notebook.loaded
    property int renaming: -1

    function askToDelete(index, title) {
        deleteDialog.index = index;
        deleteDialog.itemTitle = title;
        deleteDialog.open();
    }

    function rename(index, title) {
        if (root.renaming === index) {
            root.renaming = -1;
            root.notebook.renamePage(index, title.trim());
        }
    }

    background: null
    objectName: "pagesPanel"
    padding: Theme.gap

    TapHandler {
        onTapped: root.forceActiveFocus()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        ListView {
            id: pageList

            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            model: root.ready ? root.notebook.pages : null
            objectName: "pageList"

            displaced: Transition {
                NumberAnimation {
                    duration: Theme.quick
                    easing.type: Theme.ease
                    properties: "y"
                }
            }
            move: Transition {
                NumberAnimation {
                    duration: Theme.quick
                    easing.type: Theme.ease
                    properties: "y"
                }
            }
            delegate: ItemDelegate {
                id: row

                required property int index
                readonly property bool panelReady: root.ready
                readonly property bool renaming: root.renaming === row.index
                required property string thumbnail
                required property string title

                function askForThumbnail() {
                    if (root.ready && row.thumbnail === "") {
                        root.notebook.wantThumbnail(row.index);
                    }
                }

                height: 64
                highlighted: root.ready && row.index === root.notebook.currentPage
                rightPadding: 4
                width: pageList.width
                z: carry.active ? 2 : 1

                background: Rectangle {
                    color: row.highlighted ? Theme.base : row.hovered ? Theme.line : "transparent"
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
                            source: row.thumbnail
                        }
                    }

                    Label {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        text: row.title
                        verticalAlignment: Text.AlignVCenter
                        visible: !row.renaming
                    }

                    TextField {
                        id: name

                        Layout.fillWidth: true
                        objectName: "pageNameField"
                        text: row.title
                        visible: row.renaming

                        onAccepted: root.rename(row.index, name.text)
                        onActiveFocusChanged: {
                            if (name.visible && !name.activeFocus) {
                                root.rename(row.index, name.text);
                            }
                        }
                        onVisibleChanged: {
                            if (name.visible) {
                                name.selectAll();
                                name.forceActiveFocus();
                            }
                        }
                    }

                    QuickButton {
                        id: options

                        icon.source: Icons.settings
                        label: qsTr("Page options")
                        objectName: "pageOptionsButton"
                        opacity: row.hovered || row.highlighted ? 1 : 0

                        Behavior on opacity {
                            NumberAnimation {
                                duration: Theme.quick
                                easing.type: Theme.ease
                            }
                        }

                        onClicked: {
                            root.notebook.currentPage = row.index;
                            rowMenu.openAt(rowMenu.parent.mapFromItem(options, 0, options.height));
                        }
                    }

                    QuickButton {
                        enabled: root.ready && root.notebook.pageCount > 1
                        icon.source: Icons.close
                        label: qsTr("Delete page")
                        objectName: "deletePageButton"
                        opacity: row.hovered || row.highlighted ? 1 : 0

                        Behavior on opacity {
                            NumberAnimation {
                                duration: Theme.quick
                                easing.type: Theme.ease
                            }
                        }

                        onClicked: root.askToDelete(row.index, row.title)
                    }
                }

                Component.onCompleted: row.askForThumbnail()
                onPanelReadyChanged: row.askForThumbnail()
                onPressAndHold: rowMenu.popup()
                onThumbnailChanged: row.askForThumbnail()

                ReorderHandler {
                    id: carry

                    index: row.index
                    list: pageList
                    row: row

                    onLandingChanged: root.landing = carry.landing
                    onMoved: (from, to) => root.notebook.movePage(from, to)
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    gesturePolicy: TapHandler.DragThreshold

                    onPressedChanged: {
                        if (pressed) {
                            root.notebook.currentPage = row.index;
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton

                    onTapped: rowMenu.popup()
                }

                PageOptionsMenu {
                    id: rowMenu

                    actions: root.actions
                    index: row.index
                    pageTitle: row.title

                    onDeleteWanted: root.askToDelete(row.index, row.title)
                    onRenameWanted: root.renaming = row.index
                }
            }

            DropLine {
                list: pageList
                place: root.landing
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 2

            Item {
                Layout.fillWidth: true
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
    }

    EmptyPanelNote {
        anchors.centerIn: parent
        text: qsTr("Open a notebook to see its pages.")
        visible: !root.ready
        width: parent.width - (Theme.gap * 4)
    }

    ConfirmDialog {
        id: deleteDialog

        property int index: 0
        property string itemTitle: ""

        objectName: "deleteDialog"
        question: qsTr("Move “%1” to the deleted pages?").arg(deleteDialog.itemTitle)
        title: qsTr("Delete page")

        onAccepted: root.notebook.deletePage(deleteDialog.index)
    }
}
