pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property AppActions actions
    required property NotebooksViewModel notebooks

    objectName: "emptyState"

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 12

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 4
            Layout.preferredHeight: 112
            Layout.preferredWidth: 112
            fillMode: Image.PreserveAspectFit
            mipmap: true
            objectName: "emptyLogo"
            opacity: 0.95
            source: Theme.logo
            sourceSize.height: 224
            sourceSize.width: 224
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            font.bold: true
            font.pixelSize: 18
            text: qsTr("No notebook is open")
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            color: palette.placeholderText
            text: root.notebooks.library.length === 0 ? qsTr("Make one to start writing.") : qsTr("Make a new one, or open one that is already there.")
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8

            Button {
                action: root.actions.newNotebook
                objectName: "emptyNewButton"
            }

            Button {
                enabled: root.notebooks.library.length > 0
                objectName: "emptyOpenButton"
                text: qsTr("Open…")

                onClicked: openMenu.popup()

                Menu {
                    id: openMenu

                    Repeater {
                        model: root.notebooks.library

                        MenuItem {
                            required property string modelData

                            text: modelData

                            onTriggered: root.notebooks.openNotebook(modelData)
                        }
                    }
                }
            }
        }
    }
}
