pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import PhvikaPen.Ui

// A panel set free, in a window of its own. Where a floating panel stands over the sheet and cannot
// leave the application, this one is an ordinary window: it goes anywhere the screens reach, and
// where it stands is remembered beside everything else about the panel.
Window {
    id: root

    required property AppActions actions
    required property PanelDragState drag
    required property var place
    required property WorkspaceViewModel workspace

    function remember() {
        root.workspace.movePanelWindow(root.place.path, Math.round(root.x), Math.round(root.y));
        root.workspace.sizePanelWindow(root.place.path, Math.round(root.width), Math.round(root.height));
    }

    color: Theme.surface
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowCloseButtonHint
    height: Math.max(root.workspace.leastExtent, root.place.height)
    minimumHeight: root.workspace.leastExtent
    minimumWidth: root.workspace.leastExtent
    objectName: "loosePanelWindow_" + root.place.path
    title: Panels.titleOf(root.workspace.firstPanelOf(root.place.path), Languages.spoken)
    visible: true
    width: Math.max(root.workspace.leastExtent, root.place.width)
    x: root.place.x
    y: root.place.y

    // Closing the window brings the panel back over the sheet rather than losing it, which is what
    // a reader who shuts a window expects of a panel and not of a document.
    onClosing: close => {
        close.accepted = true;
        root.workspace.setPanelLoose(root.place.path, false);
    }
    onXChanged: settling.restart()
    onYChanged: settling.restart()
    onWidthChanged: settling.restart()
    onHeightChanged: settling.restart()

    Timer {
        id: settling

        interval: Theme.calm
        repeat: false

        onTriggered: root.remember()
    }

    Item {
        id: holder

        anchors.fill: parent

        PanelArea {
            actions: root.actions
            anchors.fill: parent
            drag: root.drag
            node: root.place.node
            view: holder
            workspace: root.workspace
        }

        PanelDropMarks {
            anchors.fill: parent
            drag: root.drag
        }
    }
}
