pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

// What was read out of a picture. The picture itself is never touched: what comes back is words to
// be taken somewhere else.
AppDialog {
    id: root

    required property AppActions actions
    readonly property NotebookViewModel notebook: root.actions.notebook
    property string pictureId: ""
    property string words: ""
    property string trouble: ""
    property bool reading: false

    function readFrom(pictureId) {
        root.pictureId = pictureId;
        root.words = "";
        root.trouble = "";
        root.reading = false;
        root.open();
        root.ask();
    }

    function ask() {
        if (root.notebook === null || root.pictureId === "") {
            return;
        }
        root.trouble = "";
        root.reading = true;
        root.notebook.readPicture(root.pictureId, "");
    }

    objectName: "pictureTextDialog"
    standardButtons: Dialog.Close
    title: qsTr("Text in this picture")

    Connections {
        function onPictureRead(pictureId, words) {
            if (pictureId !== root.pictureId) {
                return;
            }
            root.reading = false;
            root.words = words;
            root.trouble = "";
        }

        function onPictureUnread(pictureId, why) {
            if (pictureId !== root.pictureId) {
                return;
            }
            root.reading = false;
            root.words = "";
            root.trouble = why;
        }

        target: root.notebook
    }

    ColumnLayout {
        spacing: Theme.gap
        width: Math.round(420 * Theme.scale)

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap

            BusyIndicator {
                Layout.preferredHeight: Theme.smallTap
                Layout.preferredWidth: Theme.smallTap
                objectName: "pictureTextWorking"
                running: root.reading
                visible: root.reading
            }

            Label {
                Layout.fillWidth: true
                color: root.trouble === "" ? palette.placeholderText : Theme.warning
                objectName: "pictureTextState"
                text: root.reading ? qsTr("Reading the picture…") : root.trouble !== "" ? root.trouble : qsTr("Read")
                wrapMode: Text.WordWrap
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.round(200 * Theme.scale)
            clip: true
            visible: root.words !== ""

            TextArea {
                id: found

                objectName: "pictureTextFound"
                readOnly: true
                selectByMouse: true
                text: root.words
                wrapMode: TextEdit.Wrap
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap

            Button {
                enabled: root.words !== ""
                objectName: "copyPictureTextButton"
                text: qsTr("Copy")

                onClicked: root.actions.copyToClipboard(root.words)
            }

            Button {
                enabled: root.words !== "" && root.notebook !== null && root.notebook.loaded
                objectName: "insertPictureTextButton"
                text: qsTr("Put on the page")

                onClicked: {
                    root.notebook.writeDown(root.words, root.actions.tools.textStyle, false);
                    root.close();
                }
            }

            Button {
                enabled: !root.reading
                objectName: "retryPictureTextButton"
                text: qsTr("Try again")

                onClicked: {
                    root.notebook.forgetWordsInPicture(root.pictureId);
                    root.ask();
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }

        Label {
            Layout.fillWidth: true
            color: palette.placeholderText
            font.pixelSize: Math.round(11 * Theme.scale)
            text: qsTr("The picture itself is not changed.")
            wrapMode: Text.WordWrap
        }
    }
}
