pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

// How wide and how tall a sheet is, in whichever unit the reader thinks in.
GridLayout {
    id: root

    property real heightMillimetres: 297
    readonly property list<real> perMillimetre: [1, 0.1, 0.0393701, 3.77953]
    readonly property list<string> unitNames: [qsTr("mm"), qsTr("cm"), qsTr("in"), qsTr("px")]
    readonly property real unitStep: unitBox.currentIndex === 3 ? 1 : unitBox.currentIndex === 0 ? 1 : 0.5
    property real widthMillimetres: 210

    signal sizeEdited(real wide, real tall)

    function inMillimetres(value) {
        return value / root.perMillimetre[unitBox.currentIndex];
    }

    function inUnit(millimetres) {
        return millimetres * root.perMillimetre[unitBox.currentIndex];
    }

    // In a narrow panel the two sides stand one above the other.
    columnSpacing: 6
    columns: root.width < 300 ? 2 : 5
    rowSpacing: 4

    Label {
        text: qsTr("w:")
    }

    NumberField {
        Layout.fillWidth: true
        Layout.minimumWidth: 84
        maximum: root.inUnit(2000)
        minimum: root.inUnit(10)
        number: root.inUnit(root.widthMillimetres)
        objectName: "sizeWidth"
        step: root.unitStep

        onNumberEdited: value => root.sizeEdited(root.inMillimetres(value), root.heightMillimetres)
    }

    Label {
        text: qsTr("h:")
    }

    NumberField {
        Layout.fillWidth: true
        Layout.minimumWidth: 84
        maximum: root.inUnit(2000)
        minimum: root.inUnit(10)
        number: root.inUnit(root.heightMillimetres)
        objectName: "sizeHeight"
        step: root.unitStep

        onNumberEdited: value => root.sizeEdited(root.widthMillimetres, root.inMillimetres(value))
    }

    ComboBox {
        id: unitBox

        Layout.columnSpan: root.columns === 2 ? 2 : 1
        Layout.fillWidth: root.columns === 2
        Layout.preferredWidth: root.columns === 2 ? -1 : 78
        model: root.unitNames
        objectName: "sizeUnit"
    }
}
