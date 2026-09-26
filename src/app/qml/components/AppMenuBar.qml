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
            action: root.actions.cut
            objectName: "cutItem"
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
            action: root.actions.solve
            objectName: "solveItem"
        }

        MenuCommand {
            action: root.actions.paste
        }

        MenuCommand {
            action: root.actions.remove
        }

        MenuCommand {
            action: root.actions.duplicate
            objectName: "duplicateItem"
        }

        MenuCommand {
            action: root.actions.selectAll
            objectName: "selectAllItem"
        }

        MenuSeparator {
        }

        Menu {
            objectName: "arrangeMenu"
            title: qsTr("Arrange")

            MenuCommand {
                action: root.actions.rotateLeft
                objectName: "rotateLeftItem"
            }

            MenuCommand {
                action: root.actions.rotateRight
                objectName: "rotateRightItem"
            }
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
            objectName: "panelsMenu"
            title: qsTr("Panels")

            MenuCommand {
                action: root.actions.showSectionsPanel
                objectName: "sectionsPanelItem"
            }

            MenuCommand {
                action: root.actions.showPagesPanel
                objectName: "pagesPanelItem"
            }

            MenuCommand {
                action: root.actions.showLayersPanel
                objectName: "layersPanelItem"
            }

            MenuCommand {
                action: root.actions.showPageSetupPanel
                objectName: "pageSetupPanelItem"
            }

            MenuCommand {
                action: root.actions.showMathsPanel
                objectName: "mathsPanelItem"
            }

            MenuCommand {
                action: root.actions.showTimePanel
                objectName: "timePanelItem"
            }

            MenuCommand {
                action: root.actions.showSoundPanel
                objectName: "soundPanelItem"
            }

            MenuSeparator {
            }

            MenuCommand {
                action: root.actions.resetPanels
                objectName: "resetPanelsItem"
            }
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
            action: root.actions.insertPicture
            objectName: "insertPictureItem"
        }

        MenuCommand {
            action: root.actions.insertTable
            objectName: "insertTableItem"
        }

        MenuCommand {
            action: root.actions.insertEquation
            objectName: "insertEquationItem"
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
