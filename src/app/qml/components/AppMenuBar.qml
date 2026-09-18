pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

MenuBar {
    id: root

    required property AppActions actions
    required property NotebooksViewModel notebooks
    required property bool pagesPanelShown
    required property bool pagePanelShown

    signal pagesPanelToggled(bool shown)
    signal pagePanelToggled(bool shown)

    Menu {
        objectName: "fileMenu"
        title: qsTr("&File")

        MenuCommand {
            action: root.actions.newNotebook
            objectName: "newNotebookItem"
        }

        Menu {
            objectName: "openNotebookMenu"
            title: qsTr("Open Notebook")

            Repeater {
                model: root.notebooks.library

                MenuItem {
                    required property string modelData

                    text: modelData

                    onTriggered: root.notebooks.openNotebook(modelData)
                }
            }
        }

        MenuSeparator {
        }

        MenuCommand {
            action: root.actions.saveCopy
            objectName: "saveCopyItem"
        }

        MenuCommand {
            action: root.actions.exportPdf
            objectName: "exportPdfItem"
        }

        MenuSeparator {
        }

        MenuCommand {
            action: root.actions.closeNotebook
        }

        MenuCommand {
            action: root.actions.quit
        }
    }

    Menu {
        objectName: "editMenu"
        title: qsTr("&Edit")

        MenuCommand {
            action: root.actions.undo
            objectName: "undoItem"
        }

        MenuCommand {
            action: root.actions.redo
            objectName: "redoItem"
        }

        MenuSeparator {
        }

        MenuCommand {
            action: root.actions.copy
        }

        MenuCommand {
            action: root.actions.paste
        }

        MenuCommand {
            action: root.actions.remove
        }

        MenuSeparator {
        }

        MenuCommand {
            action: root.actions.clearPage
        }

        MenuCommand {
            action: root.actions.showTrash
            objectName: "trashItem"
        }
    }

    Menu {
        objectName: "viewMenu"
        title: qsTr("&View")

        MenuCommand {
            action: root.actions.zoomIn
        }

        MenuCommand {
            action: root.actions.zoomOut
        }

        MenuCommand {
            action: root.actions.fitPage
        }

        MenuSeparator {
        }

        MenuCommand {
            action: root.actions.previousPage
        }

        MenuCommand {
            action: root.actions.nextPage
        }

        MenuSeparator {
        }

        MenuItem {
            checkable: true
            checked: root.pagesPanelShown
            objectName: "pagesPanelItem"
            text: qsTr("Pages Panel")

            onTriggered: root.pagesPanelToggled(!root.pagesPanelShown)
        }

        MenuItem {
            checkable: true
            checked: root.pagePanelShown
            objectName: "pagePanelItem"
            text: qsTr("Page Panel")

            onTriggered: root.pagePanelToggled(!root.pagePanelShown)
        }
    }

    Menu {
        objectName: "insertMenu"
        title: qsTr("&Insert")

        MenuCommand {
            action: root.actions.addPage
        }

        MenuCommand {
            action: root.actions.addSection
        }

        MenuCommand {
            action: root.actions.duplicatePage
        }

        MenuSeparator {
        }

        MenuCommand {
            action: root.actions.importDocument
            objectName: "importItem"
        }
    }

    Menu {
        objectName: "toolsMenu"
        title: qsTr("&Tools")

        Repeater {
            model: root.actions.toolActions

            MenuCommand {
                required property Action modelData

                action: modelData
            }
        }

        MenuSeparator {
        }

        MenuCommand {
            action: root.actions.showSettings
            objectName: "settingsItem"
        }
    }

    Menu {
        objectName: "windowMenu"
        title: qsTr("&Window")

        Repeater {
            model: root.notebooks.openNotebooks

            MenuItem {
                required property int index
                required property string modelData

                checkable: true
                checked: index === root.notebooks.currentIndex
                text: modelData

                onTriggered: root.notebooks.currentIndex = index
            }
        }
    }

    Menu {
        objectName: "helpMenu"
        title: qsTr("&Help")

        MenuCommand {
            action: root.actions.showAbout
        }
    }
}
