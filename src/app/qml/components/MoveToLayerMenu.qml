pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

// Which layer what is picked up should stand on. The layers are not known until the notebook is
// open, so the lines are made as the list says and taken away again with it.
Menu {
    id: root

    required property AppActions actions
    readonly property NotebookViewModel notebook: root.actions.notebook

    function openAt(at) {
        const room = root.parent;
        const x = room === null ? at.x : Math.min(at.x, room.width - root.width);
        const y = room === null ? at.y : Math.min(at.y, room.height - root.height);
        root.popup(Math.max(0, x), Math.max(0, y));
    }

    objectName: "moveToLayerMenu"
    title: qsTr("Move to Layer")

    MenuItem {
        objectName: "moveToNewLayerItem"
        text: qsTr("New Layer")

        onTriggered: {
            root.notebook.addLayer();
            root.notebook.movePickedToLayer(root.notebook.activeLayer);
        }
    }

    MenuSeparator {
    }

    Instantiator {
        model: root.notebook === null ? null : root.notebook.layers

        delegate: MenuItem {
            required property string layerId
            required property string name

            objectName: "moveToLayerItem"
            text: name

            onTriggered: root.notebook.movePickedToLayer(layerId)
        }

        onObjectAdded: (at, line) => root.insertItem(at + 2, line)
        onObjectRemoved: (at, line) => root.removeItem(line)
    }
}
