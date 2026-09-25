pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property AppActions actions
    readonly property string active: root.notebook === null ? "" : root.notebook.activeLayer
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property bool ready: root.notebook !== null && root.notebook.loaded
    readonly property real lineHeight: Math.round(40 * Theme.scale)
    property int landing: -1

    function placeOfDrop(index) {
        return Math.max(0, list.count - 1 - index);
    }

    background: null
    objectName: "layersPanel"
    padding: 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.gap
            Layout.rightMargin: Theme.gap
            Layout.topMargin: Theme.gap
            spacing: 2

            Item {
                Layout.fillWidth: true
            }

            QuickButton {
                action: root.actions.addLayer
                display: AbstractButton.IconOnly
                label: root.actions.addLayer.text
                objectName: "addLayerButton"
            }

            QuickButton {
                action: root.actions.duplicateLayer
                display: AbstractButton.IconOnly
                label: root.actions.duplicateLayer.text
                objectName: "duplicateLayerButton"
            }

            QuickButton {
                action: root.actions.removeLayer
                display: AbstractButton.IconOnly
                label: root.actions.removeLayer.text
                objectName: "removeLayerButton"
            }
        }

        ListView {
            id: list

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.topMargin: Theme.gap
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            currentIndex: -1
            keyNavigationEnabled: true
            model: root.ready ? root.notebook.layers : null
            objectName: "layerList"
            spacing: 2

            ScrollBar.vertical: ScrollBar {
            }
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
                id: line

                required property int count
                required property int index
                required property string layerId
                required property bool locked
                required property string name
                required property bool shown
                readonly property bool chosen: line.layerId === root.active
                property bool renaming: false

                Accessible.name: line.name
                height: root.lineHeight
                objectName: "layerLine" + line.index
                width: list.width
                z: carry.active ? 2 : 1

                background: Rectangle {
                    color: line.chosen ? Theme.accentSoft : line.hovered ? Theme.hover : "transparent"
                    radius: 6

                    Behavior on color {
                        ColorAnimation {
                            duration: Theme.quick
                            easing.type: Theme.ease
                        }
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.top: parent.top
                        color: Theme.accent
                        radius: 2
                        visible: line.chosen
                        width: 3
                    }
                }
                contentItem: RowLayout {
                    spacing: 2

                    ShapeButton {
                        active: !line.shown
                        icon.source: line.shown ? Icons.shown : Icons.hidden
                        label: line.shown ? qsTr("Hide this layer") : qsTr("Show this layer")
                        objectName: "layerShown" + line.index

                        onClicked: root.notebook.showLayer(line.layerId, !line.shown)
                    }

                    ShapeButton {
                        active: line.locked
                        icon.source: line.locked ? Icons.locked : Icons.unlocked
                        label: line.locked ? qsTr("Unlock this layer") : qsTr("Lock this layer")
                        objectName: "layerLocked" + line.index

                        onClicked: root.notebook.lockLayer(line.layerId, !line.locked)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Label {
                            Layout.fillWidth: true
                            color: line.shown ? palette.windowText : palette.placeholderText
                            elide: Text.ElideRight
                            font.italic: !line.shown
                            objectName: "layerName" + line.index
                            text: line.name
                            visible: !line.renaming
                        }

                        TextField {
                            id: rename

                            Layout.fillWidth: true
                            objectName: "layerRename" + line.index
                            text: line.name
                            visible: line.renaming

                            Keys.onEscapePressed: line.renaming = false
                            onAccepted: {
                                root.notebook.renameLayer(line.layerId, rename.text);
                                line.renaming = false;
                            }
                            onActiveFocusChanged: {
                                if (!rename.activeFocus) {
                                    line.renaming = false;
                                }
                            }
                            onVisibleChanged: {
                                if (rename.visible) {
                                    rename.forceActiveFocus();
                                    rename.selectAll();
                                }
                            }
                        }

                        Label {
                            color: palette.placeholderText
                            font.pixelSize: Math.round(11 * Theme.scale)
                            text: line.locked ? qsTr("%n thing(s), locked", "", line.count) : qsTr("%n thing(s)", "", line.count)
                            visible: !line.renaming
                        }
                    }
                }

                onClicked: root.notebook.activeLayer = line.layerId
                onDoubleClicked: line.renaming = true
                onPressAndHold: lineMenu.popup()

                ReorderHandler {
                    id: carry

                    index: line.index
                    list: list
                    row: line

                    onLandingChanged: root.landing = carry.landing
                    onMoved: (from, to) => root.notebook.moveLayer(line.layerId, root.placeOfDrop(to))
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton

                    onTapped: lineMenu.popup()
                }

                Menu {
                    id: lineMenu

                    MenuItem {
                        text: qsTr("Rename")

                        onTriggered: line.renaming = true
                    }

                    MenuItem {
                        objectName: "duplicateLayerItem"
                        text: qsTr("Duplicate")

                        onTriggered: root.notebook.duplicateLayer(line.layerId)
                    }

                    MenuItem {
                        enabled: line.index > 0
                        text: qsTr("Move up")

                        onTriggered: root.notebook.moveLayer(line.layerId, root.placeOfDrop(line.index - 1))
                    }

                    MenuItem {
                        enabled: line.index + 1 < list.count
                        text: qsTr("Move down")

                        onTriggered: root.notebook.moveLayer(line.layerId, root.placeOfDrop(line.index + 1))
                    }

                    MenuSeparator {
                    }

                    MenuItem {
                        action: root.actions.moveToActiveLayer
                    }

                    MenuItem {
                        enabled: list.count > 1
                        text: qsTr("Delete…")

                        onTriggered: removeDialog.open()
                    }
                }

                ConfirmDialog {
                    id: removeDialog

                    objectName: "removeLayerDialog" + line.index
                    question: qsTr("“%1” and everything on it will go. This can be undone.").arg(line.name)
                    title: qsTr("Delete layer")

                    onAccepted: root.notebook.removeLayer(line.layerId)
                }
            }

            DropLine {
                list: list
                place: root.landing
            }
        }

        Button {
            Layout.bottomMargin: Theme.gap
            Layout.fillWidth: true
            Layout.leftMargin: Theme.gap
            Layout.rightMargin: Theme.gap
            Layout.topMargin: Theme.gap
            ToolTip.delay: 600
            ToolTip.text: qsTr("Send what is picked up to the layer that is drawn in.")
            ToolTip.visible: hovered
            action: root.actions.moveToActiveLayer
            objectName: "moveToLayerButton"
        }
    }

    EmptyPanelNote {
        anchors.centerIn: parent
        text: qsTr("Open a page to work on its layers.")
        visible: !root.ready
        width: parent.width - (Theme.gap * 4)
    }
}
