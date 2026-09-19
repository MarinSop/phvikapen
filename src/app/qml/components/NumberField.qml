pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

// A number with the two arrows stacked beside it, and, where it helps, a slider a click away.
Control {
    id: root

    property bool hasSlider: false
    property real maximum: 100
    property real minimum: 0
    property real number: 0
    readonly property string shown: root.step < 1 ? root.number.toFixed(1) : root.number.toFixed(0)
    property real step: 1
    property string suffix: ""

    signal numberEdited(real value)

    function change(value) {
        const stepped = Math.round(value / root.step) * root.step;
        const held = Math.min(root.maximum, Math.max(root.minimum, stepped));
        if (Math.abs(held - root.number) > 1e-6) {
            root.numberEdited(held);
        }
    }

    implicitHeight: 32
    implicitWidth: 92
    padding: 0

    background: Rectangle {
        border.color: field.activeFocus || sliderPopup.opened ? Theme.accent : Theme.line
        border.width: 1
        color: Theme.base
        radius: 4
    }
    contentItem: RowLayout {
        spacing: 0

        TextField {
            id: field

            Layout.fillHeight: true
            Layout.fillWidth: true
            background: null
            horizontalAlignment: Text.AlignLeft
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            leftPadding: 8
            objectName: "numberText"
            rightPadding: 0
            text: root.shown + root.suffix
            verticalAlignment: Text.AlignVCenter

            onActiveFocusChanged: {
                if (field.activeFocus) {
                    if (root.hasSlider) {
                        sliderPopup.open();
                    }
                    return;
                }
                field.text = Qt.binding(() => root.shown + root.suffix);
            }
            onEditingFinished: root.change(Number(field.text.replace(root.suffix, "").trim()))
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

        // Leaving the slider leaves the number as well, so the keys belong to the page again.
        onClosed: field.focus = false

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
