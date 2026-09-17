pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

ApplicationWindow {
    id: root

    height: 800
    title: qsTr("%1 — PhvikaPen %2").arg(notebookModel.title).arg(AppInfo.version)
    visible: true
    width: 1280

    header: InkToolBar {
        notebook: notebookModel
        tools: toolState
    }

    ToolViewModel {
        id: toolState
    }

    // TODO(M3): One model per notebook, following the tab in front.
    NotebookViewModel {
        id: notebookModel
    }

    Shortcut {
        sequences: [StandardKey.Undo]

        onActivated: notebookModel.undo()
    }

    Shortcut {
        sequences: [StandardKey.Redo]

        onActivated: notebookModel.redo()
    }

    Shortcut {
        sequences: [StandardKey.MoveToPreviousPage]

        onActivated: notebookModel.previousPage()
    }

    Shortcut {
        sequences: [StandardKey.MoveToNextPage]

        onActivated: notebookModel.nextPage()
    }

    Shortcut {
        sequences: [StandardKey.ZoomIn]

        onActivated: notebookModel.canvas.zoomIn()
    }

    Shortcut {
        sequences: [StandardKey.ZoomOut]

        onActivated: notebookModel.canvas.zoomOut()
    }

    Shortcut {
        sequences: ["Ctrl+0"]

        onActivated: notebookModel.canvas.fitPage()
    }

    NotebookPage {
        anchors.fill: parent
        notebook: notebookModel
        tools: toolState
    }
}
