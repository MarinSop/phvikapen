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
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property bool draws: root.tools.currentTool === ToolViewModel.Pen || root.tools.currentTool === ToolViewModel.Highlighter || root.tools.currentTool === ToolViewModel.Shape
    readonly property bool erases: root.tools.currentTool === ToolViewModel.Eraser
    readonly property bool picks: root.tools.currentTool === ToolViewModel.Selection
    readonly property list<int> shapes: [ToolViewModel.Line, ToolViewModel.Rectangle, ToolViewModel.Ellipse]

    function applyColour(wanted) {
        if (root.canvas !== null && root.canvas.selectedCount > 0) {
            root.notebook.recolourSelection(wanted);
        } else {
            root.tools.strokeColor = wanted;
        }
    }

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
        case ToolViewModel.Shape:
            return qsTr("Shape");
        case ToolViewModel.ColourPicker:
            return qsTr("Colour Picker");
        default:
            return qsTr("Pen");
        }
    }

    objectName: "optionsBar"

    background: Rectangle {
        color: Theme.shaded(palette.window, Theme.weakStep)

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

        Label {
            Layout.leftMargin: 4
            Layout.minimumWidth: 90
            font.bold: true
            text: root.toolTitle()
        }

        ToolSeparator {
        }

        Repeater {
            model: root.draws && root.tools.currentTool !== ToolViewModel.Highlighter ? root.tools.penCount : 0

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
                    text: qsTr("More colours…")

                    onTriggered: {
                        colourDialog.selectedColor = root.tools.strokeColor;
                        colourDialog.open();
                    }
                }
            }
        }

        Label {
            text: root.erases ? qsTr("Size") : qsTr("Width")
            visible: root.draws || root.erases
        }

        NumberField {
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

        ComboBox {
            id: shapeBox

            Layout.preferredWidth: 140
            currentIndex: root.shapes.indexOf(root.tools.shape)
            model: [qsTr("Straight line"), qsTr("Box"), qsTr("Circle")]
            objectName: "shapeBox"
            visible: root.tools.currentTool === ToolViewModel.Shape

            onActivated: root.tools.shape = root.shapes[shapeBox.currentIndex]
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
