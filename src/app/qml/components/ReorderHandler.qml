import QtQuick

DragHandler {
    id: root

    required property int index
    required property ListView list
    required property Item row
    property real restingY: 0
    readonly property int landing: root.active ? root.gap : -1
    property int gap: -1

    signal moved(int from, int to)

    function gapUnder(scenePosition) {
        if (root.list.count === 0) {
            return 0;
        }
        const step = root.list.contentHeight / root.list.count;
        const point = root.list.mapFromItem(null, scenePosition);
        return Math.max(0, Math.min(root.list.count, Math.round((point.y + root.list.contentY) / step)));
    }

    function landedAt(from, gap) {
        return gap > from ? gap - 1 : gap;
    }

    dragThreshold: 6
    grabPermissions: PointerHandler.CanTakeOverFromAnything
    target: root.row
    xAxis.enabled: false
    yAxis.enabled: true

    onCentroidChanged: {
        if (root.active) {
            root.gap = root.gapUnder(root.centroid.scenePosition);
        }
    }
    onActiveChanged: {
        if (root.active) {
            root.restingY = root.row.y;
            root.gap = root.index;
            return;
        }
        const gap = root.gap;
        root.gap = -1;
        root.row.y = root.restingY;
        root.list.forceLayout();
        const landed = root.landedAt(root.index, gap);
        if (gap >= 0 && landed !== root.index) {
            root.moved(root.index, landed);
        }
    }
}
