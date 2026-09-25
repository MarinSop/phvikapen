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

    function rename(index, title) {
        if (root.renaming === index) {
            root.renaming = -1;
            root.notebook.renameSection(index, title.trim());
        }
    }

    background: null
    objectName: "sectionsPanel"
    padding: Theme.gap

    TapHandler {
        onTapped: root.forceActiveFocus()
    }

    Connections {
        function onSectionAdded(index) {
            root.renaming = index;
        }

        target: root.notebook
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        RowLayout {
            Layout.fillWidth: true

            Item {
                Layout.fillWidth: true
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
            objectName: "sectionList"

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
                readonly property bool renaming: root.renaming === row.index
                required property string title

                highlighted: root.ready && row.index === root.notebook.currentSection
                rightPadding: 4
                width: sectionList.width
                z: carry.active ? 2 : 1

                background: Rectangle {
                    color: row.highlighted ? Theme.base : row.hovered ? Theme.line : "transparent"
                    radius: 6
                }
                contentItem: RowLayout {
                    spacing: 4

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
                        objectName: "sectionNameField"
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
                        enabled: root.ready && root.notebook.sectionCount > 1
                        icon.source: Icons.close
                        label: qsTr("Delete section")
                        objectName: "deleteSectionButton"
                        opacity: row.hovered || row.highlighted ? 1 : 0

                        onClicked: root.askToDelete(row.index, row.title)

                        Behavior on opacity {
                            NumberAnimation {
                                duration: Theme.quick
                                easing.type: Theme.ease
                            }
                        }
                    }
                }

                onClicked: root.notebook.currentSection = row.index
                onPressAndHold: rowMenu.popup()

                ReorderHandler {
                    id: carry

                    index: row.index
                    list: sectionList
                    row: row

                    onLandingChanged: root.landing = carry.landing
                    onMoved: (from, to) => root.notebook.moveSection(from, to)
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton

                    onTapped: rowMenu.popup()
                }

                Menu {
                    id: rowMenu

                    MenuItem {
                        text: qsTr("Rename")

                        onTriggered: root.renaming = row.index
                    }

                    MenuItem {
                        enabled: row.index > 0
                        text: qsTr("Move up")

                        onTriggered: root.notebook.moveSection(row.index, row.index - 1)
                    }

                    MenuItem {
                        enabled: root.ready && row.index + 1 < root.notebook.sectionCount
                        text: qsTr("Move down")

                        onTriggered: root.notebook.moveSection(row.index, row.index + 1)
                    }

                    MenuItem {
                        enabled: root.ready && root.notebook.sectionCount > 1
                        text: qsTr("Delete…")

                        onTriggered: root.askToDelete(row.index, row.title)
                    }
                }
            }

            DropLine {
                list: sectionList
                place: root.landing
            }
        }
    }

    EmptyPanelNote {
        anchors.centerIn: parent
        text: qsTr("Open a notebook to see its sections.")
        visible: !root.ready
        width: parent.width - (Theme.gap * 4)
    }

    function askToDelete(index, title) {
        deleteDialog.index = index;
        deleteDialog.itemTitle = title;
        deleteDialog.open();
    }

    ConfirmDialog {
        id: deleteDialog

        property int index: 0
        property string itemTitle: ""

        objectName: "deleteSectionDialog"
        question: qsTr("Move “%1” and every page in it to the deleted pages?").arg(deleteDialog.itemTitle)
        title: qsTr("Delete section")

        onAccepted: root.notebook.deleteSection(deleteDialog.index)
    }
}
