pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

Item {
    id: root

    required property MathViewModel maths
    readonly property real fromAcross: root.maths.frame.left
    readonly property real toAcross: root.maths.frame.right
    readonly property real fromUp: root.maths.frame.bottom
    readonly property real toUp: root.maths.frame.top
    readonly property real acrossSpan: root.toAcross - root.fromAcross
    readonly property real upSpan: root.toUp - root.fromUp

    function atX(across) {
        return root.acrossSpan === 0 ? 0 : (across - root.fromAcross) / root.acrossSpan * root.width;
    }

    function atY(up) {
        return root.upSpan === 0 ? 0 : root.height - ((up - root.fromUp) / root.upSpan * root.height);
    }

    function acrossOf(x) {
        return root.fromAcross + (x / Math.max(1, root.width) * root.acrossSpan);
    }

    function upOf(y) {
        return root.fromUp + ((root.height - y) / Math.max(1, root.height) * root.upSpan);
    }

    // A step between rules that is one, two or five times a power of ten, so that the numbers
    // written along the axes are ones a reader counts in.
    function stepFor(span) {
        if (!(span > 0)) {
            return 1;
        }
        const rough = span / 8;
        const power = Math.pow(10, Math.floor(Math.log(rough) / Math.LN10));
        const times = rough / power;
        return power * (times >= 5 ? 5 : times >= 2 ? 2 : 1);
    }

    function labelFor(value, step) {
        const places = Math.max(0, Math.min(6, -Math.floor(Math.log(step) / Math.LN10)));
        return value.toFixed(places);
    }

    clip: true
    implicitHeight: Math.round(220 * Theme.scale)

    onWidthChanged: {
        root.maths.setSamples(Math.round(root.width));
        sheet.requestPaint();
    }
    onHeightChanged: sheet.requestPaint()

    Canvas {
        id: sheet

        anchors.fill: parent
        renderStrategy: Canvas.Cooperative

        onPaint: {
            const context = sheet.getContext("2d");
            context.reset();
            context.fillStyle = Theme.surfaceStrong;
            context.fillRect(0, 0, sheet.width, sheet.height);
            if (root.acrossSpan <= 0 || root.upSpan <= 0) {
                return;
            }

            const acrossStep = root.stepFor(root.acrossSpan);
            const upStep = root.stepFor(root.upSpan);
            context.lineWidth = 1;
            context.strokeStyle = Theme.line;
            context.beginPath();
            for (let across = Math.ceil(root.fromAcross / acrossStep) * acrossStep; across <= root.toAcross; across += acrossStep) {
                const x = Math.round(root.atX(across)) + 0.5;
                context.moveTo(x, 0);
                context.lineTo(x, sheet.height);
            }
            for (let up = Math.ceil(root.fromUp / upStep) * upStep; up <= root.toUp; up += upStep) {
                const y = Math.round(root.atY(up)) + 0.5;
                context.moveTo(0, y);
                context.lineTo(sheet.width, y);
            }
            context.stroke();

            context.strokeStyle = Theme.subtleText;
            context.lineWidth = 1.5;
            context.beginPath();
            if (root.fromAcross <= 0 && root.toAcross >= 0) {
                const zeroX = Math.round(root.atX(0)) + 0.5;
                context.moveTo(zeroX, 0);
                context.lineTo(zeroX, sheet.height);
            }
            if (root.fromUp <= 0 && root.toUp >= 0) {
                const zeroY = Math.round(root.atY(0)) + 0.5;
                context.moveTo(0, zeroY);
                context.lineTo(sheet.width, zeroY);
            }
            context.stroke();

            context.fillStyle = Theme.subtleText;
            context.font = Math.round(10 * Theme.scale) + "px sans-serif";
            const baseY = root.fromUp <= 0 && root.toUp >= 0 ? root.atY(0) : sheet.height;
            for (let across = Math.ceil(root.fromAcross / acrossStep) * acrossStep; across <= root.toAcross; across += acrossStep) {
                if (Math.abs(across) < acrossStep / 2) {
                    continue;
                }
                context.fillText(root.labelFor(across, acrossStep), root.atX(across) + 3, Math.min(sheet.height - 3, baseY + 12));
            }
            const baseX = root.fromAcross <= 0 && root.toAcross >= 0 ? root.atX(0) : 0;
            for (let up = Math.ceil(root.fromUp / upStep) * upStep; up <= root.toUp; up += upStep) {
                if (Math.abs(up) < upStep / 2) {
                    continue;
                }
                context.fillText(root.labelFor(up, upStep), Math.min(sheet.width - 28, baseX + 4), root.atY(up) - 3);
            }

            context.strokeStyle = Theme.accent;
            context.lineWidth = 2;
            context.lineJoin = "round";
            for (const run of root.maths.runs) {
                if (run.length < 2) {
                    continue;
                }
                context.beginPath();
                context.moveTo(root.atX(run[0].x), root.atY(run[0].y));
                for (let step = 1; step < run.length; ++step) {
                    context.lineTo(root.atX(run[step].x), root.atY(run[step].y));
                }
                context.stroke();
            }
        }
    }

    Connections {
        function onCurveChanged() {
            sheet.requestPaint();
        }

        target: root.maths
    }

    DragHandler {
        id: carry

        property real heldAcross: 0
        property real heldUp: 0

        cursorShape: Qt.ClosedHandCursor
        target: null

        onActiveChanged: {
            if (carry.active) {
                carry.heldAcross = root.acrossOf(carry.centroid.position.x);
                carry.heldUp = root.upOf(carry.centroid.position.y);
            }
        }
        onCentroidChanged: {
            if (!carry.active) {
                return;
            }
            root.maths.moveBy(carry.heldAcross - root.acrossOf(carry.centroid.position.x), carry.heldUp - root.upOf(carry.centroid.position.y));
        }
    }

    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad

        onWheel: wheel => {
            const closer = wheel.angleDelta.y > 0;
            root.maths.zoomBy(closer ? 0.85 : 1 / 0.85, root.acrossOf(wheel.x), root.upOf(wheel.y));
            wheel.accepted = true;
        }
    }

    PinchHandler {
        target: null

        onActiveScaleChanged: {
            if (active && activeScale > 0) {
                root.maths.zoomBy(1 / activeScale, root.acrossOf(centroid.position.x), root.upOf(centroid.position.y));
            }
        }
    }

    EmptyPanelNote {
        anchors.centerIn: parent
        text: root.maths.curveMessage
        visible: root.maths.curveMessage !== ""
        width: parent.width - (Theme.gap * 4)
    }
}
