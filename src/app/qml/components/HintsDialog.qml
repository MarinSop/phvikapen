pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

AppDialog {
    id: root

    readonly property list<string> hintKeys: [qsTr("Shift while drawing a shape"), qsTr("Alt while drawing a shape"), qsTr("Shift with the straight line"), qsTr("Drag inside the marquee"), qsTr("Ctrl and the wheel"), qsTr("Shift and the wheel"), qsTr("Two fingers"), qsTr("Pinch"), qsTr("The eraser"), qsTr("The pen of a tablet turned over"), qsTr("A tap with the text tool"), qsTr("The bar above a box of text"), qsTr("The grip beside a box of text"), qsTr("Esc while typing"), qsTr("A right click, or the button on the pen"), qsTr("Holding the pen still"), qsTr("A corner grip of the frame"), qsTr("An edge grip of the frame"), qsTr("The knob above the frame"), qsTr("Shift while turning")]
    readonly property list<string> hintMeanings: [qsTr("Keeps it even: a square, a circle"), qsTr("Grows it from the middle instead of from the corner"), qsTr("Snaps the line to every 45 degrees"), qsTr("Moves what is picked; Delete removes it"), qsTr("Zooms in and out around the pointer"), qsTr("Moves the page sideways"), qsTr("Move the page"), qsTr("Zooms in and out"), qsTr("Takes what it is rubbed over, or the whole line at a touch"), qsTr("Erases while it is held that way"), qsTr("Puts a box there to type in; an empty box is dropped"), qsTr("Carries the box about the page"), qsTr("Says how wide the writing may run"), qsTr("Leaves the box as it is"), qsTr("Opens the menu of what can be done right here"), qsTr("Opens the same menu, and can be turned off in Settings"), qsTr("Makes what is picked larger or smaller, in proportion"), qsTr("Stretches what is picked that way only"), qsTr("Turns what is picked"), qsTr("Turns it in steps of fifteen degrees")]

    objectName: "hintsDialog"
    standardButtons: Dialog.Close
    title: qsTr("Keys and hints")
    width: 520

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            Layout.fillWidth: true
            color: palette.placeholderText
            text: qsTr("The keys of every command can be changed in Settings.")
            wrapMode: Text.WordWrap
        }

        Repeater {
            model: root.hintKeys.length

            RowLayout {
                id: hintRow

                required property int index

                Layout.fillWidth: true
                spacing: 12

                Label {
                    Layout.preferredWidth: 220
                    font.bold: true
                    text: root.hintKeys[hintRow.index]
                    wrapMode: Text.WordWrap
                }

                Label {
                    Layout.fillWidth: true
                    text: root.hintMeanings[hintRow.index]
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
