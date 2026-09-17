import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

ToolBar {
    id: root

    property NotebookViewModel notebook: null
    readonly property InkCanvas canvas: root.notebook === null ? null : root.notebook.canvas
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
            enabled: root.notebook !== null && root.notebook.hasPreviousPage
            objectName: "previousPageButton"
            text: qsTr("‹")
            ToolTip.text: qsTr("Previous page")
            ToolTip.visible: hovered

            onClicked: root.notebook.previousPage()
        }

        Label {
            text: root.notebook === null ? "" : qsTr("Page %1 of %2").arg(root.notebook.currentPage + 1).arg(root.notebook.pageCount)
        }

        ToolButton {
            enabled: root.notebook !== null && root.notebook.hasNextPage
            objectName: "nextPageButton"
            text: qsTr("›")
            ToolTip.text: qsTr("Next page")
            ToolTip.visible: hovered

            onClicked: root.notebook.nextPage()
        }

        ToolSeparator {
        }

        ToolButton {
            enabled: root.canvas !== null
            text: qsTr("−")
            ToolTip.text: qsTr("Zoom out")
            ToolTip.visible: hovered

            onClicked: root.canvas.zoomOut()
        }

        ToolButton {
            enabled: root.canvas !== null
            text: root.canvas === null ? qsTr("Fit") : Math.round(root.canvas.zoom * 100) + "%"
            ToolTip.text: qsTr("Fit the page")
            ToolTip.visible: hovered

            onClicked: root.canvas.fitPage()
        }

        ToolButton {
            enabled: root.canvas !== null
            text: qsTr("+")
            ToolTip.text: qsTr("Zoom in")
            ToolTip.visible: hovered

            onClicked: root.canvas.zoomIn()
        }

        ToolSeparator {
        }

        ToolButton {
            enabled: root.notebook !== null
            objectName: "clearButton"
            text: qsTr("Clear")

            onClicked: root.notebook.clearPage()
        }
    }
}
