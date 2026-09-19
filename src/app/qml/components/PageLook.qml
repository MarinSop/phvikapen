pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

// How a page is ruled: the paper, the lines on it and the line down the side.
ColumnLayout {
    id: root

    property color lineColor
    property real lineWidth: 1
    property bool lined: true
    property color marginColor
    property bool marginShown: true
    property real marginAt: 25
    property color paperColor
    property real lineSpacing: 7

    signal lineColorPicked(color wanted)
    signal lineWidthChosen(real wanted)
    signal marginAtChosen(real wanted)
    signal marginColorPicked(color wanted)
    signal marginShownChosen(bool wanted)
    signal paperColorPicked(color wanted)
    signal lineSpacingChosen(real wanted)

    spacing: 6

    ColorField {
        Layout.fillWidth: true
        chosen: root.paperColor
        label: qsTr("Paper")
        objectName: "paperColorField"

        onPicked: wanted => root.paperColorPicked(wanted)
    }

    ColorField {
        Layout.fillWidth: true
        chosen: root.lineColor
        label: qsTr("Lines")
        objectName: "lineColorField"

        onPicked: wanted => root.lineColorPicked(wanted)
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 6

        Label {
            Layout.fillWidth: true
            text: qsTr("Line width")
        }

        NumberField {
            hasSlider: true
            maximum: 6
            minimum: 0.5
            number: root.lineWidth
            objectName: "lineWidthField"
            step: 0.5

            onNumberEdited: value => root.lineWidthChosen(value)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 6

        Label {
            Layout.fillWidth: true
            text: qsTr("Spacing")
        }

        NumberField {
            hasSlider: true
            maximum: 30
            minimum: 2
            number: root.lineSpacing
            objectName: "spacingField"
            step: 1
            suffix: qsTr(" mm")

            onNumberEdited: value => root.lineSpacingChosen(value)
        }
    }

    Switch {
        Layout.fillWidth: true
        checked: root.marginShown
        enabled: root.lined
        objectName: "marginSwitch"
        opacity: enabled ? 1 : 0.4
        text: qsTr("Line down the side")

        onToggled: root.marginShownChosen(checked)
    }

    ColorField {
        Layout.fillWidth: true
        chosen: root.marginColor
        enabled: root.lined && root.marginShown
        label: qsTr("Side line")
        objectName: "marginColorField"
        opacity: enabled ? 1 : 0.4

        onPicked: wanted => root.marginColorPicked(wanted)
    }

    RowLayout {
        Layout.fillWidth: true
        enabled: root.lined && root.marginShown
        opacity: enabled ? 1 : 0.4
        spacing: 6

        Label {
            Layout.fillWidth: true
            text: qsTr("Side line at")
        }

        NumberField {
            hasSlider: true
            maximum: 120
            minimum: 0
            number: root.marginAt
            objectName: "marginAtField"
            step: 1
            suffix: qsTr(" mm")

            onNumberEdited: value => root.marginAtChosen(value)
        }
    }
}
