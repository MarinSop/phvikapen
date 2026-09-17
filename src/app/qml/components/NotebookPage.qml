import QtQuick
import PhvikaPen.Ui

Item {
    id: root

    required property NotebookViewModel notebook
    required property ToolViewModel tools

    function attachWhenShown() {
        if (root.visible) {
            root.notebook.canvas = canvas;
        }
    }

    Component.onCompleted: root.attachWhenShown()
    onVisibleChanged: root.attachWhenShown()

    InkCanvas {
        id: canvas

        anchors.fill: parent
        strokeColor: root.tools.strokeColor
        strokeWidth: root.tools.strokeWidth
    }
}
