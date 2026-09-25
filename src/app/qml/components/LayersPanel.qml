pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

// The layers of the page being read, top first, the way a panel of layers is read everywhere. Each
// line says whether its layer is shown, whether it is locked, what it is called and how much
// stands on it; the line that is drawn in is the one anything new is put on. A line dragged up or
// down changes the order the layers are drawn in.
Item {
    id: root

    required property AppActions actions
    readonly property string active: root.notebook === null ? "" : root.notebook.activeLayer
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property real lineHeight: Math.round(38 * Theme.scale)

    // Where a line dropped at a height belongs in the order the layers are drawn in, which runs
    // the other way round from the way they are listed.
    function placeOfDrop(index) {
        return Math.max(0, list.count - 1 - index);
    }

    objectName: "layersPanel"

    Rectangle {
        anchors.fill: parent
        color: Theme.surface

        Rectangle {
            anchors.left: parent.left
            color: Theme.line
            height: parent.height
            width: 1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 1
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 4
            Layout.topMargin: 6
            spacing: 2

            Label {
                Layout.fillWidth: true
                color: palette.placeholderText
                text: qsTr("Layers")
            }

            QuickButton {
                action: root.actions.addLayer
                label: qsTr("New layer")
                objectName: "addLayerButton"
            }

            QuickButton {
                action: root.actions.duplicateLayer
                label: qsTr("Duplicate layer")
                objectName: "duplicateLayerButton"
            }

            QuickButton {
                action: root.actions.removeLayer
                label: qsTr("Delete layer")
                objectName: "removeLayerButton"
            }
        }

        ListView {
            id: list

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.topMargin: 6
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            model: root.notebook === null ? null : root.notebook.layers
            objectName: "layerList"
            spacing: 2

            ScrollBar.vertical: ScrollBar {
            }
            delegate: Rectangle {
                id: line

                required property int count
                required property int index
                required property string layerId
                required property bool locked
                required property string name
                required property bool shown
                readonly property bool chosen: line.layerId === root.active

                color: line.chosen ? Theme.accentSoft : (hover.hovered ? Theme.base : "transparent")
                height: root.lineHeight
                objectName: "layerLine" + line.index
                radius: 6
                width: list.width

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.top: parent.top
                    color: Theme.accent
                    radius: 2
                    visible: line.chosen
                    width: 3
                }

                HoverHandler {
                    id: hover
                }

                TapHandler {
                    onTapped: root.notebook.activeLayer = line.layerId
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 4
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
                            id: label

                            Layout.fillWidth: true
                            color: line.shown ? palette.windowText : palette.placeholderText
                            elide: Text.ElideRight
                            objectName: "layerName" + line.index
                            text: line.name
                            visible: !rename.visible

                            TapHandler {
                                onDoubleTapped: {
                                    rename.text = line.name;
                                    rename.visible = true;
                                    rename.forceActiveFocus();
                                    rename.selectAll();
                                }
                            }
                        }

                        TextField {
                            id: rename

                            Layout.fillWidth: true
                            objectName: "layerRename" + line.index
                            visible: false

                            Keys.onEscapePressed: rename.visible = false
                            onAccepted: {
                                root.notebook.renameLayer(line.layerId, rename.text);
                                rename.visible = false;
                            }
                            onActiveFocusChanged: {
                                if (!rename.activeFocus) {
                                    rename.visible = false;
                                }
                            }
                        }

                        Label {
                            color: palette.placeholderText
                            font.pixelSize: Math.round(11 * Theme.scale)
                            text: line.count === 1 ? qsTr("1 thing") : qsTr("%1 things").arg(line.count)
                            visible: !rename.visible
                        }
                    }

                    // The grip the line is dragged by, so that dragging anywhere else in it is
                    // free to choose the layer rather than reorder the lot.
                    Rectangle {
                        id: grip

                        Layout.preferredHeight: root.lineHeight - 12
                        Layout.preferredWidth: 14
                        color: "transparent"
                        objectName: "layerGrip" + line.index

                        Column {
                            anchors.centerIn: parent
                            spacing: 3

                            Repeater {
                                model: 3

                                Rectangle {
                                    color: dragging.active ? Theme.accent : Theme.line
                                    height: 2
                                    radius: 1
                                    width: 12
                                }
                            }
                        }

                        HoverHandler {
                            cursorShape: Qt.SizeVerCursor
                        }

                        DragHandler {
                            id: dragging

                            target: null
                            xAxis.enabled: false

                            onActiveChanged: {
                                if (dragging.active) {
                                    return;
                                }
                                const steps = Math.round(dragging.activeTranslation.y / root.lineHeight);
                                if (steps !== 0) {
                                    root.notebook.moveLayer(line.layerId, root.placeOfDrop(line.index + steps));
                                }
                            }
                        }
                    }
                }
            }
        }

        // What is picked up can be sent to another layer, which is how a picture is put over a
        // table or under it.
        Button {
            Layout.bottomMargin: 6
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            enabled: root.actions.hasPicture || root.actions.hasTable || root.actions.hasTextBox
            objectName: "moveToLayerButton"
            text: qsTr("Move to This Layer")

            onClicked: root.notebook.movePickedToLayer(root.active)
        }
    }
}
