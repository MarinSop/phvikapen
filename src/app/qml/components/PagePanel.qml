import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property AppActions actions
    readonly property NotebookViewModel notebook: root.actions.notebook

    objectName: "pagePanel"
    padding: 8

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            Layout.fillWidth: true
            font.bold: true
            text: qsTr("Page")
        }

        PageSetup {
            Layout.fillWidth: true
            notebook: root.notebook
            visible: root.notebook !== null && root.notebook.loaded
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
