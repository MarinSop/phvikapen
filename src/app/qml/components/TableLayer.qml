pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

// Every table on the paper, drawn over the ink, and the frame around the one that is picked up. A
// tap picks a table up; a second tap opens the box it landed in, so that one editor is ever alive.
// A tap that lands on no table is let through, so that picking ink with the loop still works.
Item {
    id: root

    required property InkCanvas canvas
    required property NotebookViewModel notebook
    required property ToolViewModel tools
    // Which box the editor holds the words of, and which table it was filled from. Nothing is
    // written back to a table the editor never loaded.
    property int editingColumn: -1
    property int editingRow: -1
    // What the reader is doing to the table right now, before it is written down: how far it has
    // been carried, in pixels of the window, and how much wider and taller, in units of the page.
    property real liveTall: 0
    property real liveWide: 0
    property real liveX: 0
    property real liveY: 0
    property string loadedId: ""
    // The measures while a rule is being pulled about, before they are written down.
    property var pulledHeights: []
    property var pulledWidths: []
    readonly property real boxLeft: (((root.holding ? root.picked.columnX : 0) - root.origin.x) * root.zoom) + root.liveX
    readonly property real boxTop: (((root.holding ? root.picked.columnY : 0) - root.origin.y) * root.zoom) + root.liveY
    readonly property real cellPadding: 3
    readonly property int columns: root.widths.length
    readonly property int editingCell: root.typing ? (root.editingRow * Math.max(1, root.columns)) + root.editingColumn : -1
    readonly property int gripSize: Math.round(9 * Theme.scale)
    readonly property real grabRoom: Math.round(9 * Theme.scale)
    readonly property var keptHeights: root.holding ? root.picked.heights : []
    readonly property var keptWidths: root.holding ? root.picked.widths : []
    readonly property var heights: root.pulledHeights.length > 0 ? root.pulledHeights : root.keptHeights
    readonly property real narrowest: 16
    readonly property real shortest: 14
    readonly property bool holding: root.pickedId !== "" && root.picked.tableId !== undefined
    readonly property real onPageTall: Math.max(8, root.spanOf(root.heights) + root.liveTall)
    readonly property real onPageWide: Math.max(8, root.spanOf(root.widths) + root.liveWide)
    readonly property point origin: root.canvas === null ? Qt.point(0, 0) : root.canvas.viewOrigin
    // A point of type, in the units a page is measured in.
    readonly property real pageUnitsPerPoint: 96 / 72
    readonly property var picked: root.notebook === null ? ({}) : root.notebook.pickedTableBox
    readonly property string pickedId: root.notebook === null ? "" : root.notebook.pickedTable
    readonly property bool picking: root.tools !== null && root.tools.currentTool === ToolViewModel.Selection
    readonly property bool typing: root.editingRow >= 0 && root.editingColumn >= 0
    readonly property var widths: root.pulledWidths.length > 0 ? root.pulledWidths : root.keptWidths
    readonly property real zoom: root.canvas === null ? 1 : root.canvas.zoom

    // What is in the editor becomes what its box says, but only for the table the editor was
    // filled from: an editor that has fallen out of step with its table must not write to it.
    function commit() {
        if (root.notebook === null || root.loadedId === "" || root.editingRow < 0) {
            return;
        }
        root.notebook.writeCell(root.loadedId, root.editingRow, root.editingColumn, editor.text);
    }

    function columnPointOf(position) {
        return Qt.point((position.x / root.zoom) + root.origin.x, (position.y / root.zoom) + root.origin.y);
    }

    function edgeBefore(measures, index) {
        let edge = 0;
        for (let step = 0; step < index && step < measures.length; ++step) {
            edge += measures[step];
        }
        return edge;
    }

    function forget() {
        root.editingRow = -1;
        root.editingColumn = -1;
        root.loadedId = "";
        editor.text = "";
        if (editor.activeFocus) {
            editor.focus = false;
        }
    }

    function leave() {
        root.commit();
        root.forget();
    }

    // Which line of measures a point falls in, counting from the first, or -1 beyond the last.
    function lineUnder(measures, at) {
        let edge = 0;
        for (let step = 0; step < measures.length; ++step) {
            const next = edge + measures[step];
            if (at >= edge && at <= next) {
                return step;
            }
            edge = next;
        }
        return -1;
    }

    // A box is opened with what it already says in it, and what the box before said is written
    // down first.
    function openCell(tableId, row, column) {
        if (root.notebook === null || tableId === "" || row < 0 || column < 0) {
            return;
        }
        if (row === root.editingRow && column === root.editingColumn && root.loadedId === tableId) {
            return;
        }
        root.leave();
        root.editingRow = row;
        root.editingColumn = column;
        root.loadedId = tableId;
        editor.text = root.notebook.wordsOfCell(tableId, row, column);
        editor.forceActiveFocus();
        Qt.callLater(root.takeTheKeyboard);
    }

    // The box a tap landed in, asked of the table itself rather than of what has been published,
    // so that a table only just taken hold of answers for the tap that took it.
    function openCellAt(tableId, at) {
        const cell = root.notebook.cellUnder(tableId, at.x, at.y);
        if (cell.row === undefined) {
            return;
        }
        root.openCell(tableId, cell.row, cell.column);
    }

    // One rule between two columns pulled about: what one column gains the next gives up, so the
    // table stays as wide as it was.
    function pullColumn(at, by) {
        root.pulledWidths = root.pulledMeasures(root.keptWidths, at, by, root.narrowest);
    }

    function pullRow(at, by) {
        root.pulledHeights = root.pulledMeasures(root.keptHeights, at, by, root.shortest);
    }

    function pulledMeasures(kept, at, by, smallest) {
        if (at < 1 || at >= kept.length) {
            return [];
        }
        const before = kept[at - 1];
        const after = kept[at];
        const moved = Math.max(smallest - before, Math.min(after - smallest, by));
        const put = [];
        for (let step = 0; step < kept.length; ++step) {
            put.push(kept[step]);
        }
        put[at - 1] = before + moved;
        put[at] = after - moved;
        return put;
    }

    // Where the rules have been pulled to becomes where they stand.
    function settleRules() {
        if (root.holding && (root.pulledWidths.length > 0 || root.pulledHeights.length > 0)) {
            root.notebook.spreadTable(root.pickedId, root.pulledWidths, root.pulledHeights);
        }
        root.pulledWidths = [];
        root.pulledHeights = [];
    }

    // The next box along, or the one before it, running on into the row below. Walking off the
    // end adds a row, as it does everywhere else.
    function stepOn(step) {
        if (!root.typing) {
            return;
        }
        const tableId = root.pickedId;
        const wide = Math.max(1, root.columns);
        const rows = root.heights.length;
        const at = (root.editingRow * wide) + root.editingColumn + step;
        if (at < 0) {
            return;
        }
        if (at >= wide * rows) {
            if (step <= 0) {
                return;
            }
            root.leave();
            root.notebook.addRow(tableId, rows);
            root.openCell(tableId, rows, 0);
            return;
        }
        root.openCell(tableId, Math.floor(at / wide), at % wide);
    }

    // Where the table has been carried and pulled to becomes where it stands.
    function settle() {
        if (!root.holding) {
            return;
        }
        root.notebook.placeTable(root.pickedId, {
            "columnX": root.picked.columnX + (root.liveX / root.zoom),
            "columnY": root.picked.columnY + (root.liveY / root.zoom),
            "boxWidth": root.onPageWide,
            "boxHeight": root.onPageTall
        });
        root.liveX = 0;
        root.liveY = 0;
        root.liveWide = 0;
        root.liveTall = 0;
    }

    function spanOf(measures) {
        return root.edgeBefore(measures, measures.length);
    }

    // The caret goes into the box that is being worked on, at the end of what it says.
    function takeTheKeyboard() {
        if (root.typing && !editor.activeFocus) {
            editor.forceActiveFocus();
            editor.cursorPosition = editor.length;
        }
    }

    anchors.fill: parent
    objectName: "tableLayer"

    onPickedIdChanged: {
        // Whether a table is held is worked out from this very property, so the table the editor
        // was filled from is asked instead: that answer cannot be out of step.
        if (root.loadedId !== "" && root.loadedId !== root.pickedId) {
            root.leave();
        }
    }
    onPickingChanged: {
        if (!root.picking) {
            root.leave();
            if (root.notebook !== null) {
                root.notebook.pickedTable = "";
            }
        }
    }

    Repeater {
        model: root.notebook === null ? null : root.notebook.tables

        TableBox {
            id: drawn

            hiddenCell: drawn.tableId === root.pickedId ? root.editingCell : -1
            liveHeights: drawn.tableId === root.pickedId ? root.pulledHeights : []
            liveWidths: drawn.tableId === root.pickedId ? root.pulledWidths : []
            origin: root.origin
            zoom: root.zoom
        }
    }

    // A press asks what stands under it. Where that is nothing, the press is let through to the
    // canvas, which goes on picking ink as it always has.
    MouseArea {
        anchors.fill: parent
        enabled: root.picking && root.notebook !== null
        objectName: "tablePicker"

        onPressed: mouse => {
            const at = root.columnPointOf(Qt.point(mouse.x, mouse.y));
            const found = root.notebook.tableUnder(at.x, at.y);
            if (found === "") {
                root.leave();
                root.notebook.pickedTable = "";
                mouse.accepted = false;
                return;
            }
            mouse.accepted = true;
            if (found !== root.pickedId) {
                root.leave();
                root.notebook.pickedTable = found;
            }
            // One tap both takes hold of the table and opens the box it landed in, the way every
            // other application behaves.
            root.openCellAt(found, at);
        }
    }

    Item {
        id: frame

        height: root.onPageTall * root.zoom
        visible: root.picking && root.holding
        width: root.onPageWide * root.zoom
        x: root.boxLeft
        y: root.boxTop

        Rectangle {
            anchors.fill: parent
            border.color: Theme.accent
            border.width: 1
            color: "transparent"
            objectName: "tableFrame"
        }

        // Inside the frame carries the table about.
        DragHandler {
            id: moveDrag

            target: null

            onActiveChanged: {
                if (!moveDrag.active) {
                    root.settle();
                }
            }
            onTranslationChanged: {
                if (moveDrag.active && root.holding) {
                    root.liveX = moveDrag.activeTranslation.x;
                    root.liveY = moveDrag.activeTranslation.y;
                }
            }
        }
    }

    // The box being typed in, laid out where it stands in the table.
    Item {
        id: slot

        readonly property real boxTall: root.typing && root.editingRow < root.heights.length ? root.heights[root.editingRow] : 0
        readonly property real boxWide: root.typing && root.editingColumn < root.widths.length ? root.widths[root.editingColumn] : 0

        height: Math.max(1, slot.boxTall - (2 * root.cellPadding)) * root.zoom
        visible: root.picking && root.typing
        width: Math.max(1, slot.boxWide - (2 * root.cellPadding)) * root.zoom
        x: root.boxLeft + ((root.edgeBefore(root.widths, root.editingColumn) + root.cellPadding) * root.zoom)
        y: root.boxTop + ((root.edgeBefore(root.heights, root.editingRow) + root.cellPadding) * root.zoom)

        Item {
            height: slot.height / root.zoom
            width: slot.width / root.zoom

            transform: Scale {
                xScale: root.zoom
                yScale: root.zoom
            }

            TextEdit {
                id: editor

                clip: true
                color: root.holding ? root.picked.style.color : Theme.text
                font.bold: root.holding && root.picked.style.bold
                font.family: !root.holding || root.picked.style.font === "" ? AppInfo.plainFont : root.picked.style.font
                font.italic: root.holding && root.picked.style.italic
                font.pixelSize: Math.max(1, (root.holding ? root.picked.style.size : 12) * root.pageUnitsPerPoint)
                font.strikeout: root.holding && root.picked.style.struckOut
                font.underline: root.holding && root.picked.style.underline
                height: slot.height / root.zoom
                horizontalAlignment: [TextEdit.AlignLeft, TextEdit.AlignHCenter, TextEdit.AlignRight, TextEdit.AlignJustify][root.holding && root.typing ? root.picked.aligns[root.editingCell] : 0]
                objectName: "tableEditor"
                selectByMouse: true
                width: slot.width / root.zoom
                wrapMode: TextEdit.Wrap

                Keys.onEscapePressed: root.leave()
                // Leaving a box writes it down, so that walking a table with the keyboard keeps up
                // with what was typed.
                Keys.onTabPressed: root.stepOn(1)
                Keys.onBacktabPressed: root.stepOn(-1)
            }
        }
    }

    // The rules between the columns can be pulled about, so that one column may be wider than the
    // rest, and likewise the rules between the rows.
    Repeater {
        model: Math.max(0, root.columns - 1)

        Rectangle {
            id: downRule

            required property int index
            readonly property int at: downRule.index + 1

            color: pullDown.active ? Theme.accent : "transparent"
            height: root.onPageTall * root.zoom
            objectName: "tableColumnRule" + downRule.index
            visible: root.picking && root.holding
            width: root.grabRoom
            x: root.boxLeft + (root.edgeBefore(root.widths, downRule.at) * root.zoom) - (width / 2)
            y: root.boxTop

            HoverHandler {
                cursorShape: Qt.SplitHCursor
            }

            DragHandler {
                id: pullDown

                target: null
                yAxis.enabled: false

                onActiveChanged: {
                    if (!pullDown.active) {
                        root.settleRules();
                    }
                }
                onTranslationChanged: {
                    if (pullDown.active && root.holding) {
                        root.pullColumn(downRule.at, pullDown.activeTranslation.x / root.zoom);
                    }
                }
            }
        }
    }

    Repeater {
        model: Math.max(0, root.heights.length - 1)

        Rectangle {
            id: acrossRule

            required property int index
            readonly property int at: acrossRule.index + 1

            color: pullAcross.active ? Theme.accent : "transparent"
            height: root.grabRoom
            objectName: "tableRowRule" + acrossRule.index
            visible: root.picking && root.holding
            width: root.onPageWide * root.zoom
            x: root.boxLeft
            y: root.boxTop + (root.edgeBefore(root.heights, acrossRule.at) * root.zoom) - (height / 2)

            HoverHandler {
                cursorShape: Qt.SplitVCursor
            }

            DragHandler {
                id: pullAcross

                target: null
                xAxis.enabled: false

                onActiveChanged: {
                    if (!pullAcross.active) {
                        root.settleRules();
                    }
                }
                onTranslationChanged: {
                    if (pullAcross.active && root.holding) {
                        root.pullRow(acrossRule.at, pullAcross.activeTranslation.y / root.zoom);
                    }
                }
            }
        }
    }

    // Four corner grips size the table, every column and row taking the same share of the change.
    Repeater {
        model: 4

        Rectangle {
            id: grip

            required property int index
            readonly property int pullX: [-1, 1, 1, -1][grip.index]
            readonly property int pullY: [-1, -1, 1, 1][grip.index]

            antialiasing: true
            border.color: Theme.accent
            border.width: 1
            color: Theme.accentText
            height: root.gripSize
            objectName: "tableGrip" + grip.index
            radius: 2
            visible: root.picking && root.holding
            width: root.gripSize
            x: root.boxLeft + ((grip.pullX + 1) / 2 * root.onPageWide * root.zoom) - (width / 2)
            y: root.boxTop + ((grip.pullY + 1) / 2 * root.onPageTall * root.zoom) - (height / 2)

            DragHandler {
                id: sizeDrag

                target: null

                onActiveChanged: {
                    if (!sizeDrag.active) {
                        root.settle();
                    }
                }
                onTranslationChanged: {
                    if (!sizeDrag.active || !root.holding) {
                        return;
                    }
                    root.liveWide = sizeDrag.activeTranslation.x / root.zoom * grip.pullX;
                    root.liveTall = sizeDrag.activeTranslation.y / root.zoom * grip.pullY;
                    // The corner the reader is not holding stays where it is.
                    root.liveX = grip.pullX > 0 ? 0 : -root.liveWide * root.zoom;
                    root.liveY = grip.pullY > 0 ? 0 : -root.liveTall * root.zoom;
                }
            }
        }
    }
}
