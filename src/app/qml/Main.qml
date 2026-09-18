pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

ApplicationWindow {
    id: root

    readonly property NotebookViewModel notebook: notebooks.current

    height: 800
    title: root.notebook === null ? qsTr("PhvikaPen %1").arg(AppInfo.version) : qsTr("%1 — PhvikaPen %2").arg(root.notebook.title).arg(AppInfo.version)
    visible: true
    width: 1280

    header: InkToolBar {
        notebook: root.notebook
        tools: toolState
    }

    ToolViewModel {
        id: toolState
    }

    NotebooksViewModel {
        id: notebooks

        onErrorMessage: message => messageBar.show(message)
    }

    Shortcut {
        sequences: [StandardKey.Undo]

        onActivated: root.notebook.undo()
    }

    Shortcut {
        sequences: [StandardKey.Redo]

        onActivated: root.notebook.redo()
    }

    Shortcut {
        sequences: [StandardKey.MoveToPreviousPage]

        onActivated: root.notebook.previousPage()
    }

    Shortcut {
        sequences: [StandardKey.MoveToNextPage]

        onActivated: root.notebook.nextPage()
    }

    Shortcut {
        sequences: [StandardKey.ZoomIn]

        onActivated: notebooks.canvas.zoomIn()
    }

    Shortcut {
        sequences: [StandardKey.ZoomOut]

        onActivated: notebooks.canvas.zoomOut()
    }

    Shortcut {
        sequences: ["Ctrl+0"]

        onActivated: notebooks.canvas.fitPage()
    }

    NotebookTabs {
        id: tabs

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        notebooks: notebooks
    }

    NotebookPage {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: tabs.bottom
        notebooks: notebooks
        tools: toolState
    }

    MessageBar {
        id: messageBar

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 16
    }

    Connections {
        function onExported(path) {
            messageBar.show(qsTr("Saved as %1").arg(path));
        }

        function onErrorMessageChanged() {
            if (root.notebook !== null && root.notebook.errorMessage !== "") {
                messageBar.show(root.notebook.errorMessage);
            }
        }

        target: root.notebook
    }
}
