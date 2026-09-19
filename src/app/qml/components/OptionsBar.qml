pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import PhvikaPen.Ui

ToolBar {
    id: root

    required property AppActions actions
    required property ToolViewModel tools
    readonly property InkCanvas canvas: root.actions.canvas
    readonly property bool draws: root.tools.currentTool === ToolViewModel.Pen || root.tools.currentTool === ToolViewModel.Highlighter || root.tools.currentTool === ToolViewModel.Shape
    readonly property bool erases: root.tools.currentTool === ToolViewModel.Eraser
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property bool picks: root.tools.currentTool === ToolViewModel.Selection
    readonly property list<int> shapes: [ToolViewModel.Line, ToolViewModel.Rectangle, ToolViewModel.Ellipse]

    function applyColour(wanted) {
        if (root.canvas !== null && root.canvas.selectedCount > 0) {
            root.notebook.recolourSelection(wanted);
        } else {
            root.tools.strokeColor = wanted;
        }
    }

    objectName: "optionsBar"

    background: Rectangle {
        color: Theme.surface

        Rectangle {
            anchors.bottom: parent.bottom
            color: Theme.line
            height: 1
            width: parent.width
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 6

        Repeater {
            model: root.draws && root.tools.currentTool !== ToolViewModel.Highlighter ? root.tools.penCount : 0

            PenSwatch {
                required property int index

                Layout.leftMargin: index === 0 ? 4 : 0
                checked: root.tools.pen === index
                color: root.tools.colorOfPen(index)
                penWidth: root.tools.widthOfPen(index)

                onClicked: root.tools.pen = index
            }
        }

        ToolButton {
            id: colourButton

            leftPadding: 30
            objectName: "colorButton"
            text: qsTr("Color")
            visible: root.draws

            onClicked: colourMenu.popup()

            Rectangle {
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                border.color: Qt.rgba(0, 0, 0, 0.35)
                border.width: 1
                color: root.tools.strokeColor
                height: 16
                radius: height / 2
                width: 16
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
                                    root.applyColour(swatch.modelData);
                                    colourMenu.close();
                                }
                            }
                        }
                    }
                }

                MenuSeparator {
                }

                MenuItem {
                    objectName: "moreColoursItem"
                    text: qsTr("More colors…")

                    onTriggered: {
                        colourDialog.selectedColor = root.tools.strokeColor;
                        colourDialog.open();
                    }
                }
            }
        }

        Label {
            Layout.leftMargin: 2
            color: palette.placeholderText
            text: root.erases ? qsTr("Size") : qsTr("Width")
            visible: root.draws || root.erases
        }

        WidthField {
            maximum: root.erases ? 40 : 24
            minimum: root.erases ? 4 : 0.5
            number: root.erases ? root.tools.eraserRadius : root.tools.strokeWidth
            objectName: "widthField"
            step: root.erases ? 1 : 0.5
            visible: root.draws || root.erases

            onNumberEdited: value => {
                if (root.erases) {
                    root.tools.eraserRadius = value;
                } else {
                    root.tools.strokeWidth = value;
                }
            }
        }

        ToolSeparator {
            visible: root.tools.currentTool === ToolViewModel.Shape
        }

        Repeater {
            model: root.tools.currentTool === ToolViewModel.Shape ? 3 : 0

            ShapeButton {
                required property int index

                active: root.tools.shape === root.shapes[index]
                icon.source: [Icons.straightLine, Icons.square, Icons.circle][index]
                label: [qsTr("Straight line"), qsTr("Box"), qsTr("Circle")][index]
                objectName: ["lineShapeButton", "boxShapeButton", "circleShapeButton"][index]

                onClicked: root.tools.shape = root.shapes[index]
            }
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

    ColorDialog {
        id: colourDialog

        objectName: "colourDialog"
        options: ColorDialog.ShowAlphaChannel

        onAccepted: root.applyColour(colourDialog.selectedColor)
    }
}
