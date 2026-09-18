pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import PhvikaPen.Ui

ApplicationWindow {
    id: root

    property bool pagePanelShown: true
    property bool pagesPanelShown: true
    readonly property NotebookViewModel notebook: notebooks.current

    height: 800
    title: root.notebook === null ? qsTr("PhvikaPen %1").arg(AppInfo.version) : qsTr("%1 — PhvikaPen %2").arg(root.notebook.title).arg(AppInfo.version)
    visible: true
    width: 1280

    footer: AppStatusBar {
        actions: appActions
    }
    header: ColumnLayout {
        spacing: 0

        NotebookTabs {
            Layout.fillWidth: true
            notebooks: notebooks
        }

        OptionsBar {
            Layout.fillWidth: true
            actions: appActions
            tools: toolState
        }

        UpdateBar {
            Layout.fillWidth: true
            updates: updates
        }
    }
    menuBar: AppMenuBar {
        actions: appActions
        notebooks: notebooks
        pagePanelShown: root.pagePanelShown
        pagesPanelShown: root.pagesPanelShown

        onPagePanelToggled: shown => root.pagePanelShown = shown
        onPagesPanelToggled: shown => root.pagesPanelShown = shown
    }

    Component.onCompleted: notebooks.canvas = canvas

    ToolViewModel {
        id: toolState
    }

    SettingsViewModel {
        id: settings
    }

    NotebooksViewModel {
        id: notebooks

        onErrorMessage: message => messageBar.show(message)
    }

    UpdateViewModel {
        id: updates

        Component.onCompleted: {
            if (settings.lookForUpdates) {
                updates.check();
            }
        }
        onRestartWanted: Qt.quit()
    }

    AppActions {
        id: appActions

        notebooks: notebooks
        tools: toolState

        onAboutWanted: aboutDialog.open()
        onCopyWanted: {
            const folder = StandardPaths.writableLocation(StandardPaths.DocumentsLocation);
            copyDialog.currentFolder = folder;
            copyDialog.selectedFile = folder + "/" + root.notebook.title + ".phvika";
            copyDialog.open();
        }
        onExportWanted: {
            const folder = StandardPaths.writableLocation(StandardPaths.DocumentsLocation);
            exportDialog.currentFolder = folder;
            exportDialog.selectedFile = folder + "/" + root.notebook.title + ".pdf";
            exportDialog.open();
        }
        onImportWanted: importDialog.open()
        onNewNotebookWanted: notebooks.createNotebook(notebooks.suggestedName())
        onSettingsWanted: settingsDialog.open()
        onTrashWanted: trashDialog.open()
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        ToolPalette {
            Layout.fillHeight: true
            actions: appActions
        }

        PagesPanel {
            Layout.fillHeight: true
            Layout.preferredWidth: 220
            actions: appActions
            visible: root.pagesPanelShown
        }

        InkCanvas {
            id: canvas

            Layout.fillHeight: true
            Layout.fillWidth: true
            enabled: root.notebook !== null && root.notebook.loaded
            eraserRadius: toolState.eraserRadius
            erasing: toolState.currentTool === ToolViewModel.Eraser
            panning: toolState.currentTool === ToolViewModel.Hand
            pressureSensitive: toolState.pressureSensitive
            selecting: toolState.currentTool === ToolViewModel.Selection
            shape: toolState.shape
            strokeColor: toolState.strokeColor
            strokeWidth: toolState.strokeWidth
        }

        PagePanel {
            Layout.fillHeight: true
            Layout.preferredWidth: 220
            actions: appActions
            visible: root.pagePanelShown
        }
    }

    MessageBar {
        id: messageBar

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 16
    }

    SettingsDialog {
        id: settingsDialog

        settings: settings
        updates: updates
    }

    AboutDialog {
        id: aboutDialog
    }

    TrashDialog {
        id: trashDialog

        notebook: root.notebook
    }

    FileDialog {
        id: copyDialog

        defaultSuffix: "phvika"
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Notebooks (*.phvika)")]
        title: qsTr("Save a copy")

        onAccepted: root.notebook.saveCopy(copyDialog.selectedFile)
    }

    FileDialog {
        id: exportDialog

        defaultSuffix: "pdf"
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("PDF documents (*.pdf)")]
        title: qsTr("Export as PDF")

        onAccepted: root.notebook.exportToPdf(exportDialog.selectedFile)
    }

    FileDialog {
        id: importDialog

        nameFilters: [qsTr("Documents and pictures (*.pdf *.png *.jpg *.jpeg *.webp)")]
        title: qsTr("Import")

        onAccepted: root.notebook.importDocument(importDialog.selectedFile)
    }

    Connections {
        function onFailed(message) {
            messageBar.show(message);
        }

        target: updates
    }

    Connections {
        function onCopied(path) {
            messageBar.show(qsTr("Copied to %1").arg(path));
        }

        function onErrorMessageChanged() {
            if (root.notebook !== null && root.notebook.errorMessage !== "") {
                messageBar.show(root.notebook.errorMessage);
            }
        }

        function onExported(path) {
            messageBar.show(qsTr("Saved as %1").arg(path));
        }

        target: root.notebook
    }
}
