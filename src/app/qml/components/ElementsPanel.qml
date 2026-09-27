pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property AppActions actions
    readonly property ElementsViewModel library: root.actions.library
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property bool ready: root.notebook !== null && root.notebook.loaded
    // The element being carried out of the library, and where the hand has it, so that something
    // follows the pointer instead of the element appearing out of nowhere where it lands.
    property string carrying: ""
    property url carriedPicture: ""
    property point carriedAt: Qt.point(0, 0)

    function askToDelete(elementId, name) {
        deleteDialog.elementId = elementId;
        deleteDialog.itemName = name;
        deleteDialog.open();
    }

    background: null
    objectName: "elementsPanel"
    padding: Theme.gap

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.gap

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap
            visible: root.library.trouble !== ""

            Label {
                Layout.fillWidth: true
                color: Theme.warning
                objectName: "elementsTrouble"
                text: root.library.trouble
                wrapMode: Text.WordWrap
            }

            ShapeButton {
                icon.source: Icons.close
                label: qsTr("Dismiss")

                onClicked: root.library.forgetTrouble()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap

            TextField {
                id: looking

                Accessible.name: qsTr("Search elements")
                Layout.fillWidth: true
                objectName: "elementSearch"
                placeholderText: qsTr("Search")

                onTextEdited: root.library.looking = looking.text
            }

            ComboBox {
                id: kinds

                Accessible.name: qsTr("Kind of element")
                Layout.preferredWidth: Math.round(120 * Theme.scale)
                model: root.library.kinds
                objectName: "elementKinds"

                onActivated: root.library.kind = kinds.currentText
            }
        }

        GridView {
            id: grid

            Accessible.name: qsTr("Elements")
            Layout.fillHeight: true
            Layout.fillWidth: true
            cellHeight: Math.round(104 * Theme.scale)
            cellWidth: Math.round(92 * Theme.scale)
            clip: true
            model: root.library.elements
            objectName: "elementGrid"

            ScrollBar.vertical: ScrollBar {
            }
            delegate: ItemDelegate {
                id: one

                required property string elementId
                required property int index
                required property string kind
                required property string name
                required property string picture

                Accessible.name: one.name
                height: grid.cellHeight - 4
                objectName: "element" + one.index
                width: grid.cellWidth - 4

                background: Rectangle {
                    border.color: one.hovered ? Theme.accent : Theme.line
                    border.width: 1
                    color: one.hovered ? Theme.hover : "transparent"
                    radius: 6
                }
                contentItem: ColumnLayout {
                    spacing: 2

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredHeight: Math.round(56 * Theme.scale)
                        Layout.preferredWidth: Math.round(70 * Theme.scale)
                        color: "white"
                        radius: 3

                        Image {
                            anchors.fill: parent
                            anchors.margins: 2
                            cache: false
                            fillMode: Image.PreserveAspectFit
                            objectName: "elementPicture" + one.index
                            source: one.picture
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font.pixelSize: Math.round(11 * Theme.scale)
                        horizontalAlignment: Text.AlignHCenter
                        objectName: "elementName" + one.index
                        text: one.name
                    }
                }

                onClicked: root.library.put(root.notebook, one.elementId)
                onPressAndHold: oneMenu.popup()

                // An element dragged out of the library lands where the hand lets go, so it can be
                // put exactly where it is wanted rather than in the middle of the view.
                DragHandler {
                    id: carry

                    dragThreshold: 6
                    grabPermissions: PointerHandler.CanTakeOverFromAnything
                    target: null

                    onActiveChanged: {
                        if (carry.active) {
                            root.carrying = one.elementId;
                            root.carriedPicture = one.picture;
                            root.carriedAt = carry.centroid.scenePosition;
                            return;
                        }
                        root.carrying = "";
                        if (root.actions.canvas === null || !root.ready) {
                            return;
                        }
                        const onto = root.actions.canvas.mapFromItem(null, carry.centroid.scenePosition);
                        if (onto.x < 0 || onto.y < 0 || onto.x > root.actions.canvas.width || onto.y > root.actions.canvas.height) {
                            return;
                        }
                        root.library.putAt(root.notebook, one.elementId, onto.x, onto.y);
                    }
                    onCentroidChanged: {
                        if (carry.active) {
                            root.carriedAt = carry.centroid.scenePosition;
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton

                    onTapped: oneMenu.popup()
                }

                Menu {
                    id: oneMenu

                    MenuItem {
                        objectName: "putElementItem"
                        text: qsTr("Put on the page")

                        onTriggered: root.library.put(root.notebook, one.elementId)
                    }

                    MenuItem {
                        text: qsTr("Rename")

                        onTriggered: {
                            renameDialog.elementId = one.elementId;
                            renameDialog.itemName = one.name;
                            renameDialog.open();
                        }
                    }

                    MenuItem {
                        objectName: "deleteElementItem"
                        text: qsTr("Delete…")

                        onTriggered: root.askToDelete(one.elementId, one.name)
                    }
                }
            }
        }

        Button {
            Layout.fillWidth: true
            enabled: root.ready && root.library.anythingToKeep(root.notebook)
            objectName: "keepElementButton"
            text: qsTr("Keep what is picked up")

            onClicked: {
                keepDialog.itemName = "";
                keepDialog.open();
            }
        }
    }

    // What is being carried, drawn over everything so it can be seen on its way to the page.
    Image {
        id: ghost

        readonly property point here: ghost.parent === null ? Qt.point(0, 0) : ghost.parent.mapFromItem(null, root.carriedAt)

        cache: false
        fillMode: Image.PreserveAspectFit
        height: Math.round(56 * Theme.scale)
        opacity: 0.75
        parent: Overlay.overlay
        source: root.carriedPicture
        visible: root.carrying !== "" && ghost.source !== ""
        width: Math.round(70 * Theme.scale)
        x: ghost.here.x - (ghost.width / 2)
        y: ghost.here.y - (ghost.height / 2)
        z: 100
    }

    EmptyPanelNote {
        anchors.centerIn: parent
        text: root.library.elements.count === 0 ? qsTr("Pick something up on a page and keep it here to use it again.") : ""
        visible: root.library.elements.count === 0
        width: parent.width - (Theme.gap * 4)
    }

    AppDialog {
        id: keepDialog

        property string itemName: ""

        objectName: "keepElementDialog"
        standardButtons: Dialog.Ok | Dialog.Cancel
        title: qsTr("Keep as an element")

        onAccepted: root.library.keep(root.notebook, nameField.text, kindField.text)
        onAboutToShow: {
            nameField.text = "";
            kindField.text = root.library.kind === "" || root.library.kind === root.library.kinds[0] ? "" : root.library.kind;
        }

        ColumnLayout {
            spacing: Theme.gap
            width: Math.round(320 * Theme.scale)

            TextField {
                id: nameField

                Accessible.name: qsTr("Name")
                Layout.fillWidth: true
                objectName: "elementNameField"
                placeholderText: qsTr("Name")
            }

            TextField {
                id: kindField

                Accessible.name: qsTr("Kind")
                Layout.fillWidth: true
                objectName: "elementKindField"
                placeholderText: qsTr("Kind, such as Stickers or Shapes")
            }

            Label {
                Layout.fillWidth: true
                color: palette.placeholderText
                font.pixelSize: Math.round(11 * Theme.scale)
                text: qsTr("Ink, words, tables and pictures are all kept.")
                wrapMode: Text.WordWrap
            }
        }
    }

    AppDialog {
        id: renameDialog

        property string elementId: ""
        property string itemName: ""

        objectName: "renameElementDialog"
        standardButtons: Dialog.Ok | Dialog.Cancel
        title: qsTr("Rename element")

        onAccepted: root.library.rename(renameDialog.elementId, renameField.text)
        onAboutToShow: renameField.text = renameDialog.itemName

        TextField {
            id: renameField

            Accessible.name: qsTr("Name")
            objectName: "renameElementField"
            width: Math.round(280 * Theme.scale)
        }
    }

    ConfirmDialog {
        id: deleteDialog

        property string elementId: ""
        property string itemName: ""

        objectName: "deleteElementDialog"
        question: qsTr("“%1” will go from the elements. This cannot be undone.").arg(deleteDialog.itemName)
        title: qsTr("Delete element")

        onAccepted: root.library.remove(deleteDialog.elementId)
    }
}
