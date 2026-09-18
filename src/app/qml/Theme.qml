pragma Singleton

import QtQuick

QtObject {
    id: root

    readonly property color line: Qt.rgba(0.5, 0.5, 0.5, 0.4)
    readonly property real strongStep: 0.12
    readonly property real weakStep: 0.05

    // Light themes are shaded downwards and dark ones upwards, so a bar stands apart from the
    // window whichever way round the colours are.
    function shaded(base, amount) {
        return base.hslLightness > 0.5 ? Qt.darker(base, 1 + amount) : Qt.lighter(base, 1 + amount);
    }
}
