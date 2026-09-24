pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

// The frame around what is picked up, with the grips that size and turn it. It stands exactly
// where the canvas draws its own frame, and takes that drawing over while something is being
// turned or sized, so that the frame follows the ink instead of standing still beside it. Only
// the grips take the pointer; everything inside falls through to the canvas, which carries what
// is picked about as it always has.
Item {
    id: root

    required property InkCanvas canvas
    required property NotebookViewModel notebook
    required property ToolViewModel tools
    // Where what is picked stands, in the coordinates of the column of sheets.
    property rect area: Qt.rect(0, 0, 0, 0)
    // What the reader is doing to it right now, before it becomes a change that can be undone.
    property real liveWide: 1
    property real liveTall: 1
    property real liveTurn: 0
    property point pivot: Qt.point(0, 0)
    property real turnedFrom: 0
    property bool working: false
    readonly property real quarter: 15
    readonly property int gripSize: Math.round(9 * Theme.scale)
    readonly property real margin: root.canvas === null ? 0 : root.canvas.selectionMargin
    readonly property point origin: root.canvas === null ? Qt.point(0, 0) : root.canvas.viewOrigin
    readonly property bool picking: root.tools !== null && root.tools.currentTool === ToolViewModel.Selection
    readonly property real zoom: root.canvas === null ? 1 : root.canvas.zoom
    readonly property bool shown: root.picking && root.notebook !== null && root.canvas !== null && root.canvas.selectedCount > 0 && root.area.width > 0 && root.area.height > 0
    readonly property real boxLeft: (root.area.x - root.margin - root.origin.x) * root.zoom
    readonly property real boxTop: (root.area.y - root.margin - root.origin.y) * root.zoom
    readonly property real wide: (root.area.width + (root.margin * 2)) * root.zoom
    readonly property real tall: (root.area.height + (root.margin * 2)) * root.zoom
    // The point that stays still, where it sits in this item.
    readonly property real pivotX: (root.pivot.x - root.origin.x) * root.zoom
    readonly property real pivotY: (root.pivot.y - root.origin.y) * root.zoom

    // What is being done, in the words the notebook reads it in.
    function change() {
        return {
            "pivotX": root.pivot.x,
            "pivotY": root.pivot.y,
            "dx": 0,
            "dy": 0,
            "wide": root.liveWide,
            "tall": root.liveTall,
            "turn": root.liveTurn
        };
    }

    // The angle from the middle of the frame out to where the pointer is, in degrees.
    function facing(scenePoint) {
        const at = root.mapFromItem(null, scenePoint);
        const middleX = root.boxLeft + (root.wide / 2);
        const middleY = root.boxTop + (root.tall / 2);
        return Math.atan2(at.y - middleY, at.x - middleX) * 180 / Math.PI;
    }

    function letGo() {
        if (!root.working) {
            return;
        }
        root.working = false;
        root.notebook.applyTransform(root.change());
        root.liveWide = 1;
        root.liveTall = 1;
        root.liveTurn = 0;
        root.refresh();
    }

    function refresh() {
        root.area = root.notebook === null ? Qt.rect(0, 0, 0, 0) : root.notebook.selectionArea();
    }

    function show() {
        if (root.working) {
            root.notebook.showTransform(root.change());
        }
    }

    // Sizing keeps the corner opposite the grip exactly where it is.
    function sizeFrom(cornerX, cornerY) {
        root.pivot = Qt.point(cornerX, cornerY);
        root.working = true;
    }

    function turnFrom(scenePoint) {
        root.pivot = Qt.point(root.area.x + (root.area.width / 2), root.area.y + (root.area.height / 2));
        root.turnedFrom = root.facing(scenePoint);
        root.working = true;
    }

    objectName: "selectionLayer"
    visible: root.shown

    onPickingChanged: {
        if (!root.picking) {
            root.letGo();
        }
    }

    Binding {
        property: "markSelection"
        target: root.canvas
        value: !root.working
    }

    Connections {
        function onSelectionChanged() {
            if (!root.working && root.notebook !== null) {
                root.notebook.dropTransform();
            }
            root.refresh();
        }

        function onViewChanged() {
            root.refresh();
        }

        target: root.canvas
    }

    Connections {
        function onHistoryChanged() {
            root.refresh();
        }

        function onPageChanged() {
            root.refresh();
        }

        target: root.notebook
    }

    // The frame follows the ink through the whole drag: it is scaled and turned about the very
    // point the strokes below it are scaled and turned about.
    Item {
        height: root.tall
        width: root.wide
        x: root.boxLeft
        y: root.boxTop

        transform: [
            Scale {
                origin.x: root.pivotX - root.boxLeft
                origin.y: root.pivotY - root.boxTop
                xScale: root.liveWide
                yScale: root.liveTall
            },
            Rotation {
                angle: root.liveTurn
                origin.x: root.pivotX - root.boxLeft
                origin.y: root.pivotY - root.boxTop
            }
        ]

        Rectangle {
            anchors.fill: parent
            border.color: Theme.accent
            border.width: 1
            color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.08)
            objectName: "selectionFrame"
        }
    }

    // Eight grips: the corners size both ways at once, the edges one way at a time. They step
    // aside while a drag is under way, so that nothing stands between the reader and the shape
    // being made.
    Repeater {
        model: 8

        Rectangle {
            id: grip

            required property int index
            // Which way the grip pulls: -1 towards the start of the axis, 1 towards the end, 0 not
            // at all. The eight are the four corners, then the four edges.
            readonly property int pullX: [-1, 1, 1, -1, 0, 1, 0, -1][grip.index]
            readonly property int pullY: [-1, -1, 1, 1, -1, 0, 1, 0][grip.index]

            antialiasing: true
            border.color: Theme.accent
            border.width: 1
            color: Theme.accentText
            height: root.gripSize
            objectName: "sizeGrip" + grip.index
            radius: 2
            visible: !root.working
            width: root.gripSize
            x: root.boxLeft + ((grip.pullX + 1) / 2 * root.wide) - (width / 2)
            y: root.boxTop + ((grip.pullY + 1) / 2 * root.tall) - (height / 2)

            DragHandler {
                id: sizeDrag

                target: null

                onActiveChanged: {
                    if (sizeDrag.active) {
                        // The far corner is the one that stays still.
                        root.sizeFrom(grip.pullX > 0 ? root.area.x : root.area.x + root.area.width, grip.pullY > 0 ? root.area.y : root.area.y + root.area.height);
                    } else {
                        root.letGo();
                    }
                }
                onTranslationChanged: {
                    if (!sizeDrag.active || !root.shown) {
                        return;
                    }
                    const alongX = sizeDrag.activeTranslation.x / root.zoom * grip.pullX;
                    const alongY = sizeDrag.activeTranslation.y / root.zoom * grip.pullY;
                    const wider = root.area.width <= 0 ? 1 : Math.max(0.05, (root.area.width + alongX) / root.area.width);
                    const taller = root.area.height <= 0 ? 1 : Math.max(0.05, (root.area.height + alongY) / root.area.height);
                    // A corner keeps the drawing in proportion; an edge pulls only its own way.
                    if (grip.pullX !== 0 && grip.pullY !== 0) {
                        const evenly = Math.max(wider, taller);
                        root.liveWide = evenly;
                        root.liveTall = evenly;
                    } else {
                        root.liveWide = grip.pullX === 0 ? 1 : wider;
                        root.liveTall = grip.pullY === 0 ? 1 : taller;
                    }
                    root.show();
                }
            }
        }
    }

    // The knob above the frame turns what is picked. Held with Shift it stops every fifteen
    // degrees, the way a drawing program turns things.
    Rectangle {
        antialiasing: true
        border.color: Theme.accent
        border.width: 1
        color: Theme.accentText
        height: root.gripSize + 2
        objectName: "turnGrip"
        radius: height / 2
        visible: !root.working
        width: root.gripSize + 2
        x: root.boxLeft + (root.wide / 2) - (width / 2)
        y: root.boxTop - (root.gripSize * 2)

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.bottom
            color: Theme.accent
            height: root.gripSize
            opacity: 0.7
            width: 1
        }

        DragHandler {
            id: turnDrag

            acceptedModifiers: Qt.NoModifier
            target: null

            onActiveChanged: {
                if (turnDrag.active) {
                    root.turnFrom(turnDrag.centroid.scenePosition);
                } else {
                    root.letGo();
                }
            }
            onCentroidChanged: {
                if (!turnDrag.active || !root.shown) {
                    return;
                }
                root.liveTurn = root.facing(turnDrag.centroid.scenePosition) - root.turnedFrom;
                root.show();
            }
        }

        DragHandler {
            id: stepDrag

            acceptedModifiers: Qt.ShiftModifier
            target: null

            onActiveChanged: {
                if (stepDrag.active) {
                    root.turnFrom(stepDrag.centroid.scenePosition);
                } else {
                    root.letGo();
                }
            }
            onCentroidChanged: {
                if (!stepDrag.active || !root.shown) {
                    return;
                }
                const wanted = root.facing(stepDrag.centroid.scenePosition) - root.turnedFrom;
                root.liveTurn = Math.round(wanted / root.quarter) * root.quarter;
                root.show();
            }
        }
    }
}
