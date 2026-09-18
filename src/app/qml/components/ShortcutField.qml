import QtQuick
import QtQuick.Controls

TextField {
    id: root

    property string sequence: ""

    signal captured(string sequence)

    function textOf(modifiers, key) {
        const parts = [];
        if (modifiers & Qt.ControlModifier) {
            parts.push("Ctrl");
        }
        if (modifiers & Qt.AltModifier) {
            parts.push("Alt");
        }
        if (modifiers & Qt.ShiftModifier) {
            parts.push("Shift");
        }
        if (modifiers & Qt.MetaModifier) {
            parts.push("Meta");
        }
        parts.push(AppInfo.keyName(key));
        return parts.join("+");
    }

    horizontalAlignment: Text.AlignHCenter
    implicitWidth: 150
    placeholderText: qsTr("Press keys")
    readOnly: true
    text: root.activeFocus ? qsTr("Press keys…") : root.sequence

    Keys.onPressed: event => {
        const key = event.key;
        const ignored = [Qt.Key_Control, Qt.Key_Shift, Qt.Key_Alt, Qt.Key_Meta, Qt.Key_Tab, Qt.Key_unknown];
        if (ignored.indexOf(key) >= 0) {
            return;
        }
        event.accepted = true;
        if (key === Qt.Key_Escape) {
            root.focus = false;
            return;
        }
        root.captured(root.textOf(event.modifiers, key));
        root.focus = false;
    }
}
