pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import PhvikaPen.Ui

// What the application has to say about what just happened, at the foot of the window and out of
// the way of the paper. One note to a line, the newest underneath, each gone again on its own.
Item {
    id: root

    readonly property int kindBad: 2
    readonly property int kindBusy: 3
    readonly property int kindGood: 1
    readonly property int kindPlain: 0
    readonly property int longest: 90
    readonly property int mostAtOnce: 4
    readonly property int noteCount: notes.count
    readonly property int staysFor: 6000

    // A note of something going on stays until it is settled, so there is never a spinner left
    // turning over work that is already done.
    function busy(text) {
        root.settled();
        root.add(text, root.kindBusy);
    }

    function settled() {
        for (let at = notes.count - 1; at >= 0; --at) {
            if (notes.get(at).kind === root.kindBusy) {
                notes.remove(at);
            }
        }
    }

    function say(text) {
        root.add(text, root.kindPlain);
    }

    function wellDone(text) {
        root.add(text, root.kindGood);
    }

    function wentWrong(text) {
        root.add(text, root.kindBad);
    }

    // Said with what it was about where that is short enough to read at a glance, and cut short
    // with an ellipsis where it is not.
    function about(text, said) {
        const trimmed = said.replace(/\s+/g, " ").trim();
        if (trimmed === "") {
            return text;
        }
        const shown = trimmed.length > root.longest ? trimmed.slice(0, root.longest) + "…" : trimmed;
        return text + ": " + shown;
    }

    function add(text, kind) {
        if (text === "") {
            return;
        }
        notes.append({
            "text": text,
            "kind": kind,
            "until": kind === root.kindBusy ? 0 : Date.now() + root.staysFor
        });
        while (notes.count > root.mostAtOnce) {
            notes.remove(0);
        }
    }

    implicitHeight: lines.implicitHeight
    implicitWidth: lines.implicitWidth
    visible: notes.count > 0

    ListModel {
        id: notes
    }

    Column {
        id: lines

        spacing: 6

        Repeater {
            model: notes

            Pane {
                id: note

                required property int kind
                required property string text

                padding: 12

                background: Rectangle {
                    border.color: note.kind === root.kindGood ? Theme.good : note.kind === root.kindBad ? Theme.warning : Theme.line
                    border.width: 1
                    color: Theme.surface
                    radius: 8
                }
                contentItem: Row {
                    spacing: 8

                    BusyIndicator {
                        anchors.verticalCenter: parent.verticalCenter
                        implicitHeight: Theme.glyph
                        implicitWidth: Theme.glyph
                        running: note.kind === root.kindBusy
                        visible: note.kind === root.kindBusy
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        color: note.kind === root.kindBad ? Theme.warning : note.kind === root.kindGood ? Theme.good : Theme.text
                        objectName: "messageLabel"
                        text: note.text
                    }
                }
            }
        }
    }

    Timer {
        id: sweep

        interval: 250
        repeat: true
        running: notes.count > 0

        onTriggered: {
            const now = Date.now();
            for (let at = notes.count - 1; at >= 0; --at) {
                const until = notes.get(at).until;
                if (until > 0 && until <= now) {
                    notes.remove(at);
                }
            }
        }
    }
}
