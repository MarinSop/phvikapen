import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

ToolBar {
    id: root

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
            text: qsTr("Clear")

            onClicked: root.tools.clearRequested()
        }
    }
}
