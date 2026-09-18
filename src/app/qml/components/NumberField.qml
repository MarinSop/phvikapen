import QtQuick
import QtQuick.Controls

SpinBox {
    id: root

    property real maximum: 100
    property real minimum: 0
    property real number: 0
    property real step: 1
    property string suffix: ""

    signal numberEdited(real value)

    editable: true
    from: Math.round(root.minimum / root.step)
    implicitWidth: 120
    stepSize: 1
    to: Math.round(root.maximum / root.step)
    value: Math.round(root.number / root.step)
    textFromValue: (value, locale) => Number(value * root.step).toLocaleString(locale, 'f', root.step < 1 ? 1 : 0) + root.suffix
    valueFromText: (text, locale) => Math.round(Number.fromLocaleString(locale, text.replace(root.suffix, "").trim()) / root.step)

    onValueModified: root.numberEdited(root.value * root.step)
}
