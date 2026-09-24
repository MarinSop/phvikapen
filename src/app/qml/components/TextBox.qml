pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

// One box of typed text as it sits on the paper, laid out in the units of the page and drawn at
// the zoom of the canvas. The box being typed in is drawn by the layer instead.
Item {
    id: root

    required property int align
    required property bool bold
    required property real boxHeight
    required property real boxWidth
    required property color color
    required property real columnX
    required property real columnY
    required property string font
    required property bool italic
    required property NotebookViewModel notebook
    required property point origin
    required property real size
    required property bool struckOut
    required property string text
    required property string textId
    required property bool underline
    required property real zoom
    // A point of type, in the units a page is measured in.
    readonly property real pageUnitsPerPoint: 96 / 72
    readonly property bool picked: root.notebook !== null && root.notebook.pickedText === root.textId

    height: Math.max(label.implicitHeight, label.font.pixelSize, root.boxHeight) * root.zoom
    visible: !root.picked
    width: root.boxWidth * root.zoom
    x: (root.columnX - root.origin.x) * root.zoom
    y: (root.columnY - root.origin.y) * root.zoom

    Item {
        height: label.implicitHeight
        width: root.boxWidth

        transform: Scale {
            xScale: root.zoom
            yScale: root.zoom
        }

        Text {
            id: label

            color: root.color
            font.bold: root.bold
            font.family: root.font === "" ? AppInfo.plainFont : root.font
            font.italic: root.italic
            font.pixelSize: Math.max(1, root.size * root.pageUnitsPerPoint)
            font.strikeout: root.struckOut
            font.underline: root.underline
            horizontalAlignment: [Text.AlignLeft, Text.AlignHCenter, Text.AlignRight, Text.AlignJustify][root.align]
            objectName: "textLabel"
            text: root.text
            width: root.boxWidth
            wrapMode: Text.Wrap
        }
    }

    // A box takes a tap to pick it up and start typing in it.
    MouseArea {
        anchors.fill: parent
        objectName: "textPicker"

        onPressed: {
            if (root.notebook !== null) {
                root.notebook.pickedText = root.textId;
            }
        }
    }
}
