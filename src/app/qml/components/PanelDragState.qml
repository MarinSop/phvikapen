import QtQuick

QtObject {
    id: root

    readonly property int closeRank: 40
    readonly property int tabRank: 30
    readonly property int cornerRank: 25
    readonly property int edgeRank: 20
    readonly property int intoRank: 10
    property string panelId: ""
    property string homePath: ""
    property real homeWidth: 0
    property real homeHeight: 0
    property point at: Qt.point(0, 0)
    property point startAt: Qt.point(0, 0)
    property string kind: ""
    property string path: ""
    property int edge: -1
    property int at2: 0
    property rect hint: Qt.rect(0, 0, 0, 0)
    property rect hint2: Qt.rect(0, 0, 0, 0)
    property rect windowRect: Qt.rect(0, 0, 0, 0)
    property int rank: -1
    property point settledAt: Qt.point(-1, -1)
    readonly property bool dragging: root.panelId !== ""

    signal dropped
    signal looseWanted(string panelId, point at, real wide, real tall)

    function forget() {
        root.rank = -1;
        root.kind = "";
        root.path = "";
        root.edge = -1;
        root.at2 = 0;
        root.hint = Qt.rect(0, 0, 0, 0);
        root.hint2 = Qt.rect(0, 0, 0, 0);
    }

    function report(rank, kind, path, edge, at2, hint) {
        root.reportTwo(rank, kind, path, edge, at2, hint, Qt.rect(0, 0, 0, 0));
    }

    function reportTwo(rank, kind, path, edge, at2, hint, hint2) {
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
        root.hint2 = hint2;
    }

    function take(panelId, scenePosition, homePath, homeWidth, homeHeight) {
        root.panelId = panelId;
        root.homePath = homePath;
        root.homeWidth = homeWidth;
        root.homeHeight = homeHeight;
        root.at = scenePosition;
        root.startAt = scenePosition;
        root.windowRect = Qt.rect(0, 0, 0, 0);
        root.settledAt = Qt.point(-1, -1);
    }

    function letGo() {
        root.panelId = "";
        root.homePath = "";
        root.windowRect = Qt.rect(0, 0, 0, 0);
        root.settledAt = Qt.point(-1, -1);
        root.forget();
    }
}
