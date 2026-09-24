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
    // What the reader is doing to the box right now, before it is written down: how far it has
    // been carried, in pixels of the window, and how much wider and taller, in units of the page.
    property real liveX: 0
    property real liveY: 0
    property real liveWide: 0
    property real liveTall: 0
    readonly property int gripSize: Math.round(9 * Theme.scale)
    readonly property real narrowest: 24
    readonly property real heldHeight: root.typing ? root.picked.boxHeight : 0
    // A box is as tall as what is typed in it, or as tall as it was pulled, whichever is more.
    readonly property real onPage: Math.max(editor.implicitHeight, editor.font.pixelSize, root.heldHeight + root.liveTall)
    readonly property real onPageWide: Math.max(root.narrowest, (root.typing ? root.picked.boxWidth : 0) + root.liveWide)
    readonly property point origin: root.canvas === null ? Qt.point(0, 0) : root.canvas.viewOrigin
    // A point of type, in the units a page is measured in.
    readonly property real pageUnitsPerPoint: 96 / 72
    readonly property var picked: root.notebook === null ? ({}) : root.notebook.pickedBox
    readonly property bool placing: root.tools !== null && root.tools.currentTool === ToolViewModel.Text
    // The pick tool takes hold of a box of words as well, so that one can be opened again and
    // corrected without first reaching for the text tool.
    readonly property bool picking: root.tools !== null && root.tools.currentTool === ToolViewModel.Selection
    readonly property bool reachable: root.placing || root.picking
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
        root.liveX = 0;
        root.liveY = 0;
        root.liveWide = 0;
        root.liveTall = 0;
        root.editingId = "";
        if (root.notebook !== null) {
            root.notebook.pickedText = "";
        }
    }

    // A box is put down and made ready to type in. Reaching for the text tool puts one where the
    // reader is looking, a little in from the top left of what is on the screen, so that typing
    // can start at once; tapping elsewhere gives that one up, since a box nobody typed in is
    // never kept.
    function putABoxAt(x, y) {
        const at = root.columnPointOf(Qt.point(x, y));
        // Every new box starts plain; the face of the last one stays with the last one.
        root.tools.resetTextStyle();
        root.notebook.addTextAt(at.x, at.y, root.tools.textStyle);
    }

    // Where the box has been carried and pulled to becomes where it is.
    function settle() {
        if (!root.typing) {
            return;
        }
        root.notebook.placeText(root.picked.textId, root.picked.columnX + (root.liveX / root.zoom), root.picked.columnY + (root.liveY / root.zoom), root.onPageWide, root.onPage);
        root.liveX = 0;
        root.liveY = 0;
        root.liveWide = 0;
        root.liveTall = 0;
    }

    anchors.fill: parent
    // Only the text tool reaches the paper through this layer; every other tool draws below it.
    enabled: (root.reachable || root.typing) && root.notebook !== null && root.canvas !== null
    objectName: "textLayer"

    // Picking up a tool that has no business with words finishes the box being typed in.
    onReachableChanged: {
        if (!root.reachable && root.typing) {
            root.leave();
        }
    }
    // Reaching for the text tool is enough: a box is waiting with the caret in it.
    onPlacingChanged: {
        if (root.placing && !root.typing && root.notebook !== null && root.canvas !== null && root.width > 0) {
            root.putABoxAt(root.width * 0.3, root.height * 0.4);
        }
    }
    onPickedIdChanged: {
        if (root.editingId !== "" && root.editingId !== root.pickedId) {
            root.commit();
        }
        root.editingId = root.pickedId;
        // The editor is one and the same for every box, so what it holds is always replaced,
        // never left over from the box before.
        editor.text = root.typing ? root.picked.text : "";
        if (root.typing) {
            editor.forceActiveFocus();
        } else if (editor.activeFocus) {
            editor.focus = false;
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.placing || root.typing
        objectName: "textPlacer"

        onPressed: mouse => {
            if (root.typing) {
                root.leave();
                return;
            }
            root.putABoxAt(mouse.x, mouse.y);
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
        width: root.onPageWide * root.zoom
        x: (root.typing ? (root.picked.columnX - root.origin.x) * root.zoom : 0) + root.liveX
        y: (root.typing ? (root.picked.columnY - root.origin.y) * root.zoom : 0) + root.liveY

        Item {
            height: root.onPage
            width: root.onPageWide

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
                width: root.onPageWide
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
            objectName: "textFrame"
        }

        // The knob above the box carries it about, and follows the pointer as it goes.
        Rectangle {
            antialiasing: true
            border.color: Theme.accent
            border.width: 1
            color: Theme.accentText
            height: root.gripSize + 2
            objectName: "textMoveBar"
            radius: height / 2
            width: root.gripSize + 2
            x: (parent.width / 2) - (width / 2)
            y: -(root.gripSize * 2) - 2

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.bottom
                color: Theme.accent
                height: root.gripSize
                opacity: 0.7
                width: 1
            }

            DragHandler {
                id: moveDrag

                target: null

                onActiveChanged: {
                    if (!moveDrag.active) {
                        root.settle();
                    }
                }
                onTranslationChanged: {
                    if (moveDrag.active && root.typing) {
                        root.liveX = moveDrag.activeTranslation.x;
                        root.liveY = moveDrag.activeTranslation.y;
                    }
                }
            }
        }

        // Three grips, as the frame around picked ink has: one for how wide the writing may run,
        // one for how tall the box stands, and one for both at once.
        Repeater {
            model: 3

            Rectangle {
                id: grip

                required property int index
                readonly property bool pullsWide: grip.index !== 1
                readonly property bool pullsTall: grip.index !== 0

                antialiasing: true
                border.color: Theme.accent
                border.width: 1
                color: Theme.accentText
                height: root.gripSize
                objectName: ["textWidthHandle", "textHeightHandle", "textCornerHandle"][grip.index]
                radius: 2
                width: root.gripSize
                x: (grip.pullsWide ? slot.width : slot.width / 2) - (width / 2)
                y: (grip.pullsTall ? slot.height : slot.height / 2) - (height / 2)

                DragHandler {
                    id: sizeDrag

                    target: null

                    onActiveChanged: {
                        if (!sizeDrag.active) {
                            root.settle();
                        }
                    }
                    onTranslationChanged: {
                        if (!sizeDrag.active || !root.typing) {
                            return;
                        }
                        root.liveWide = grip.pullsWide ? sizeDrag.activeTranslation.x / root.zoom : 0;
                        root.liveTall = grip.pullsTall ? sizeDrag.activeTranslation.y / root.zoom : 0;
                    }
                }
            }
        }

        // Taking the box away, beside the knob that carries it.
        Rectangle {
            antialiasing: true
            border.color: Theme.accent
            border.width: 1
            color: Theme.accentText
            height: root.gripSize + 2
            objectName: "textRemove"
            radius: height / 2
            width: root.gripSize + 2
            x: parent.width + 6
            y: -(root.gripSize * 2) - 2

            Text {
                anchors.centerIn: parent
                color: Theme.accent
                font.pixelSize: Math.round(parent.height * 0.62)
                text: "\u2715"
            }

            TapHandler {
                onTapped: root.notebook.removeText(root.picked.textId)
            }
        }
    }
}
