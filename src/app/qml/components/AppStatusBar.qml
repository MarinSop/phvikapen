import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

ToolBar {
    id: root

    required property AppActions actions
    readonly property InkCanvas canvas: root.actions.canvas
    readonly property NotebookViewModel notebook: root.actions.notebook

    objectName: "statusBar"

    background: Rectangle {
        color: Theme.surfaceStrong

        Rectangle {
            color: Theme.line
            height: 1
            width: parent.width
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 6

        Item {
            Layout.fillWidth: true
        }

        QuickButton {
            action: root.actions.previousPage
            label: qsTr("Previous page")
            objectName: "previousPageButton"
        }

        Label {
            objectName: "pageLabel"
            text: root.notebook === null ? "" : qsTr("Page %1 of %2").arg(root.notebook.currentPage + 1).arg(root.notebook.pageCount)
        }

        QuickButton {
            action: root.actions.nextPage
            label: qsTr("Next page")
            objectName: "nextPageButton"
        }

        ToolSeparator {
        }

        QuickButton {
            action: root.actions.zoomOut
            label: qsTr("Zoom out")
            objectName: "zoomOutButton"
        }

        Label {
            Layout.minimumWidth: 44
            horizontalAlignment: Text.AlignHCenter
            objectName: "zoomLabel"
            text: root.canvas === null ? "" : Math.round(root.canvas.zoom * 100) + "%"
        }

        QuickButton {
            action: root.actions.zoomIn
            label: qsTr("Zoom in")
            objectName: "zoomInButton"
        }

        QuickButton {
            Layout.rightMargin: 8
            action: root.actions.fitPage
            label: qsTr("Fit page")
            objectName: "fitPageButton"
        }
    }
}
