pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

ToolBar {
    id: root

    property NotebookViewModel notebook: null
    required property ToolViewModel tools
    readonly property InkCanvas canvas: root.notebook === null ? null : root.notebook.canvas

    signal settingsWanted

    RowLayout {
        anchors.fill: parent
        spacing: 6

        ToolButton {
            checkable: true
            checked: root.tools.currentTool === ToolViewModel.Pen
            objectName: "penButton"
            text: qsTr("Pen")

            onClicked: root.tools.currentTool = ToolViewModel.Pen
        }

        Repeater {
            model: root.tools.penCount

            PenSwatch {
                required property int index

                checked: root.tools.pen === index && root.tools.currentTool !== ToolViewModel.Eraser
                color: root.tools.colorOfPen(index)
                penWidth: root.tools.widthOfPen(index)

                onClicked: {
                    root.tools.pen = index;
                    root.tools.currentTool = ToolViewModel.Pen;
                }
            }
        }

        ToolButton {
            checkable: true
            checked: root.tools.currentTool === ToolViewModel.Highlighter
            objectName: "highlighterButton"
            text: qsTr("Highlighter")

            onClicked: root.tools.currentTool = ToolViewModel.Highlighter
        }

        ToolButton {
            checkable: true
            checked: root.tools.currentTool === ToolViewModel.Selection
            objectName: "selectionButton"
            text: qsTr("Select")

            onClicked: root.tools.currentTool = ToolViewModel.Selection
        }

        ToolButton {
            enabled: root.canvas !== null && root.canvas.selectedCount > 0
            objectName: "copySelectionButton"
            text: qsTr("Copy")
            visible: root.tools.currentTool === ToolViewModel.Selection

            onClicked: root.notebook.copySelection()
        }

        ToolButton {
            enabled: root.notebook !== null && root.notebook.hasCopiedStrokes
            objectName: "pasteButton"
            text: qsTr("Paste")
            visible: root.tools.currentTool === ToolViewModel.Selection

            onClicked: root.notebook.pasteStrokes()
        }

        ToolButton {
            enabled: root.canvas !== null && root.canvas.selectedCount > 0
            objectName: "deleteSelectionButton"
            text: qsTr("Delete")
            ToolTip.text: qsTr("Delete what is picked")
            ToolTip.visible: hovered
            visible: root.tools.currentTool === ToolViewModel.Selection

            onClicked: root.notebook.deleteSelection()
        }

        ToolButton {
            checkable: true
            checked: root.tools.currentTool === ToolViewModel.Eraser
            objectName: "eraserButton"
            text: qsTr("Eraser")

            onClicked: root.tools.currentTool = ToolViewModel.Eraser
        }

        ToolSeparator {
        }

        ToolButton {
            objectName: "colorButton"
            text: qsTr("Color")

            onClicked: colorMenu.popup()

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.margins: 4
                color: root.tools.strokeColor
                height: 3
                width: parent.width - 16
            }

            Menu {
                id: colorMenu

                GridLayout {
                    columns: 4

                    Repeater {
                        model: root.tools.palette

                        Rectangle {
                            id: swatch

                            required property color modelData

                            Layout.margins: 4
                            border.color: Qt.darker(swatch.modelData, 1.4)
                            border.width: root.tools.strokeColor.toString() === swatch.modelData.toString() ? 2 : 1
                            color: swatch.modelData
                            implicitHeight: 24
                            implicitWidth: 24
                            radius: 4

                            TapHandler {
                                onTapped: {
                                    if (root.canvas !== null && root.canvas.selectedCount > 0) {
                                        root.notebook.recolourSelection(swatch.modelData);
                                    } else {
                                        root.tools.strokeColor = swatch.modelData;
                                    }
                                    colorMenu.close();
                                }
                            }
                        }
                    }
                }
            }
        }

        Label {
            text: root.tools.currentTool === ToolViewModel.Eraser ? qsTr("Eraser") : qsTr("Width")
        }

        Slider {
            id: sizeSlider

            Layout.preferredWidth: 140
            from: root.tools.currentTool === ToolViewModel.Eraser ? 4 : 0.5
            to: root.tools.currentTool === ToolViewModel.Eraser ? 40 : 24
            value: root.tools.currentTool === ToolViewModel.Eraser ? root.tools.eraserRadius : root.tools.strokeWidth

            onMoved: {
                if (root.tools.currentTool === ToolViewModel.Eraser) {
                    root.tools.eraserRadius = sizeSlider.value;
                } else {
                    root.tools.strokeWidth = sizeSlider.value;
                }
            }
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

        ToolButton {
            objectName: "settingsButton"
            text: qsTr("⚙")
            ToolTip.text: qsTr("Settings")
            ToolTip.visible: hovered

            onClicked: root.settingsWanted()
        }
    }
}
