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
    // Where the hand is and where it began, on the screens rather than in one window's scene, so
    // that a panel carried out of its own window can be aimed at another one.
    property point at: Qt.point(0, 0)
    property point startAt: Qt.point(0, 0)
    property string kind: ""
    property string path: ""
    property int edge: -1
    property int at2: 0
    // The lines that show where it would land, also on the screens: whichever window the hand is
    // over draws them in its own coordinates.
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

    // Where a point of an item stands on the screens. Every window reports and reads through this,
    // so one drag is reckoned in one set of coordinates however many windows it crosses.
    function screenOf(item, scenePoint) {
        if (item === null) {
            return scenePoint;
        }
        const local = item.mapFromItem(null, scenePoint.x, scenePoint.y);
        return item.mapToGlobal(local.x, local.y);
    }

    function take(panelId, screenPosition, homePath, homeWidth, homeHeight) {
        root.panelId = panelId;
        root.homePath = homePath;
        root.homeWidth = homeWidth;
        root.homeHeight = homeHeight;
        root.at = screenPosition;
        root.startAt = screenPosition;
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
