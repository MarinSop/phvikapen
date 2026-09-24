pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

// Everything typed on the paper, drawn over the ink. Every box shows what it says; the one being
// worked on is typed in here, so only one editor is ever alive.
Item {
    id: root

    required property InkCanvas canvas
    required property NotebookViewModel notebook
    required property ToolViewModel tools
    // Which box the editor holds the words of, so that leaving one box for another puts what was
    // typed where it belongs.
    property string editingId: ""
    readonly property real grip: 18
    readonly property real onPage: Math.max(editor.implicitHeight, editor.font.pixelSize)
    readonly property point origin: root.canvas === null ? Qt.point(0, 0) : root.canvas.viewOrigin
    // A point of type, in the units a page is measured in.
    readonly property real pageUnitsPerPoint: 96 / 72
    readonly property var picked: root.notebook === null ? ({}) : root.notebook.pickedBox
    readonly property bool placing: root.tools !== null && root.tools.currentTool === ToolViewModel.Text
    readonly property string pickedId: root.picked.textId === undefined ? "" : root.picked.textId
    readonly property bool typing: root.pickedId !== ""
    readonly property real zoom: root.canvas === null ? 1 : root.canvas.zoom

    function columnPointOf(position) {
        return Qt.point((position.x / root.zoom) + root.origin.x, (position.y / root.zoom) + root.origin.y);
    }

    // What is in the editor becomes what the box says.
    function commit() {
        if (root.notebook !== null && root.editingId !== "") {
            root.notebook.finishText(root.editingId, editor.text, root.onPage);
        }
    }

    function leave() {
        root.commit();
        root.editingId = "";
        if (root.notebook !== null) {
            root.notebook.pickedText = "";
        }
    }

    anchors.fill: parent
    // Only the text tool reaches the paper through this layer; every other tool draws below it.
    enabled: (root.placing || root.typing) && root.notebook !== null && root.canvas !== null
    objectName: "textLayer"

    // Picking up another tool finishes the box that was being typed in.
    onPlacingChanged: {
        if (!root.placing && root.typing) {
            root.leave();
        }
    }
    onPickedIdChanged: {
        if (root.editingId !== "" && root.editingId !== root.pickedId) {
            root.commit();
        }
        root.editingId = root.pickedId;
        if (root.typing) {
            editor.text = root.picked.text;
            editor.forceActiveFocus();
        } else if (editor.activeFocus) {
            editor.focus = false;
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.placing || root.typing

        onPressed: mouse => {
            if (root.typing) {
                root.leave();
                return;
            }
            const at = root.columnPointOf(Qt.point(mouse.x, mouse.y));
            root.notebook.addTextAt(at.x, at.y, root.tools.textStyle);
        }
    }

    Repeater {
        model: root.notebook === null ? null : root.notebook.texts

        TextBox {
            notebook: root.notebook
            origin: root.origin
            zoom: root.zoom
        }
    }

    Item {
        id: slot

        height: root.onPage * root.zoom
        visible: root.typing
        width: (root.typing ? root.picked.boxWidth : 0) * root.zoom
        x: root.typing ? (root.picked.columnX - root.origin.x) * root.zoom : 0
        y: root.typing ? (root.picked.columnY - root.origin.y) * root.zoom : 0

        Item {
            height: root.onPage
            width: root.typing ? root.picked.boxWidth : 0

            transform: Scale {
                xScale: root.zoom
                yScale: root.zoom
            }

            TextEdit {
                id: editor

                color: root.typing ? root.picked.color : Theme.text
                font.bold: root.typing && root.picked.bold
                font.family: !root.typing || root.picked.font === "" ? AppInfo.plainFont : root.picked.font
                font.italic: root.typing && root.picked.italic
                font.pixelSize: Math.max(1, (root.typing ? root.picked.size : 12) * root.pageUnitsPerPoint)
                font.strikeout: root.typing && root.picked.struckOut
                font.underline: root.typing && root.picked.underline
                horizontalAlignment: [TextEdit.AlignLeft, TextEdit.AlignHCenter, TextEdit.AlignRight, TextEdit.AlignJustify][root.typing ? root.picked.align : 0]
                objectName: "textEditor"
                selectByMouse: true
                width: root.typing ? root.picked.boxWidth : 0
                wrapMode: TextEdit.Wrap

                Keys.onEscapePressed: root.leave()
            }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            border.color: Theme.accent
            border.width: 1
            color: "transparent"
            opacity: 0.8
        }

        // The bar above the box carries it about.
        Rectangle {
            id: bar

            property real lastX: 0
            property real lastY: 0

            anchors.bottom: parent.top
            anchors.bottomMargin: 4
            anchors.horizontalCenter: parent.horizontalCenter
            color: Theme.accent
            height: root.grip
            objectName: "textMoveBar"
            radius: 4
            width: Math.max(root.grip * 2, Math.min(parent.width, 64))

            DragHandler {
                id: moveDrag

                target: null

                onActiveChanged: {
                    if (moveDrag.active || !root.typing) {
                        return;
                    }
                    root.notebook.placeText(root.picked.textId, root.picked.columnX + (bar.lastX / root.zoom), root.picked.columnY + (bar.lastY / root.zoom), root.picked.boxWidth, root.onPage);
                }
                onTranslationChanged: {
                    if (moveDrag.active) {
                        bar.lastX = moveDrag.activeTranslation.x;
                        bar.lastY = moveDrag.activeTranslation.y;
                    }
                }
            }
        }

        // The grip on the right edge says how wide the writing may run.
        Rectangle {
            id: handle

            property real wanted: 0

            anchors.left: parent.right
            anchors.leftMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.accent
            height: root.grip
            objectName: "textWidthHandle"
            radius: 3
            width: 8

            DragHandler {
                id: wideDrag

                target: null
                xAxis.enabled: true
                yAxis.enabled: false

                onActiveChanged: {
                    if (wideDrag.active && root.typing) {
                        handle.wanted = root.picked.boxWidth;
                    }
                }
                onTranslationChanged: {
                    if (wideDrag.active && root.typing) {
                        root.notebook.placeText(root.picked.textId, root.picked.columnX, root.picked.columnY, handle.wanted + (wideDrag.activeTranslation.x / root.zoom), root.onPage);
                    }
                }
            }
        }

        Rectangle {
            anchors.bottom: parent.top
            anchors.bottomMargin: 4
            anchors.right: parent.right
            border.color: Theme.line
            border.width: 1
            color: Theme.surface
            height: root.grip
            objectName: "textRemove"
            radius: 4
            width: root.grip

            Image {
                anchors.centerIn: parent
                height: 12
                source: Icons.close
                width: 12
            }

            MouseArea {
                anchors.fill: parent

                onClicked: root.notebook.removeText(root.picked.textId)
            }
        }
    }
}
