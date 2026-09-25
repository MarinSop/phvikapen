pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

// One box of typed text as it sits on the paper, laid out in the units of the page and drawn at
// the zoom of the canvas. The box being typed in is drawn by the layer instead.
//
// A box holding a sum is drawn the way arithmetic is written — fractions one part over the other,
// powers raised, roots under a roof — from what the notebook laid out. While it is being typed in,
// the layer shows what was typed instead, so that it can be corrected.
Item {
    id: root

    required property int align
    required property bool bold
    required property real boxHeight
    required property real boxWidth
    required property color color
    required property real columnX
    required property real columnY
    required property var drawing
    required property string font
    required property bool formula
    required property bool italic
    required property NotebookViewModel notebook
    required property point origin
    required property real size
    required property bool struckOut
    required property string text
    required property string textId
    required property bool underline
    required property real zoom
    readonly property var bars: root.drawn ? root.drawing.bars : []
    // How tall a line of type stands, which is how the sum was laid out.
    readonly property real lineRoom: 1.2
    readonly property var glyphs: root.drawn ? root.drawing.glyphs : []
    readonly property bool drawn: root.formula && root.drawing !== undefined && root.drawing.glyphs !== undefined && root.drawing.glyphs.length > 0
    // A point of type, in the units a page is measured in.
    readonly property real pageUnitsPerPoint: 96 / 72
    readonly property bool picked: root.notebook !== null && root.notebook.pickedText === root.textId

    height: (root.drawn ? root.drawing.height : Math.max(label.implicitHeight, label.font.pixelSize, root.boxHeight)) * root.zoom
    visible: !root.picked
    width: (root.drawn ? root.drawing.width : root.boxWidth) * root.zoom
    x: (root.columnX - root.origin.x) * root.zoom
    y: (root.columnY - root.origin.y) * root.zoom

    Item {
        height: root.drawn ? root.drawing.height : label.implicitHeight
        width: root.drawn ? root.drawing.width : root.boxWidth

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
            visible: !root.drawn
            width: root.boxWidth
            wrapMode: Text.Wrap
        }

        // The lines that hold a sum together: the bar of a fraction and the roof of a root.
        Repeater {
            model: root.bars

            Rectangle {
                id: bar

                required property var modelData

                color: root.color
                height: bar.modelData.height
                objectName: "formulaBar"
                width: bar.modelData.width
                x: bar.modelData.x
                y: bar.modelData.y
            }
        }

        Repeater {
            model: root.glyphs

            Text {
                id: piece

                required property var modelData

                color: root.color
                font.bold: root.bold
                font.family: root.font === "" ? AppInfo.plainFont : root.font
                font.italic: root.italic
                font.pixelSize: Math.max(1, piece.modelData.size * root.pageUnitsPerPoint)
                height: piece.modelData.size * root.pageUnitsPerPoint * root.lineRoom
                objectName: "formulaGlyph"
                text: piece.modelData.text
                verticalAlignment: Text.AlignVCenter
                x: piece.modelData.x
                y: piece.modelData.y
            }
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
