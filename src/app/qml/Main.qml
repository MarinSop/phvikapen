pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

ApplicationWindow {
    id: root

    height: 800
    title: qsTr("PhvikaPen %1").arg(AppInfo.version)
    visible: true
    width: 1280

    header: InkToolBar {
        notebook: notebookModel
        tools: toolState
    }

    ToolViewModel {
        id: toolState
    }

    NotebooksViewModel {
        id: notebooks
    }

    // TODO(M3): One model per notebook.
    NotebookViewModel {
        id: notebookModel
    }

    Shortcut {
        sequences: [StandardKey.Undo]

        onActivated: notebookModel.undo()
    }

    Shortcut {
        sequences: [StandardKey.Redo]

        onActivated: notebookModel.redo()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabBar

            Layout.fillWidth: true

            Repeater {
                model: notebooks.titles

                TabButton {
                    required property string modelData

                    text: modelData
                }
            }
        }

        StackLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            currentIndex: tabBar.currentIndex

            Repeater {
                model: notebooks.titles

                NotebookPage {
                    notebook: notebookModel
                    tools: toolState
                }
            }
        }
    }
}
