import QtQuick
import PhvikaPen.Ui

Item {
    id: root

    required property ToolViewModel tools

    InkCanvas {
        id: canvas

        anchors.fill: parent
        strokeColor: root.tools.strokeColor
        strokeWidth: root.tools.strokeWidth
    }

    Connections {
        function onClearRequested() {
            canvas.clear();
        }

        target: root.tools
    }
}
