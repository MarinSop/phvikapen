pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import PhvikaPen.Ui

// A colour to pick, with the choice of leaving it to the application.
RowLayout {
    id: root

    property color chosen
    required property string label
    // An invalid colour means nobody chose one, and the usual colour is used.
    readonly property bool usual: !root.chosen.valid

    signal picked(color wanted)

    spacing: 6

    Label {
        Layout.fillWidth: true
        elide: Text.ElideRight
        text: root.label
    }

    Button {
        id: swatch

        Layout.preferredWidth: 46
        objectName: "colorButton"

        background: Rectangle {
            border.color: swatch.hovered ? Theme.accent : Theme.line
            border.width: 1
            color: Theme.base
            radius: 6

            Rectangle {
                anchors.centerIn: parent
                border.color: Theme.line
                border.width: 1
                color: root.usual ? "transparent" : root.chosen
                height: 18
                radius: 4
                width: 26

                Rectangle {
                    anchors.centerIn: parent
                    color: Theme.subtleText
                    height: 1
                    rotation: -20
                    visible: root.usual
                    width: parent.width - 6
                }
            }
        }

        onClicked: {
            picker.selectedColor = root.usual ? "#ffffff" : root.chosen;
            picker.open();
        }
    }

    ToolButton {
        enabled: !root.usual
        objectName: "clearColorButton"
        text: qsTr("Usual")
        // A colour with nothing in it is how "let the application choose" is handed over.

        onClicked: root.picked("transparent")
    }

    ColorDialog {
        id: picker

        title: root.label

        onAccepted: root.picked(picker.selectedColor)
    }
}
