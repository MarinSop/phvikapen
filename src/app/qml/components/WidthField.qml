pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

// How thick the line is: a number with the two arrows stacked, and a slider a click away.
Control {
    id: root

    property real maximum: 24
    property real minimum: 0.5
    property real number: 1
    property real step: 0.5
    readonly property string shown: root.step < 1 ? root.number.toFixed(1) : root.number.toFixed(0)

    signal numberEdited(real value)

    function change(value) {
        const stepped = Math.round(value / root.step) * root.step;
        const held = Math.min(root.maximum, Math.max(root.minimum, stepped));
        if (Math.abs(held - root.number) > 1e-6) {
            root.numberEdited(held);
        }
    }

    implicitHeight: 32
    implicitWidth: 74
    padding: 0

    background: Rectangle {
        border.color: sliderPopup.opened ? Theme.accent : Theme.line
        border.width: 1
        color: Theme.base
        radius: 4
    }
    contentItem: RowLayout {
        spacing: 0

        Label {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.leftMargin: 8
            horizontalAlignment: Text.AlignLeft
            objectName: "widthNumber"
            text: root.shown
            verticalAlignment: Text.AlignVCenter

            TapHandler {
                onTapped: sliderPopup.opened ? sliderPopup.close() : sliderPopup.open()
            }
        }

        ColumnLayout {
            Layout.rightMargin: 2
            spacing: 0

            ArrowButton {
                enabled: root.number < root.maximum
                icon.source: Icons.chevronUp
                objectName: "widthUp"

                onClicked: root.change(root.number + root.step)
            }

            ArrowButton {
                enabled: root.number > root.minimum
                icon.source: Icons.chevronDown
                objectName: "widthDown"

                onClicked: root.change(root.number - root.step)
            }
        }
    }

    MouseArea {
        acceptedButtons: Qt.NoButton
        anchors.fill: parent

        onWheel: wheel => root.change(root.number + (wheel.angleDelta.y > 0 ? root.step : -root.step))
    }

    Popup {
        id: sliderPopup

        objectName: "widthSlider"
        padding: 6
        width: 170
        y: root.height + 4

        background: Rectangle {
            border.color: Theme.line
            border.width: 1
            color: Theme.surface
            radius: 8
        }

        RowLayout {
            anchors.fill: parent
            spacing: 6

            Slider {
                id: slider

                Layout.fillWidth: true
                from: root.minimum
                objectName: "widthSliderBar"
                to: root.maximum
                value: root.number

                onMoved: root.change(slider.value)
            }

            Label {
                Layout.minimumWidth: 24
                horizontalAlignment: Text.AlignRight
                text: root.shown
            }
        }
    }
}
