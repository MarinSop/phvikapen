pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

AppDialog {
    id: root

    property NotebookViewModel notebook: null
    property bool searched: false
    property var results: []

    function search() {
        if (root.notebook === null) {
            return;
        }
        root.searched = true;
        root.notebook.find(field.text);
    }

    height: 420
    objectName: "findDialog"
    standardButtons: Dialog.Close
    title: qsTr("Find in handwriting")
    width: 520

    onOpened: {
        root.searched = false;
        root.results = [];
        field.forceActiveFocus();
        field.selectAll();
    }

    Connections {
        function onFound(words) {
            root.results = words;
        }

        target: root.notebook
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            TextField {
                id: field

                Layout.fillWidth: true
                objectName: "findField"
                placeholderText: qsTr("What was written")

                onAccepted: root.search()
            }

            Button {
                enabled: root.notebook !== null
                objectName: "findButton"
                text: qsTr("Find")

                onClicked: root.search()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap
            visible: root.notebook !== null && root.notebook.canReadPictures()

            Label {
                Layout.fillWidth: true
                color: palette.placeholderText
                objectName: "findPictureState"
                text: root.notebook === null ? "" : root.notebook.picturesToRead > 0 ? qsTr("Reading %1 more picture(s)…", "", root.notebook.picturesToRead) : qsTr("A picture is searched once its words have been read.")
                wrapMode: Text.WordWrap
            }

            Button {
                enabled: root.notebook !== null && root.notebook.loaded
                objectName: "readPicturesButton"
                text: root.notebook !== null && root.notebook.picturesToRead > 0 ? qsTr("Stop") : qsTr("Read every picture")

                onClicked: {
                    if (root.notebook.picturesToRead > 0) {
                        root.notebook.giveUpReadingPictures();
                    } else {
                        root.notebook.readEveryPicture("");
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            color: palette.placeholderText
            text: {
                if (root.notebook === null) {
                    return "";
                }
                if (!root.notebook.readsHandwriting) {
                    return qsTr("This machine cannot read handwriting, so there is nothing to search.");
                }
                if (root.notebook.pagesToRead > 0) {
                    return qsTr("Reading %1 more page(s) of handwriting…", "", root.notebook.pagesToRead);
                }
                if (!root.searched) {
                    return qsTr("Every page of this notebook has been read.");
                }
                return root.results.length === 0 ? qsTr("Nothing like that was written here.") : qsTr("%1 found", "", root.results.length);
            }
            wrapMode: Text.WordWrap
        }

        ListView {
            id: found

            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            model: root.results
            objectName: "findResults"
            spacing: 2

            ScrollBar.vertical: ScrollBar {
            }
            delegate: ItemDelegate {
                id: hit

                required property int index
                required property var modelData

                objectName: "findResult"
                width: found.width

                contentItem: ColumnLayout {
                    spacing: 0

                    Label {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        font.bold: true
                        text: hit.modelData.text
                    }

                    Label {
                        Layout.fillWidth: true
                        color: palette.placeholderText
                        elide: Text.ElideRight
                        text: hit.modelData.sectionTitle === "" ? hit.modelData.pageTitle : hit.modelData.sectionTitle + " — " + hit.modelData.pageTitle
                    }
                }

                onClicked: {
                    root.notebook.goToFound(hit.index);
                    root.close();
                }
            }
        }
    }
}
