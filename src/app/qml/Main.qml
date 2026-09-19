pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import PhvikaPen.Ui

ApplicationWindow {
    id: root

    property int exportScope: settings.exportScope
    property bool leaving: false
    readonly property NotebookViewModel notebook: notebooks.current

    // Everything with changes is written where it belongs, or the one that has nowhere to go
    // asks for a place first.
    function saveEverything() {
        for (let index = 0; index < notebooks.openNotebooks.length; ++index) {
            if (!notebooks.isEdited(index)) {
                continue;
            }
            const kept = notebooks.notebookAt(index);
            if (kept === null) {
                continue;
            }
            if (!kept.save()) {
                notebooks.currentIndex = index;
                appActions.saveWanted();
                return false;
            }
        }
        return true;
    }

    function somethingIsUnsaved() {
        for (let index = 0; index < notebooks.openNotebooks.length; ++index) {
            if (notebooks.isEdited(index)) {
                return true;
            }
        }
        return false;
    }

    color: Theme.window
    height: 800
    palette.accent: Theme.accent
    palette.base: Theme.base
    palette.button: Theme.surface
    palette.buttonText: Theme.text
    palette.dark: Theme.surfaceStrong
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentText
    palette.light: Theme.surface
    palette.mid: Theme.line
    palette.midlight: Theme.line
    palette.placeholderText: Theme.subtleText
    palette.shadow: Theme.surfaceStrong
    palette.text: Theme.text
    palette.toolTipBase: Theme.surface
    palette.toolTipText: Theme.text
    palette.window: Theme.window
    palette.windowText: Theme.text
    title: root.notebook === null ? qsTr("PhvikaPen %1").arg(AppInfo.version) : qsTr("%1%2 — PhvikaPen %3").arg(root.notebook.title).arg(root.notebook.edited ? " •" : "").arg(AppInfo.version)
    visible: true
    width: 1280

    footer: AppStatusBar {
        actions: appActions
    }
    header: ColumnLayout {
        spacing: 0

        NotebookTabs {
            Layout.fillWidth: true
            actions: appActions
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
    }

    onClosing: close => {
        if (!root.leaving && root.somethingIsUnsaved()) {
            close.accepted = false;
            leaveDialog.open();
        }
    }
    Component.onCompleted: {
        notebooks.canvas = canvas;
        if (!settings.themeChosen) {
            welcomeDialog.open();
        }
    }

    Binding {
        property: "mode"
        target: Theme
        value: settings.theme
    }

    ToolViewModel {
        id: toolState
    }

    SettingsViewModel {
        id: settings
    }

    NotebooksViewModel {
        id: notebooks

        continuousPages: settings.continuousPages

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
        settings: settings
        tools: toolState

        onAboutWanted: aboutDialog.open()
        onLeaveWanted: root.close()
        onCloseAsked: index => {
            closeDialog.index = index;
            closeDialog.notebookName = notebooks.openNotebooks[index];
            closeDialog.open();
        }
        onSaveWanted: {
            const kept = root.notebook.keptAt;
            const folder = kept === "" ? StandardPaths.writableLocation(StandardPaths.DocumentsLocation) : "file://" + kept.substring(0, kept.lastIndexOf("/"));
            saveDialog.currentFolder = folder;
            saveDialog.selectedFile = kept === "" ? folder + "/" + root.notebook.title + ".phvika" : "file://" + kept;
            saveDialog.open();
        }
        onExportWanted: scope => {
            root.exportScope = scope;
            const folder = StandardPaths.writableLocation(StandardPaths.DocumentsLocation);
            exportDialog.currentFolder = folder;
            exportDialog.selectedFile = folder + "/" + root.notebook.title + ".pdf";
            exportDialog.open();
        }
        onHintsWanted: hintsDialog.open()
        onImportWanted: importDialog.open()
        onNewNotebookWanted: newNotebookDialog.open()
        onPageSetupWanted: pageSetupDialog.open()
        onSettingsWanted: settingsDialog.open()
        onTrashWanted: trashDialog.open()
    }

    SplitView {
        id: mainSplit

        anchors.fill: parent
        orientation: Qt.Horizontal

        handle: Item {
            id: sideHandle

            implicitWidth: 7

            Rectangle {
                anchors.centerIn: parent
                color: sideHandle.SplitHandle.pressed || sideHandle.SplitHandle.hovered ? Theme.accent : Theme.line
                height: parent.height
                width: sideHandle.SplitHandle.pressed || sideHandle.SplitHandle.hovered ? 3 : 1
            }
        }

        ToolPalette {
            SplitView.maximumWidth: implicitWidth
            SplitView.minimumWidth: implicitWidth
            actions: appActions
        }

        PagesPanel {
            id: pagesPanel

            SplitView.maximumWidth: 520
            SplitView.minimumWidth: 140
            actions: appActions
            visible: root.notebook !== null && (settings.showSections || settings.showPages)

            Component.onCompleted: SplitView.preferredWidth = settings.panelWidth
            onWidthChanged: {
                if (mainSplit.resizing) {
                    settings.panelWidth = pagesPanel.width;
                }
            }
        }

        EmptyState {
            SplitView.fillWidth: true
            actions: appActions
            notebooks: notebooks
            visible: root.notebook === null
        }

        InkCanvas {
            id: canvas

            SplitView.fillWidth: true
            enabled: root.notebook !== null && root.notebook.loaded
            deskColor: Theme.desk
            eraserRadius: toolState.eraserRadius
            erasing: toolState.currentTool === ToolViewModel.Eraser
            panning: toolState.currentTool === ToolViewModel.Hand
            picking: toolState.currentTool === ToolViewModel.ColourPicker
            pressureSensitive: toolState.pressureSensitive
            selecting: toolState.currentTool === ToolViewModel.Selection
            corner: toolState.corner
            shape: toolState.currentTool === ToolViewModel.Shape ? toolState.shape : ToolViewModel.Freehand
            smoothing: settings.smoothing
            strokeColor: toolState.strokeColor
            strokeWidth: toolState.strokeWidth
            visible: root.notebook !== null

            // How wide the line, or the eraser, will be right here on the page.
            Rectangle {
                readonly property real sizeOnPage: toolState.currentTool === ToolViewModel.Eraser ? toolState.eraserRadius * 2 : toolState.strokeWidth

                border.color: Theme.text
                border.width: 1
                color: "transparent"
                height: width
                opacity: 0.7
                radius: width / 2
                visible: canvas.pointerInside && canvas.enabled && (toolState.currentTool === ToolViewModel.Pen || toolState.currentTool === ToolViewModel.Highlighter || toolState.currentTool === ToolViewModel.Shape || toolState.currentTool === ToolViewModel.Eraser)
                width: Math.max(8, sizeOnPage * canvas.zoom)
                x: canvas.pointerAt.x - (width / 2)
                y: canvas.pointerAt.y - (height / 2)

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    border.color: Theme.window
                    border.width: 1
                    color: "transparent"
                    radius: width / 2
                }
            }
        }

        PagePanel {
            SplitView.maximumWidth: 420
            SplitView.minimumWidth: 160
            SplitView.preferredWidth: 220
            actions: appActions
            visible: settings.showPagePanel && root.notebook !== null
        }
    }

    MessageBar {
        id: messageBar

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 16
    }

    AppDialog {
        id: leaveDialog

        objectName: "leaveDialog"
        standardButtons: Dialog.Save | Dialog.Discard | Dialog.Cancel
        title: qsTr("Leave PhvikaPen")
        width: 380

        onAccepted: {
            if (root.saveEverything()) {
                root.leaving = true;
                root.close();
            }
        }
        onDiscarded: {
            root.leaving = true;
            leaveDialog.close();
            root.close();
        }

        Label {
            width: parent.width
            text: qsTr("Some notebooks have changes that are not saved yet.")
            wrapMode: Text.WordWrap
        }
    }

    AppDialog {
        id: closeDialog

        property int index: -1
        property string notebookName: ""

        objectName: "closeDialog"
        standardButtons: Dialog.Save | Dialog.Discard | Dialog.Cancel
        title: qsTr("Close notebook")
        width: 380

        onAccepted: {
            if (root.notebook !== null && !root.notebook.save()) {
                appActions.saveWanted();
                return;
            }
            notebooks.closeNotebook(closeDialog.index);
        }
        onDiscarded: {
            notebooks.closeNotebook(closeDialog.index);
            closeDialog.close();
        }

        Label {
            width: parent.width
            text: qsTr("“%1” has changes that are not saved yet.").arg(closeDialog.notebookName)
            wrapMode: Text.WordWrap
        }
    }

    NewNotebookDialog {
        id: newNotebookDialog

        notebooks: notebooks
        settings: settings
    }

    SettingsDialog {
        id: settingsDialog

        settings: settings
        updates: updates
    }

    PageSetupDialog {
        id: pageSetupDialog

        notebook: root.notebook
    }

    HintsDialog {
        id: hintsDialog
    }

    AboutDialog {
        id: aboutDialog
    }

    WelcomeDialog {
        id: welcomeDialog

        settings: settings
    }

    TrashDialog {
        id: trashDialog

        notebook: root.notebook
    }

    FileDialog {
        id: saveDialog

        defaultSuffix: "phvika"
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Notebooks (*.phvika)")]
        objectName: "saveDialog"
        title: qsTr("Save as")

        onAccepted: root.notebook.saveAs(saveDialog.selectedFile)
    }

    FileDialog {
        id: exportDialog

        defaultSuffix: "pdf"
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("PDF documents (*.pdf)")]
        title: qsTr("Export as PDF")

        onAccepted: root.notebook.exportToPdf(exportDialog.selectedFile, root.exportScope)
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
        function onColourPicked(colour) {
            toolState.usePickedColour(colour);
        }

        function onErrorMessageChanged() {
            if (root.notebook !== null && root.notebook.errorMessage !== "") {
                messageBar.show(root.notebook.errorMessage);
            }
        }

        function onExported(path) {
            messageBar.show(qsTr("Saved as %1").arg(path));
        }

        function onSaved(path) {
            messageBar.show(qsTr("Saved to %1").arg(path));
        }

        target: root.notebook
    }
}
