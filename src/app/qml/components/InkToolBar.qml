import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

ToolBar {
    id: root

    property NotebookViewModel notebook: null
    required property ToolViewModel tools

    RowLayout {
        anchors.fill: parent
        spacing: 8

        ToolButton {
            checkable: true
            checked: root.tools.currentTool === ToolViewModel.Pen
            text: qsTr("Pen")

            onClicked: root.tools.currentTool = ToolViewModel.Pen
        }

        ToolButton {
            // TODO(M2): Erasing needs the stroke model and a spatial index.
            checkable: true
            checked: root.tools.currentTool === ToolViewModel.Eraser
            text: qsTr("Eraser")

            onClicked: root.tools.currentTool = ToolViewModel.Eraser
        }

        ToolSeparator {
        }

        ToolButton {
            enabled: root.notebook !== null && root.notebook.canUndo
            objectName: "undoButton"
            text: qsTr("Undo")

            onClicked: root.notebook.undo()
        }

        ToolButton {
            enabled: root.notebook !== null && root.notebook.canRedo
            objectName: "redoButton"
            text: qsTr("Redo")

            onClicked: root.notebook.redo()
        }

        ToolSeparator {
        }

        Label {
            text: qsTr("Width")
        }

        Slider {
            id: widthSlider

            from: 1
            to: 12
            value: root.tools.strokeWidth

            onMoved: root.tools.strokeWidth = widthSlider.value
        }

        Item {
            Layout.fillWidth: true
        }

        ToolButton {
            enabled: root.notebook !== null
            objectName: "clearButton"
            text: qsTr("Clear")

            onClicked: root.notebook.clearPage()
        }
    }
}
