import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property AppActions actions
    readonly property NotebookViewModel notebook: root.actions.notebook

    objectName: "pageSetupPanel"
    padding: 8

    background: null

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            spacing: 12
            width: root.availableWidth

            PageSetup {
                Layout.fillWidth: true
                notebook: root.notebook
                visible: root.notebook !== null && root.notebook.loaded
            }

            Label {
                Layout.fillWidth: true
                color: palette.placeholderText
                font.pixelSize: Math.round(11 * Theme.scale)
                text: qsTr("The setup belongs to the whole section: every page in it is written on the same paper.")
                wrapMode: Text.WordWrap
            }

            EmptyPanelNote {
                Layout.fillWidth: true
                text: qsTr("Open a notebook to set its paper up.")
                visible: root.notebook === null || !root.notebook.loaded
            }
        }
    }
}
