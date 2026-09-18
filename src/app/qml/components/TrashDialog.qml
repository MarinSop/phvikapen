pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

AppDialog {
    id: root

    property NotebookViewModel notebook: null
    property bool confirming: false
    readonly property int trashCount: root.notebook === null ? 0 : root.notebook.trash.count

    height: 380
    objectName: "trashDialog"
    standardButtons: Dialog.Close
    title: qsTr("Deleted pages and sections")
    width: 460

    onOpened: {
        confirming = false;
        if (root.notebook !== null) {
            root.notebook.refreshTrash();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: root.trashCount === 0 ? qsTr("Nothing has been deleted") : qsTr("%1 waiting to be put back or deleted for good").arg(root.trashCount)
            wrapMode: Text.WordWrap
        }

        ListView {
            id: trashList

            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            model: root.notebook === null ? null : root.notebook.trash

            delegate: RowLayout {
                id: trashDelegate

                required property int index
                required property bool restorable
                required property string title
                required property bool wholeSection

                width: trashList.width

                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: trashDelegate.wholeSection ? qsTr("%1 (whole section)").arg(trashDelegate.title) : trashDelegate.title
                }

                Button {
                    enabled: trashDelegate.restorable
                    flat: true
                    objectName: "restoreButton"
                    text: qsTr("Put back")

                    onClicked: root.notebook.restoreTrashed(trashDelegate.index)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: root.confirming ? qsTr("This cannot be undone.") : ""
            }

            Button {
                enabled: root.trashCount > 0
                objectName: "emptyTrashButton"
                text: root.confirming ? qsTr("Delete for good") : qsTr("Empty the trash")

                onClicked: {
                    if (root.confirming) {
                        root.notebook.emptyTrash();
                        root.confirming = false;
                    } else {
                        root.confirming = true;
                    }
                }
            }
        }
    }
}
