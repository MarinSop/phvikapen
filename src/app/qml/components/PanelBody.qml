pragma ComponentBehavior: Bound

import QtQuick
import PhvikaPen.Ui

Loader {
    id: root

    required property AppActions actions
    required property string panelId

    asynchronous: false
    sourceComponent: root.panelId === Panels.layers ? layersBody : root.panelId === Panels.maths ? mathsBody : root.panelId === Panels.time ? timeBody : root.panelId === Panels.sound ? soundBody : root.panelId === Panels.elements ? elementsBody : root.panelId === Panels.pageSetup ? pageSetupBody : root.panelId === Panels.sections ? sectionsBody : pagesBody

    Component {
        id: pagesBody

        PagesPanel {
            actions: root.actions
        }
    }

    Component {
        id: sectionsBody

        SectionsPanel {
            actions: root.actions
        }
    }

    Component {
        id: elementsBody

        ElementsPanel {
            actions: root.actions
        }
    }

    Component {
        id: soundBody

        RecordingPanel {
            actions: root.actions
        }
    }

    Component {
        id: timeBody

        TimeKeeperPanel {
            actions: root.actions
        }
    }

    Component {
        id: mathsBody

        MathPanel {
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
