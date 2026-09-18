import QtQuick
import QtQuick.Layouts
import PhvikaPen.Ui

RowLayout {
    id: root

    required property NotebooksViewModel notebooks
    required property ToolViewModel tools
    readonly property NotebookViewModel notebook: root.notebooks.current

    spacing: 0

    Component.onCompleted: root.notebooks.canvas = canvas

    OutlineSidebar {
        Layout.fillHeight: true
        Layout.preferredWidth: 220
        notebook: root.notebook
    }

    InkCanvas {
        id: canvas

        Layout.fillHeight: true
        Layout.fillWidth: true
        enabled: root.notebook !== null && root.notebook.loaded
        eraserRadius: root.tools.eraserRadius
        erasing: root.tools.currentTool === ToolViewModel.Eraser
        selecting: root.tools.currentTool === ToolViewModel.Selection
        shape: root.tools.shape
        pressureSensitive: root.tools.pressureSensitive
        strokeColor: root.tools.strokeColor
        strokeWidth: root.tools.strokeWidth
    }
}
