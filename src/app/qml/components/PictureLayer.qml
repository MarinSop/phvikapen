pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

// The frame around the picture that is picked up, with the grips that move, size and turn it. The
// picture itself is drawn by the canvas, under the ink; only its frame lives here. A tap that
// lands on no picture is let through, so that picking ink with the loop still works.
Item {
    id: root

    required property InkCanvas canvas
    required property NotebookViewModel notebook
    required property ToolViewModel tools
    // What the reader is doing to it right now, before it is written down.
    property real liveX: 0
    property real liveY: 0
    property real liveWide: 0
    property real liveTall: 0
    property real liveTurn: 0
    property real turnedFrom: 0
    readonly property real quarter: 15
    readonly property int gripSize: Math.round(9 * Theme.scale)
    readonly property point origin: root.canvas === null ? Qt.point(0, 0) : root.canvas.viewOrigin
    readonly property var picked: root.notebook === null ? ({}) : root.notebook.pickedPictureBox
    readonly property bool picking: root.tools !== null && root.tools.currentTool === ToolViewModel.Selection
    readonly property string pickedId: root.notebook === null ? "" : root.notebook.pickedPicture
    readonly property bool holding: root.pickedId !== "" && root.picked.pictureId !== undefined
    readonly property real zoom: root.canvas === null ? 1 : root.canvas.zoom
    readonly property real onPageWide: Math.max(8, (root.holding ? root.picked.boxWidth : 0) + root.liveWide)
    readonly property real onPageTall: Math.max(8, (root.holding ? root.picked.boxHeight : 0) + root.liveTall)
    readonly property real turnNow: (root.holding ? root.picked.turn : 0) + root.liveTurn
    readonly property real boxLeft: ((root.holding ? root.picked.columnX : 0) - root.origin.x) * root.zoom + root.liveX
    readonly property real boxTop: ((root.holding ? root.picked.columnY : 0) - root.origin.y) * root.zoom + root.liveY
    // How many readings have come back. It is counted so that the runs below are asked for again
    // once a reading finishes, which is not something a property of the notebook announces.
    property int readings: 0
    // What has been read out of the picture in hand, so it can be marked out on the picture itself.
    readonly property var found: root.wordsNow(root.readings)

    // A run of words read out of the picture was tapped, so that what it says can be taken
    // somewhere else. The layer itself keeps no clipboard and writes nothing.
    signal wordTapped(string said)
    // The same run, asked for as a box of type where it stands on the page.
    signal wordWanted(string said, point at)

    function wordsNow(readings) {
        return root.holding && root.notebook !== null ? root.notebook.wordsFoundInPicture(root.pickedId) : [];
    }

    function columnPointOf(position) {
        return Qt.point((position.x / root.zoom) + root.origin.x, (position.y / root.zoom) + root.origin.y);
    }

    // The angle from the middle of the frame out to where the pointer is, in degrees.
    function facing(scenePoint) {
        const at = root.mapFromItem(null, scenePoint);
        const middleX = root.boxLeft + (root.onPageWide * root.zoom / 2);
        const middleY = root.boxTop + (root.onPageTall * root.zoom / 2);
        return Math.atan2(at.y - middleY, at.x - middleX) * 180 / Math.PI;
    }

    // Where the picture has been carried, pulled and turned to becomes where it stands.
    function settle() {
        if (!root.holding) {
            return;
        }
        root.notebook.placePicture(root.pickedId, {
            "columnX": root.picked.columnX + (root.liveX / root.zoom),
            "columnY": root.picked.columnY + (root.liveY / root.zoom),
            "boxWidth": root.onPageWide,
            "boxHeight": root.onPageTall,
            "turn": root.turnNow
        });
        root.liveX = 0;
        root.liveY = 0;
        root.liveWide = 0;
        root.liveTall = 0;
        root.liveTurn = 0;
    }

    objectName: "pictureLayer"
    // The layer reaches the paper whenever the pick tool is in hand, whether a picture is held or
    // not: a layer that hides itself once one is let go can never be asked for another.
    visible: root.picking

    onPickingChanged: {
        if (!root.picking && root.notebook !== null) {
            root.notebook.pickedPicture = "";
        }
    }

    Connections {
        function onPictureRead(pictureId, words) {
            root.readings += 1;
        }

        function onPictureUnread(pictureId, why) {
            root.readings += 1;
        }

        target: root.notebook
    }

    // A press asks what stands under it. Where that is nothing, the press is let through to the
    // canvas, which goes on picking ink as it always has.
    MouseArea {
        anchors.fill: parent
        enabled: root.picking && root.notebook !== null
        objectName: "picturePicker"

        onPressed: mouse => {
            const at = root.columnPointOf(Qt.point(mouse.x, mouse.y));
            const found = root.notebook.pictureUnder(at.x, at.y);
            root.notebook.pickedPicture = found;
            mouse.accepted = found !== "";
        }
    }

    Item {
        id: frame

        height: root.onPageTall * root.zoom
        visible: root.holding
        width: root.onPageWide * root.zoom
        x: root.boxLeft
        y: root.boxTop

        transform: Rotation {
            angle: root.turnNow
            origin.x: frame.width / 2
            origin.y: frame.height / 2
        }

        Rectangle {
            anchors.fill: parent
            border.color: Theme.accent
            border.width: 1
            color: "transparent"
            objectName: "pictureFrame"
        }

        // What was read out of the picture, marked out on the picture itself. The corners are kept
        // as shares of it, so they stay right whatever size it is drawn at and however it is turned.
        Repeater {
            model: root.found

            Rectangle {
                id: run

                required property int index
                required property var modelData

                Accessible.name: run.modelData.text
                border.color: Theme.accent
                border.width: 1
                color: Theme.accent
                height: Math.max(1, (modelData.bottom - modelData.top) * frame.height)
                objectName: "pictureWord" + index
                opacity: tapped.hovered ? 0.36 : 0.18
                radius: 2
                width: Math.max(1, (modelData.right - modelData.left) * frame.width)
                x: modelData.left * frame.width
                y: modelData.top * frame.height

                HoverHandler {
                    id: tapped

                    cursorShape: Qt.PointingHandCursor
                }

                TapHandler {
                    // A tap takes the words; a drag still carries the picture about underneath.
                    gesturePolicy: TapHandler.ReleaseWithinBounds

                    onLongPressed: runMenu.popup()
                    onTapped: root.wordTapped(run.modelData.text)
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton

                    onTapped: runMenu.popup()
                }

                Menu {
                    id: runMenu

                    MenuItem {
                        objectName: "copyPictureWord"
                        text: qsTr("Copy these words")

                        onTriggered: root.wordTapped(run.modelData.text)
                    }

                    MenuItem {
                        objectName: "typePictureWord"
                        text: qsTr("Put these words on the page as type")

                        onTriggered: root.wordWanted(run.modelData.text, run.mapToItem(null, 0, 0))
                    }
                }
            }
        }

        // Inside the frame carries the picture about.
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

    // Four corner grips size the picture, keeping the shape it came with.
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
            objectName: "pictureGrip" + grip.index
            radius: 2
            visible: root.holding
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
                    const along = sizeDrag.activeTranslation.x / root.zoom * grip.pullX;
                    const wide = root.picked.boxWidth;
                    const evenly = wide <= 0 ? 1 : Math.max(0.05, (wide + along) / wide);
                    root.liveWide = (wide * evenly) - wide;
                    root.liveTall = (root.picked.boxHeight * evenly) - root.picked.boxHeight;
                    // The corner the reader is not holding stays where it is.
                    root.liveX = grip.pullX > 0 ? 0 : -root.liveWide * root.zoom;
                    root.liveY = grip.pullY > 0 ? 0 : -root.liveTall * root.zoom;
                }
            }
        }
    }

    // The knob above the frame turns the picture, in steps of fifteen degrees while Shift is held.
    Rectangle {
        antialiasing: true
        border.color: Theme.accent
        border.width: 1
        color: Theme.accentText
        height: root.gripSize + 2
        objectName: "pictureTurnGrip"
        radius: height / 2
        visible: root.holding
        width: root.gripSize + 2
        x: root.boxLeft + (root.onPageWide * root.zoom / 2) - (width / 2)
        y: root.boxTop - (root.gripSize * 2)

        DragHandler {
            id: turnDrag

            acceptedModifiers: Qt.NoModifier
            target: null

            onActiveChanged: {
                if (turnDrag.active) {
                    root.turnedFrom = root.facing(turnDrag.centroid.scenePosition) - root.turnNow;
                } else {
                    root.settle();
                }
            }
            onCentroidChanged: {
                if (turnDrag.active && root.holding) {
                    root.liveTurn = root.facing(turnDrag.centroid.scenePosition) - root.turnedFrom - root.picked.turn;
                }
            }
        }

        DragHandler {
            id: stepDrag

            acceptedModifiers: Qt.ShiftModifier
            target: null

            onActiveChanged: {
                if (stepDrag.active) {
                    root.turnedFrom = root.facing(stepDrag.centroid.scenePosition) - root.turnNow;
                } else {
                    root.settle();
                }
            }
            onCentroidChanged: {
                if (!stepDrag.active || !root.holding) {
                    return;
                }
                const wanted = root.facing(stepDrag.centroid.scenePosition) - root.turnedFrom;
                root.liveTurn = (Math.round(wanted / root.quarter) * root.quarter) - root.picked.turn;
            }
        }
    }
}
