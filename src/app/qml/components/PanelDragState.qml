import QtQuick

QtObject {
    id: root

    readonly property int closeRank: 40
    readonly property int tabRank: 30
    readonly property int edgeRank: 20
    readonly property int intoRank: 10
    property string panelId: ""
    property point at: Qt.point(0, 0)
    property string kind: ""
    property string path: ""
    property int edge: -1
    property int at2: 0
    property rect hint: Qt.rect(0, 0, 0, 0)
    property int rank: -1
    property point settledAt: Qt.point(-1, -1)
    readonly property bool dragging: root.panelId !== ""

    signal dropped

    function forget() {
        root.rank = -1;
        root.kind = "";
        root.path = "";
        root.edge = -1;
        root.at2 = 0;
        root.hint = Qt.rect(0, 0, 0, 0);
    }

    function report(rank, kind, path, edge, at2, hint) {
        if (root.settledAt.x !== root.at.x || root.settledAt.y !== root.at.y) {
            root.settledAt = root.at;
            root.forget();
        }
        if (kind === "" || rank <= root.rank) {
            return;
        }
        root.rank = rank;
        root.kind = kind;
        root.path = path;
        root.edge = edge;
        root.at2 = at2;
        root.hint = hint;
    }

    function take(panelId, scenePosition) {
        root.panelId = panelId;
        root.at = scenePosition;
        root.settledAt = Qt.point(-1, -1);
    }

    function letGo() {
        root.panelId = "";
        root.settledAt = Qt.point(-1, -1);
        root.forget();
    }
}
