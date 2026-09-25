pragma Singleton

import QtQuick
import PhvikaPen.Ui

QtObject {
    id: root

    readonly property string layers: "layers"
    readonly property string pageSetup: "pageSetup"
    readonly property string pages: "pages"
    readonly property string sections: "sections"

    function titleOf(panelId, spoken) {
        switch (panelId) {
        case root.layers:
            return qsTr("Layers");
        case root.pageSetup:
            return qsTr("Page Setup");
        case root.sections:
            return qsTr("Sections");
        default:
            return qsTr("Pages");
        }
    }

    function iconOf(panelId) {
        switch (panelId) {
        case root.layers:
            return Icons.layer;
        case root.pageSetup:
            return Icons.settings;
        default:
            return Icons.grid;
        }
    }
}
