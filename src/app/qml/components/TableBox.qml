pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

// One table as it stands on the paper: its ruling, what stands behind the boxes that were given a
// colour of their own, and what is typed in them, laid out in the units of the page and drawn at
// the zoom of the canvas. A box with no colour of its own is left clear, so that what is written
// by hand inside one is seen through the table. The box being typed in is drawn by the layer.
//
// Every box is ruled round on its own rather than the whole grid being drawn line by line, so that
// a box reaching over others has no ruling running through the middle of it.
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
    required property var acrosses
    required property var downs
    required property var fills
    required property var inks
    required property var bolds
    required property var italics
    required property var rises
    // Which box the layer is typing in, so that it is not drawn twice.
    property int hiddenCell: -1
    // The measures while a rule is being pulled about or the whole table sized, so that the ruling
    // follows the pointer before anything is written down.
    property var liveHeights: []
    property var liveWidths: []
    // How far the table has been carried before it is set down, in pixels of the window.
    property real liveX: 0
    property real liveY: 0
    required property point origin
    required property color rule
    required property real ruleWidth
    required property real zoom
    readonly property real cellPadding: 3
    readonly property var acrossNow: root.liveWidths.length > 0 ? root.liveWidths : root.widths
    readonly property var downNow: root.liveHeights.length > 0 ? root.liveHeights : root.heights
    readonly property int columns: Math.max(1, root.acrossNow.length)
    readonly property real onPageTall: root.spanOf(root.downNow)
    readonly property real onPageWide: root.spanOf(root.acrossNow)
    // A point of type, in the units a page is measured in.
    readonly property real pageUnitsPerPoint: 96 / 72

    function edgeBefore(measures, index) {
        let edge = 0;
        for (let step = 0; step < index && step < measures.length; ++step) {
            edge += measures[step];
        }
        return edge;
    }

    // How far a box reaches, counting one where the table has not said.
    function reachOf(measures, index) {
        const said = measures[index];
        return said === undefined ? 1 : said;
    }

    function spanOf(measures) {
        return root.edgeBefore(measures, measures.length);
    }

    // What a box says for itself, or nothing where it says nothing and the table speaks for it.
    function wantOf(said, index, plain) {
        const answer = said[index];
        return answer === undefined ? plain : answer;
    }

    height: root.onPageTall * root.zoom
    objectName: "tableBox"
    width: root.onPageWide * root.zoom
    x: ((root.columnX - root.origin.x) * root.zoom) + root.liveX
    y: ((root.columnY - root.origin.y) * root.zoom) + root.liveY

    Item {
        height: root.onPageTall
        width: root.onPageWide

        transform: Scale {
            xScale: root.zoom
            yScale: root.zoom
        }

        Repeater {
            model: root.words.length

            Item {
                id: cell

                required property int index
                readonly property int across: root.reachOf(root.acrosses, cell.index)
                readonly property real boxLeft: root.edgeBefore(root.acrossNow, cell.column)
                readonly property real boxTop: root.edgeBefore(root.downNow, cell.row)
                readonly property int column: cell.index % root.columns
                readonly property bool covered: cell.across < 1 || cell.down < 1
                readonly property int down: root.reachOf(root.downs, cell.index)
                readonly property color fill: root.wantOf(root.fills, cell.index, "transparent")
                readonly property color ink: root.wantOf(root.inks, cell.index, "transparent")
                readonly property int rise: root.wantOf(root.rises, cell.index, 0)
                readonly property int row: Math.floor(cell.index / root.columns)

                height: root.edgeBefore(root.downNow, cell.row + Math.max(1, cell.down)) - cell.boxTop
                visible: !cell.covered
                width: root.edgeBefore(root.acrossNow, cell.column + Math.max(1, cell.across)) - cell.boxLeft
                x: cell.boxLeft
                y: cell.boxTop

                // What was asked to stand behind this box, drawn under the ruling so that the
                // rules are not half covered by the box beside them.
                Rectangle {
                    anchors.fill: parent
                    color: cell.fill
                    objectName: "tableCellFill"
                    visible: cell.fill.a > 0
                }

                // The rule is drawn about the edge of the box rather than inside it, so that two
                // boxes side by side share one rule instead of each drawing its own.
                Rectangle {
                    border.color: root.rule
                    border.width: root.ruleWidth
                    color: "transparent"
                    height: cell.height + root.ruleWidth
                    width: cell.width + root.ruleWidth
                    x: -root.ruleWidth / 2
                    y: -root.ruleWidth / 2
                }

                Text {
                    clip: true
                    color: cell.ink.a > 0 ? cell.ink : root.style.color
                    font.bold: root.style.bold || root.wantOf(root.bolds, cell.index, false)
                    font.family: root.style.font === "" ? AppInfo.plainFont : root.style.font
                    font.italic: root.style.italic || root.wantOf(root.italics, cell.index, false)
                    font.pixelSize: Math.max(1, root.style.size * root.pageUnitsPerPoint)
                    font.strikeout: root.style.struckOut
                    font.underline: root.style.underline
                    height: Math.max(1, cell.height - (2 * root.cellPadding))
                    horizontalAlignment: [Text.AlignLeft, Text.AlignHCenter, Text.AlignRight, Text.AlignJustify][root.aligns[cell.index]]
                    objectName: "tableCell"
                    text: root.words[cell.index]
                    verticalAlignment: [Text.AlignTop, Text.AlignVCenter, Text.AlignBottom][cell.rise]
                    visible: cell.index !== root.hiddenCell
                    width: Math.max(1, cell.width - (2 * root.cellPadding))
                    wrapMode: Text.Wrap
                    x: root.cellPadding
                    y: root.cellPadding
                }
            }
        }
    }
}
