import QtQuick
import QtQuick.Layouts
import PhvikaPen.Ui

RowLayout {
    id: root

    required property NotebookViewModel notebook
    required property ToolViewModel tools

    function attachWhenShown() {
        if (root.visible) {
            root.notebook.canvas = canvas;
        }
    }

    spacing: 0

    Component.onCompleted: root.attachWhenShown()
    onVisibleChanged: root.attachWhenShown()

    OutlineSidebar {
        Layout.fillHeight: true
        Layout.preferredWidth: 220
        notebook: root.notebook
    }

    InkCanvas {
        id: canvas

        Layout.fillHeight: true
        Layout.fillWidth: true
        enabled: root.notebook.loaded
        erasing: root.tools.currentTool === ToolViewModel.Eraser
        strokeColor: root.tools.strokeColor
        strokeWidth: root.tools.strokeWidth
    }
}
