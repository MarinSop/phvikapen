pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

Menu {
    id: root

    required property AppActions actions
    required property int index
    required property string pageTitle
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property bool ready: root.notebook !== null && root.notebook.loaded
    readonly property list<int> backgrounds: [PageOptions.Blank, PageOptions.Lined, PageOptions.Grid, PageOptions.Dotted]
    readonly property list<string> backgroundNames: [qsTr("Blank"), qsTr("Lined"), qsTr("Squares"), qsTr("Dots")]
    readonly property list<int> papers: [PageOptions.Infinite, PageOptions.A3, PageOptions.A4, PageOptions.A5, PageOptions.Letter, PageOptions.Legal, PageOptions.Custom]
    readonly property list<string> paperNames: [qsTr("Infinite"), qsTr("A3"), qsTr("A4"), qsTr("A5"), qsTr("Letter"), qsTr("Legal"), qsTr("Own size")]

    signal deleteWanted
    signal renameWanted

    function openAt(at) {
        const room = root.parent;
        const x = room === null ? at.x : Math.min(at.x, room.width - root.width);
        const y = room === null ? at.y : Math.min(at.y, room.height - root.height);
        root.popup(Math.max(0, x), Math.max(0, y));
    }

    objectName: "pageOptionsMenu"
    width: Math.round(260 * Theme.scale)

    MenuItem {
        objectName: "renamePageItem"
        text: qsTr("Rename")

        onTriggered: root.renameWanted()
    }

    MenuItem {
        objectName: "duplicatePageItem"
        text: qsTr("Duplicate")

        onTriggered: root.notebook.duplicatePage(root.index)
    }

    MenuItem {
        enabled: root.index > 0
        text: qsTr("Move up")

        onTriggered: root.notebook.movePage(root.index, root.index - 1)
    }

    MenuItem {
        enabled: root.ready && root.index + 1 < root.notebook.pageCount
        text: qsTr("Move down")

        onTriggered: root.notebook.movePage(root.index, root.index + 1)
    }

    Menu {
        id: intoSection

        // The last page of a section stays, as it does when one is deleted, so a section of one page
        // has nothing to give away.
        enabled: root.ready && root.notebook.sectionCount > 1 && root.notebook.pageCount > 1
        objectName: "movePageToSectionMenu"
        title: qsTr("Move to another section")

        Instantiator {
            model: root.ready ? root.notebook.sections : null

            delegate: MenuItem {
                required property int index
                required property string title

                enabled: index !== (root.ready ? root.notebook.currentSection : -1)
                objectName: "movePageToSection" + index
                text: title

                onTriggered: root.notebook.movePageToSection(root.index, index)
            }

            onObjectAdded: (index, object) => intoSection.insertItem(index, object)
            onObjectRemoved: (index, object) => intoSection.removeItem(object)
        }
    }

    MenuLine {
    }

    Menu {
        objectName: "paperMenu"
        title: qsTr("Paper of this section")

        Repeater {
            model: root.paperNames

            MenuItem {
                required property int index
                required property string modelData

                checkable: true
                checked: root.ready && root.notebook.paper === root.papers[index]
                text: modelData

                onTriggered: root.notebook.paper = root.papers[index]
            }
        }
    }

    Menu {
        objectName: "backgroundMenu"
        title: qsTr("Background")

        Repeater {
            model: root.backgroundNames

            MenuItem {
                required property int index
                required property string modelData

                checkable: true
                checked: root.ready && root.notebook.background === root.backgrounds[index]
                text: modelData

                onTriggered: root.notebook.background = root.backgrounds[index]
            }
        }
    }

    MenuItem {
        checkable: true
        checked: root.ready && root.notebook.orientation === PageOptions.Landscape
        enabled: root.ready && root.notebook.paper !== PageOptions.Infinite && root.notebook.paper !== PageOptions.Custom
        objectName: "landscapeItem"
        text: qsTr("Landscape")

        onTriggered: root.notebook.orientation = checked ? PageOptions.Landscape : PageOptions.Portrait
    }

    MenuItem {
        objectName: "pageSetupItem"
        text: qsTr("More page setup…")

        onTriggered: root.actions.workspace.showPanel(Panels.pageSetup)
    }

    MenuLine {
    }

    MenuItem {
        enabled: root.ready && root.notebook.pageCount > 1
        objectName: "deletePageItem"
        text: qsTr("Delete…")

        onTriggered: root.deleteWanted()
    }
}
