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
    // A table in hand has a bar of its own. What is offered for ink picked with the loop stands
    // aside while it does, so that the bar is never two things at once.
    readonly property bool tables: root.actions.hasTable
    readonly property bool picksInk: root.picks && !root.tables
    // What the box in hand is already shown in, so the bar shows back what is set.
    readonly property color cellFill: root.actions.lookInHand.fill === undefined ? "transparent" : root.actions.lookInHand.fill
    readonly property color cellInk: root.actions.lookInHand.ink === undefined ? "transparent" : root.actions.lookInHand.ink
    readonly property list<int> shapes: [ToolViewModel.Line, ToolViewModel.Rectangle, ToolViewModel.Ellipse]
    readonly property list<int> eraserModes: [ToolViewModel.Touched, ToolViewModel.WholeStroke]
    readonly property var families: Qt.fontFamilies()
    readonly property string pickedText: root.notebook === null ? "" : root.notebook.pickedText
    readonly property bool types: root.tools.currentTool === ToolViewModel.Text || root.pickedText !== ""

    function applyColour(wanted) {
        if (root.canvas !== null && root.canvas.selectedCount > 0) {
            root.notebook.recolourSelection(wanted);
        } else {
            root.tools.strokeColor = wanted;
        }
    }

    // What the bar shows belongs to the box being worked on, where there is one.
    function applyText() {
        if (root.notebook !== null && root.pickedText !== "") {
            root.notebook.styleText(root.pickedText, root.tools.textStyle);
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
                color: root.tools.penColors[index]
                penWidth: root.tools.penWidths[index]

                onClicked: root.tools.pen = index
            }
        }

        ToolButton {
            id: colourButton

            leftPadding: 32
            objectName: "colorButton"
            text: qsTr("Color")
            visible: root.draws

            onClicked: colourMenu.popup()

            Rectangle {
                anchors.left: parent.left
                anchors.leftMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                color: Theme.line
                height: 24
                radius: height / 2
                width: 24

                Rectangle {
                    anchors.centerIn: parent
                    color: root.tools.strokeColor
                    height: 14
                    radius: height / 2
                    width: 14
                }
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

        NumberField {
            hasSlider: true
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

        // What the eraser takes: the part it is rubbed over, or the whole line at a touch. One
        // control says which, rather than two buttons that each say half of it.
        Label {
            color: palette.placeholderText
            text: qsTr("Erase")
            visible: root.erases
        }

        ComboBox {
            Layout.maximumWidth: 200
            currentIndex: Math.max(0, root.eraserModes.indexOf(root.tools.eraserMode))
            model: [qsTr("What it touches"), qsTr("The whole line")]
            objectName: "eraseModeField"
            visible: root.erases
            ToolTip.delay: 600
            ToolTip.text: qsTr("What the eraser takes away")
            ToolTip.visible: hovered

            onActivated: index => root.tools.eraserMode = root.eraserModes[index]
        }

        ComboBox {
            Layout.leftMargin: 4
            Layout.maximumWidth: 180
            currentIndex: Math.max(0, root.families.indexOf(root.tools.textFont === "" ? AppInfo.plainFont : root.tools.textFont))
            model: root.families
            objectName: "fontField"
            visible: root.types

            onActivated: index => {
                root.tools.textFont = root.families[index];
                root.applyText();
            }
        }

        Label {
            color: palette.placeholderText
            text: qsTr("Size")
            visible: root.types
        }

        NumberField {
            maximum: 144
            minimum: 6
            number: root.tools.textSize
            objectName: "textSizeField"
            step: 1
            visible: root.types

            onNumberEdited: value => {
                root.tools.textSize = value;
                root.applyText();
            }
        }

        ToolButton {
            leftPadding: 32
            objectName: "textColorButton"
            text: qsTr("Color")
            visible: root.types

            onClicked: {
                textColourDialog.selectedColor = root.tools.textColor;
                textColourDialog.open();
            }

            Rectangle {
                anchors.left: parent.left
                anchors.leftMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                color: Theme.line
                height: 24
                radius: height / 2
                width: 24

                Rectangle {
                    anchors.centerIn: parent
                    color: root.tools.textColor
                    height: 14
                    radius: height / 2
                    width: 14
                }
            }
        }

        Repeater {
            model: root.types ? 4 : 0

            ShapeButton {
                required property int index

                active: [root.tools.textBold, root.tools.textItalic, root.tools.textUnderline, root.tools.textStruckOut][index]
                icon.source: [Icons.bold, Icons.italic, Icons.underline, Icons.strikethrough][index]
                label: [qsTr("Bold"), qsTr("Italic"), qsTr("Underline"), qsTr("Strikethrough")][index]
                objectName: ["boldButton", "italicButton", "underlineButton", "strikeButton"][index]

                onClicked: {
                    if (index === 0) {
                        root.tools.textBold = !root.tools.textBold;
                    } else if (index === 1) {
                        root.tools.textItalic = !root.tools.textItalic;
                    } else if (index === 2) {
                        root.tools.textUnderline = !root.tools.textUnderline;
                    } else {
                        root.tools.textStruckOut = !root.tools.textStruckOut;
                    }
                    root.applyText();
                }
            }
        }

        Repeater {
            model: root.types ? 4 : 0

            ShapeButton {
                required property int index

                active: root.tools.textAlign === index
                icon.source: [Icons.alignLeft, Icons.alignCenter, Icons.alignRight, Icons.alignJustify][index]
                label: [qsTr("Align left"), qsTr("Center"), qsTr("Align right"), qsTr("Justify")][index]
                objectName: ["alignLeftButton", "alignCenterButton", "alignRightButton", "alignJustifyButton"][index]

                onClicked: {
                    root.tools.textAlign = index;
                    root.applyText();
                }
            }
        }

        QuickButton {
            action: root.actions.convertToText
            label: qsTr("To text")
            objectName: "convertToTextButton"
            shortcutText: AppInfo.shortcutText(root.actions.convertToText.shortcut)
            visible: root.picksInk
        }

        QuickButton {
            action: root.actions.solve
            label: qsTr("Solve")
            objectName: "solveButton"
            shortcutText: AppInfo.shortcutText(root.actions.solve.shortcut)
            visible: root.picksInk || root.types
        }

        ToolSeparator {
            visible: root.tables
        }

        Repeater {
            model: root.tables ? 2 : 0

            ShapeButton {
                required property int index

                active: [root.actions.lookInHand.bold === true, root.actions.lookInHand.italic === true][index]
                icon.source: [Icons.bold, Icons.italic][index]
                label: [qsTr("Bold"), qsTr("Italic")][index]
                objectName: ["cellBoldButton", "cellItalicButton"][index]

                onClicked: (index === 0 ? root.actions.boldCells : root.actions.italicCells).trigger()
            }
        }

        Repeater {
            model: root.tables ? 3 : 0

            ShapeButton {
                required property int index

                active: root.actions.lookInHand.align === index
                icon.source: [Icons.alignLeft, Icons.alignCenter, Icons.alignRight][index]
                label: [qsTr("Align left"), qsTr("Center"), qsTr("Align right")][index]
                objectName: ["cellAlignLeftButton", "cellAlignCenterButton", "cellAlignRightButton"][index]

                onClicked: root.actions.alignTheBoxesInHand(index)
            }
        }

        Repeater {
            model: root.tables ? 3 : 0

            ShapeButton {
                required property int index

                active: root.actions.lookInHand.rise === index
                icon.source: [Icons.sitAtTop, Icons.sitAtMiddle, Icons.sitAtFoot][index]
                label: [qsTr("Sit at top"), qsTr("Sit at middle"), qsTr("Sit at foot")][index]
                objectName: ["cellTopButton", "cellMiddleButton", "cellFootButton"][index]

                onClicked: root.actions.raiseTheBoxesInHand(index)
            }
        }

        ColourTap {
            colour: root.cellFill
            icon.source: Icons.cellColour
            label: qsTr("Colour behind the boxes")
            objectName: "cellFillButton"
            visible: root.tables

            onClicked: {
                cellFillDialog.selectedColor = root.cellFill.a > 0 ? root.cellFill : Theme.surface;
                cellFillDialog.open();
            }
        }

        ColourTap {
            colour: root.cellInk
            icon.source: Icons.inkColour
            label: qsTr("Colour of the words")
            objectName: "cellInkButton"
            visible: root.tables

            onClicked: {
                cellInkDialog.selectedColor = root.cellInk.a > 0 ? root.cellInk : Theme.text;
                cellInkDialog.open();
            }
        }

        Label {
            color: palette.placeholderText
            text: qsTr("Rules")
            visible: root.tables
        }

        NumberField {
            maximum: 8
            minimum: 1
            number: root.notebook === null || !root.tables ? 1 : Math.round(root.notebook.pickedTableBox.ruleWidth)
            objectName: "ruleWidthField"
            step: 1
            visible: root.tables

            onNumberEdited: value => root.notebook.ruleTable(root.actions.notebook.pickedTable, null, value)
        }

        QuickButton {
            action: root.actions.mergeCells
            label: qsTr("Merge boxes")
            objectName: "mergeCellsButton"
            visible: root.tables
        }

        QuickButton {
            action: root.actions.splitCell
            label: qsTr("Split box")
            objectName: "splitCellButton"
            visible: root.tables
        }

        ToolButton {
            objectName: "tableMenuButton"
            text: qsTr("Table")
            visible: root.tables

            onClicked: tableMenu.popup()

            Menu {
                id: tableMenu

                objectName: "tableMenu"

                MenuCommand {
                    action: root.actions.addRowAbove
                }

                MenuCommand {
                    action: root.actions.addRowBelow
                }

                MenuCommand {
                    action: root.actions.addColumnBefore
                }

                MenuCommand {
                    action: root.actions.addColumnAfter
                }

                MenuLine {
                }

                MenuCommand {
                    action: root.actions.duplicateRow
                }

                MenuCommand {
                    action: root.actions.duplicateColumn
                }

                MenuLine {
                }

                MenuCommand {
                    action: root.actions.removeRow
                }

                MenuCommand {
                    action: root.actions.removeColumn
                }

                MenuLine {
                }

                MenuCommand {
                    action: root.actions.clearCellLook
                }
            }
        }

        ToolSeparator {
            visible: root.tools.currentTool === ToolViewModel.Shape
        }

        Label {
            color: palette.placeholderText
            text: qsTr("Corners")
            visible: root.tools.currentTool === ToolViewModel.Shape
        }

        NumberField {
            hasSlider: true
            maximum: 60
            minimum: 0
            number: root.tools.corner
            objectName: "cornerField"
            step: 1
            visible: root.tools.currentTool === ToolViewModel.Shape

            onNumberEdited: value => root.tools.corner = value
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
            visible: root.picksInk
        }

        QuickButton {
            action: root.actions.duplicate
            label: qsTr("Duplicate")
            objectName: "duplicateButton"
            shortcutText: AppInfo.shortcutText(root.actions.duplicate.shortcut)
            visible: root.picksInk
        }

        QuickButton {
            action: root.actions.rotateLeft
            label: qsTr("Turn left")
            objectName: "turnLeftButton"
            shortcutText: AppInfo.shortcutText(root.actions.rotateLeft.shortcut)
            visible: root.picksInk
        }

        QuickButton {
            action: root.actions.rotateRight
            label: qsTr("Turn right")
            objectName: "turnRightButton"
            shortcutText: AppInfo.shortcutText(root.actions.rotateRight.shortcut)
            visible: root.picksInk
        }

        QuickButton {
            action: root.actions.paste
            label: qsTr("Paste")
            objectName: "pasteButton"
            shortcutText: AppInfo.shortcutText(root.actions.paste.shortcut)
            visible: root.picksInk
        }

        QuickButton {
            action: root.actions.remove
            label: qsTr("Delete")
            objectName: "deleteSelectionButton"
            shortcutText: AppInfo.shortcutText(root.actions.remove.shortcut)
            visible: root.picksInk
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
        id: textColourDialog

        objectName: "textColourDialog"
        options: ColorDialog.ShowAlphaChannel

        onAccepted: {
            root.tools.textColor = textColourDialog.selectedColor;
            root.applyText();
        }
    }

    Connections {
        function onPickedTextChanged() {
            if (root.pickedText === "") {
                root.tools.resetTextStyle();
            } else {
                root.tools.useTextStyle(root.notebook.styleOfText(root.pickedText));
            }
        }

        target: root.notebook
    }

    ColorDialog {
        id: colourDialog

        objectName: "colourDialog"
        options: ColorDialog.ShowAlphaChannel

        onAccepted: root.applyColour(colourDialog.selectedColor)
    }

    ColorDialog {
        id: cellFillDialog

        objectName: "cellFillDialog"
        options: ColorDialog.ShowAlphaChannel

        onAccepted: root.actions.fillTheBoxesInHand(cellFillDialog.selectedColor)
    }

    ColorDialog {
        id: cellInkDialog

        objectName: "cellInkDialog"
        options: ColorDialog.ShowAlphaChannel

        onAccepted: root.actions.inkTheBoxesInHand(cellInkDialog.selectedColor)
    }
}
