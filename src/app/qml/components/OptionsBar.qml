pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

ToolBar {
    id: root

    required property AppActions actions
    required property ToolViewModel tools
    readonly property InkCanvas canvas: root.actions.canvas
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property bool draws: root.tools.currentTool === ToolViewModel.Pen || root.tools.currentTool === ToolViewModel.Highlighter
    readonly property bool erases: root.tools.currentTool === ToolViewModel.Eraser
    readonly property bool picks: root.tools.currentTool === ToolViewModel.Selection

    function toolTitle() {
        switch (root.tools.currentTool) {
        case ToolViewModel.Selection:
            return qsTr("Select");
        case ToolViewModel.Hand:
            return qsTr("Hand");
        case ToolViewModel.Highlighter:
            return qsTr("Highlighter");
        case ToolViewModel.Eraser:
            return qsTr("Eraser");
        default:
            return root.tools.shape === ToolViewModel.Freehand ? qsTr("Pen") : qsTr("Shape");
        }
    }

    objectName: "optionsBar"

    RowLayout {
        anchors.fill: parent
        spacing: 6

        Label {
            Layout.leftMargin: 4
            font.bold: true
            text: root.toolTitle()
        }

        ToolSeparator {
        }

        Repeater {
            model: root.draws && root.tools.currentTool === ToolViewModel.Pen ? root.tools.penCount : 0

            PenSwatch {
                required property int index

                checked: root.tools.pen === index
                color: root.tools.colorOfPen(index)
                penWidth: root.tools.widthOfPen(index)

                onClicked: root.tools.pen = index
            }
        }

        ToolButton {
            objectName: "colorButton"
            text: qsTr("Colour")
            visible: root.draws

            onClicked: colourMenu.popup()

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.margins: 4
                color: root.tools.strokeColor
                height: 3
                width: parent.width - 16
            }

            Menu {
                id: colourMenu

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
                                    colourMenu.close();
                                }
                            }
                        }
                    }
                }
            }
        }

        Label {
            text: root.erases ? qsTr("Size") : qsTr("Width")
            visible: root.draws || root.erases
        }

        Slider {
            id: sizeSlider

            Layout.preferredWidth: 140
            from: root.erases ? 4 : 0.5
            to: root.erases ? 40 : 24
            value: root.erases ? root.tools.eraserRadius : root.tools.strokeWidth
            visible: root.draws || root.erases

            onMoved: {
                if (root.erases) {
                    root.tools.eraserRadius = sizeSlider.value;
                } else {
                    root.tools.strokeWidth = sizeSlider.value;
                }
            }
        }

        ComboBox {
            Layout.preferredWidth: 140
            currentIndex: root.tools.shape
            model: [qsTr("Freehand"), qsTr("Straight line"), qsTr("Box"), qsTr("Circle")]
            objectName: "shapeBox"
            visible: root.draws

            onActivated: root.tools.shape = currentIndex
        }

        QuickButton {
            action: root.actions.copy
            label: qsTr("Copy")
            objectName: "copySelectionButton"
            shortcutText: AppInfo.shortcutText(root.actions.copy.shortcut)
            visible: root.picks
        }

        QuickButton {
            action: root.actions.paste
            label: qsTr("Paste")
            objectName: "pasteButton"
            shortcutText: AppInfo.shortcutText(root.actions.paste.shortcut)
            visible: root.picks
        }

        QuickButton {
            action: root.actions.remove
            label: qsTr("Delete")
            objectName: "deleteSelectionButton"
            shortcutText: AppInfo.shortcutText(root.actions.remove.shortcut)
            visible: root.picks
        }

        Label {
            color: palette.placeholderText
            text: qsTr("Shift keeps it even, Alt grows it from the middle")
            visible: root.draws && root.tools.shape !== ToolViewModel.Freehand
        }

        Label {
            color: palette.placeholderText
            text: qsTr("Drag the page to move it")
            visible: root.tools.currentTool === ToolViewModel.Hand
        }

        Item {
            Layout.fillWidth: true
        }

        QuickButton {
            action: root.actions.undo
            label: qsTr("Undo")
            objectName: "undoButton"
            shortcutText: AppInfo.shortcutText(root.actions.undo.shortcut)
        }

        QuickButton {
            Layout.rightMargin: 4
            action: root.actions.redo
            label: qsTr("Redo")
            objectName: "redoButton"
            shortcutText: AppInfo.shortcutText(root.actions.redo.shortcut)
        }
    }
}
