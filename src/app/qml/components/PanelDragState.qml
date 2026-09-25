import QtQuick

QtObject {
    id: root

    readonly property int closeRank: 30
    readonly property int stackRank: 20
    readonly property int edgeRank: 10
    property string panelId: ""
    property point at: Qt.point(0, 0)
    property string kind: ""
    property int side: -1
    property int group: -1
    property int rank: -1
    property point settledAt: Qt.point(-1, -1)
    readonly property bool dragging: root.panelId !== ""

    signal dropped

    function report(rank, kind, side, group) {
        if (root.settledAt.x !== root.at.x || root.settledAt.y !== root.at.y) {
            root.settledAt = root.at;
            root.rank = -1;
            root.kind = "";
            root.side = -1;
            root.group = -1;
        }
        if (kind === "" || rank <= root.rank) {
            return;
        }
        root.rank = rank;
        root.kind = kind;
        root.side = side;
        root.group = group;
    }

    function take(panelId, scenePosition) {
        root.panelId = panelId;
        root.at = scenePosition;
        root.settledAt = Qt.point(-1, -1);
    }

    function letGo() {
        root.panelId = "";
        root.kind = "";
        root.side = -1;
        root.group = -1;
        root.rank = -1;
        root.settledAt = Qt.point(-1, -1);
    }
}
