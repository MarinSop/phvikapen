pragma Singleton

import QtQuick
import PhvikaPen.Ui

QtObject {
    id: root

    readonly property string elements: "elements"
    readonly property string layers: "layers"
    readonly property string maths: "maths"
    readonly property string pageSetup: "pageSetup"
    readonly property string pages: "pages"
    readonly property string sections: "sections"
    readonly property string sound: "sound"
    readonly property string time: "time"

    function titleOf(panelId, spoken) {
        switch (panelId) {
        case root.elements:
            return qsTr("Elements");
        case root.layers:
            return qsTr("Layers");
        case root.maths:
            return qsTr("Maths");
        case root.pageSetup:
            return qsTr("Page Setup");
        case root.sections:
            return qsTr("Sections");
        case root.sound:
            return qsTr("Recordings");
        case root.time:
            return qsTr("Time");
        default:
            return qsTr("Pages");
        }
    }

    function iconOf(panelId) {
        switch (panelId) {
        case root.layers:
            return Icons.layer;
        case root.elements:
            return Icons.elements;
        case root.maths:
            return Icons.solve;
        case root.sound:
            return Icons.microphone;
        case root.time:
            return Icons.time;
        case root.pageSetup:
            return Icons.settings;
        default:
            return Icons.grid;
        }
    }
}
