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

        // The clock stays in sight while it is counting, whether or not its panel is open.
        QuickButton {
            id: timeShown

            Accessible.name: qsTr("Time %1").arg(root.actions.timeKeeper.said)
            display: AbstractButton.TextBesideIcon
            icon.color: root.actions.timeKeeper.rang ? Theme.warning : Theme.text
            icon.source: Icons.time
            label: qsTr("Time")
            objectName: "timeInTheBar"
            text: root.actions.timeKeeper.said
            visible: root.actions.timeKeeper.started || root.actions.timeKeeper.rang

            onClicked: {
                root.actions.timeKeeper.seen();
                root.actions.workspace.showPanel(Panels.time);
            }
        }

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
