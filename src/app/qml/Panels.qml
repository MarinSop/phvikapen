pragma Singleton

import QtQuick
import PhvikaPen.Ui

QtObject {
    id: root

    readonly property string contents: "contents"
    readonly property string layers: "layers"
    readonly property string pageSetup: "pageSetup"

    function titleOf(panelId, spoken) {
        switch (panelId) {
        case root.layers:
            return qsTr("Layers");
        case root.pageSetup:
            return qsTr("Page Setup");
        default:
            return qsTr("Contents");
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
