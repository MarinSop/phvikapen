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

        onSettingsWanted: settingsDialog.open()
    }

    ToolViewModel {
        id: toolState
    }

    SettingsViewModel {
        id: settings
    }

    UpdateViewModel {
        id: updates

        Component.onCompleted: {
            if (settings.lookForUpdates) {
                updates.check();
            }
        }
        onRestartWanted: Qt.quit()
    }

    SettingsDialog {
        id: settingsDialog

        settings: settings
        updates: updates
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

    Shortcut {
        sequences: ["P"]

        onActivated: toolState.currentTool = ToolViewModel.Pen
    }

    Shortcut {
        sequences: ["H"]

        onActivated: toolState.currentTool = ToolViewModel.Highlighter
    }

    Shortcut {
        sequences: ["E"]

        onActivated: toolState.currentTool = ToolViewModel.Eraser
    }

    Repeater {
        model: toolState.penCount

        Item {
            id: penShortcut

            required property int index

            Shortcut {
                sequences: [String(penShortcut.index + 1)]

                onActivated: {
                    toolState.pen = penShortcut.index;
                    toolState.currentTool = ToolViewModel.Pen;
                }
            }
        }
    }

    Shortcut {
        sequences: ["["]

        onActivated: toolState.strokeWidth = toolState.strokeWidth - 1
    }

    Shortcut {
        sequences: ["]"]

        onActivated: toolState.strokeWidth = toolState.strokeWidth + 1
    }

    NotebookTabs {
        id: tabs

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        notebooks: notebooks
    }

    UpdateBar {
        id: updateBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: tabs.bottom
        updates: updates
    }

    NotebookPage {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: updateBar.visible ? updateBar.bottom : tabs.bottom
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
        function onFailed(message) {
            messageBar.show(message);
        }

        target: updates
    }

    Connections {
        function onCopied(path) {
            messageBar.show(qsTr("Copied to %1").arg(path));
        }

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
