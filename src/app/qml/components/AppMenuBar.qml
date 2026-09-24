pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

MenuBar {
    id: root

    required property AppActions actions
    required property NotebooksViewModel notebooks

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
            action: root.actions.save
            objectName: "saveItem"
        }

        MenuCommand {
            action: root.actions.saveCopy
            objectName: "saveCopyItem"
        }

        Menu {
            objectName: "exportMenu"
            title: qsTr("Export as PDF")

            MenuCommand {
                action: root.actions.exportEverything
                objectName: "exportPdfItem"
            }

            MenuCommand {
                action: root.actions.exportSheets
                objectName: "exportSheetsItem"
            }

            MenuCommand {
                action: root.actions.exportImported
                objectName: "exportImportedItem"
            }
        }

        MenuSeparator {
        }

        MenuCommand {
            action: root.actions.pageSetup
            objectName: "pageSetupItem"
        }

        MenuCommand {
            action: root.actions.showSettings
            objectName: "settingsItem"
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
            action: root.actions.copyAsText
            objectName: "copyAsTextItem"
        }

        MenuCommand {
            action: root.actions.convertToText
            objectName: "convertToTextItem"
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

        MenuSeparator {
        }

        MenuCommand {
            action: root.actions.findWriting
            objectName: "findItem"
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

        MenuCommand {
            action: root.actions.continuousPages
            objectName: "continuousPagesItem"
        }

        Menu {
            objectName: "contentsMenu"
            title: qsTr("Contents Panel")

            MenuCommand {
                action: root.actions.sectionsList
                objectName: "sectionsListItem"
            }

            MenuCommand {
                action: root.actions.pagesList
                objectName: "pagesListItem"
            }
        }

        MenuCommand {
            action: root.actions.pagePanel
            objectName: "pagePanelItem"
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
        objectName: "helpMenu"
        title: qsTr("&Help")

        MenuCommand {
            action: root.actions.showHints
            objectName: "hintsItem"
        }

        MenuCommand {
            action: root.actions.showAbout
        }
    }
}
