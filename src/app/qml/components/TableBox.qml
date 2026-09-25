pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

// One table as it stands on the paper: its ruling and what is typed in its boxes, laid out in the
// units of the page and drawn at the zoom of the canvas. Nothing fills a box, so what is written
// by hand inside one is seen through the table. The box being typed in is drawn by the layer.
Item {
    id: root

    required property var aligns
    required property real columnX
    required property real columnY
    required property var heights
    required property var style
    required property string tableId
    required property var widths
    required property var words
    // Which box the layer is typing in, so that it is not drawn twice.
    property int hiddenCell: -1
    required property point origin
    required property color rule
    required property real ruleWidth
    required property real zoom
    readonly property real cellPadding: 3
    readonly property int columns: Math.max(1, root.widths.length)
    readonly property real onPageTall: root.spanOf(root.heights)
    readonly property real onPageWide: root.spanOf(root.widths)
    // A point of type, in the units a page is measured in.
    readonly property real pageUnitsPerPoint: 96 / 72

    function edgeBefore(measures, index) {
        let edge = 0;
        for (let step = 0; step < index && step < measures.length; ++step) {
            edge += measures[step];
        }
        return edge;
    }

    function spanOf(measures) {
        return root.edgeBefore(measures, measures.length);
    }

    height: root.onPageTall * root.zoom
    width: root.onPageWide * root.zoom
    x: (root.columnX - root.origin.x) * root.zoom
    y: (root.columnY - root.origin.y) * root.zoom

    Item {
        height: root.onPageTall
        width: root.onPageWide

        transform: Scale {
            xScale: root.zoom
            yScale: root.zoom
        }

        Repeater {
            model: root.widths.length + 1

            Rectangle {
                id: downLine

                required property int index

                color: root.rule
                height: root.onPageTall
                width: root.ruleWidth
                x: root.edgeBefore(root.widths, downLine.index) - (root.ruleWidth / 2)
                y: 0
            }
        }

        Repeater {
            model: root.heights.length + 1

            Rectangle {
                id: acrossLine

                required property int index

                color: root.rule
                height: root.ruleWidth
                width: root.onPageWide
                x: 0
                y: root.edgeBefore(root.heights, acrossLine.index) - (root.ruleWidth / 2)
            }
        }

        Repeater {
            model: root.words.length

            Text {
                id: label

                required property int index
                readonly property int column: label.index % root.columns
                readonly property int row: Math.floor(label.index / root.columns)

                clip: true
                color: root.style.color
                font.bold: root.style.bold
                font.family: root.style.font === "" ? AppInfo.plainFont : root.style.font
                font.italic: root.style.italic
                font.pixelSize: Math.max(1, root.style.size * root.pageUnitsPerPoint)
                font.strikeout: root.style.struckOut
                font.underline: root.style.underline
                height: Math.max(1, root.heights[label.row] - (2 * root.cellPadding))
                horizontalAlignment: [Text.AlignLeft, Text.AlignHCenter, Text.AlignRight, Text.AlignJustify][root.aligns[label.index]]
                objectName: "tableCell"
                text: root.words[label.index]
                visible: label.index !== root.hiddenCell
                width: Math.max(1, root.widths[label.column] - (2 * root.cellPadding))
                wrapMode: Text.Wrap
                x: root.edgeBefore(root.widths, label.column) + root.cellPadding
                y: root.edgeBefore(root.heights, label.row) + root.cellPadding
            }
        }
    }
}
