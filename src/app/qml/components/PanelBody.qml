pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

Loader {
    id: root

    required property AppActions actions
    required property string panelId

    asynchronous: false
    sourceComponent: root.panelId === Panels.layers ? layersBody : root.panelId === Panels.pageSetup ? pageSetupBody : contentsBody

    Component {
        id: contentsBody

        PagesPanel {
            actions: root.actions
        }
    }

    Component {
        id: layersBody

        LayersPanel {
            actions: root.actions
        }
    }

    Component {
        id: pageSetupBody

        PageSetupPanel {
            actions: root.actions
        }
    }
}
