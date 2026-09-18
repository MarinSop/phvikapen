pragma Singleton

import QtQuick

QtObject {
    id: root

    // Brand follows the application's own colours, Dark is the plain grey one, Light is for a
    // lit room. The sheet itself stays white in all three: paper is paper, and it is what a
    // written document holds.
    enum Mode {
        Brand,
        Dark,
        Light
    }

    property int mode: Theme.Brand
    readonly property var brandColours: ({
            "window": "#12102e",
            "surface": "#191640",
            "surfaceStrong": "#0d0b24",
            "base": "#221d55",
            "desk": "#0b0a1e",
            "text": "#f1f0fa",
            "subtleText": "#9e99c8",
            "line": "#2c2763",
            "accent": "#6e45f7"
        })
    readonly property var darkColours: ({
            "window": "#1d1d20",
            "surface": "#252529",
            "surfaceStrong": "#151517",
            "base": "#2c2c31",
            "desk": "#2b2b2f",
            "text": "#ececef",
            "subtleText": "#98979f",
            "line": "#3a3a41",
            "accent": "#3b82f6"
        })
    readonly property var lightColours: ({
            "window": "#f4f3fa",
            "surface": "#ffffff",
            "surfaceStrong": "#eae8f5",
            "base": "#ffffff",
            "desk": "#e6e4f0",
            "text": "#171432",
            "subtleText": "#6a6690",
            "line": "#dedbee",
            "accent": "#5b34e8"
        })
    readonly property var current: root.coloursOf(root.mode)
    readonly property bool dark: root.mode !== Theme.Light
    // The picture the window, the about box and the icon all come from.
    readonly property url logo: "qrc:/brand/logo-256.png"
    readonly property color brandStart: "#8b2cf5"
    readonly property color brandEnd: "#1e7bf0"
    readonly property color window: root.current.window
    readonly property color surface: root.current.surface
    readonly property color surfaceStrong: root.current.surfaceStrong
    readonly property color base: root.current.base
    readonly property color desk: root.current.desk
    readonly property color text: root.current.text
    readonly property color subtleText: root.current.subtleText
    readonly property color line: root.current.line
    readonly property color accent: root.current.accent
    readonly property color accentText: "#ffffff"
    readonly property color accentSoft: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, root.dark ? 0.26 : 0.16)
    readonly property color hover: Qt.rgba(root.text.r, root.text.g, root.text.b, 0.08)

    function coloursOf(mode) {
        switch (mode) {
        case Theme.Dark:
            return root.darkColours;
        case Theme.Light:
            return root.lightColours;
        default:
            return root.brandColours;
        }
    }

    function nameOf(mode) {
        switch (mode) {
        case Theme.Dark:
            return qsTr("Dark");
        case Theme.Light:
            return qsTr("Light");
        default:
            return qsTr("PhvikaPen");
        }
    }

    function noteOf(mode) {
        switch (mode) {
        case Theme.Dark:
            return qsTr("Plain dark grey, out of the way of the page.");
        case Theme.Light:
            return qsTr("For a lit room, and beside a window.");
        default:
            return qsTr("The colours of the application itself: indigo, violet and blue.");
        }
    }
}
