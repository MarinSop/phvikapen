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
    property var shut: ({})

    // What was found, gathered under the section and the page it was found on, so where a word was
    // written is read off the list rather than worked out line by line. Every find keeps the place
    // it has in what came back, which is what the notebook is asked to go to.
    readonly property var rows: {
        const gathered = [];
        const places = ({});
        for (let at = 0; at < root.results.length; ++at) {
            const hit = root.results[at];
            const section = hit.sectionTitle === undefined ? "" : hit.sectionTitle;
            const page = hit.pageTitle === undefined ? "" : hit.pageTitle;
            const key = section + "\u0000" + page;
            if (places[key] === undefined) {
                places[key] = {
                    "section": section,
                    "page": page,
                    "hits": []
                };
                gathered.push(places[key]);
            }
            places[key].hits.push({
                "text": hit.text,
                "at": at
            });
        }

        const rows = [];
        let standing = null;
        for (const place of gathered) {
            if (place.section !== standing) {
                standing = place.section;
                rows.push({
                    "kind": "section",
                    "said": place.section === "" ? qsTr("This notebook") : place.section,
                    "section": place.section,
                    "at": -1
                });
            }
            if (root.shut[place.section] === true) {
                continue;
            }
            rows.push({
                "kind": "page",
                "said": place.page,
                "section": place.section,
                "at": -1
            });
            for (const hit of place.hits) {
                rows.push({
                    "kind": "hit",
                    "said": hit.text,
                    "section": place.section,
                    "at": hit.at
                });
            }
        }
        return rows;
    }

    function search() {
        if (root.notebook === null) {
            return;
        }
        root.searched = true;
        root.notebook.find(field.text);
    }

    function openOrShut(section) {
        const now = Object.assign({}, root.shut);
        now[section] = !now[section];
        root.shut = now;
    }

    height: 420
    objectName: "findDialog"
    standardButtons: Dialog.Close
    title: qsTr("Find in handwriting")
    width: 520

    onOpened: {
        root.searched = false;
        root.results = [];
        root.shut = ({});
        field.forceActiveFocus();
        field.selectAll();
    }

    Connections {
        function onFound(words) {
            root.results = words;
        }

        target: root.notebook
    }

    Timer {
        id: waiting

        interval: 250

        onTriggered: root.search()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            // Searched as it is typed, a moment after the typing stops rather than at every letter.
            TextField {
                id: field

                Layout.fillWidth: true
                objectName: "findField"
                placeholderText: qsTr("What was written")

                onAccepted: {
                    waiting.stop();
                    root.search();
                }
                onTextChanged: waiting.restart()
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
            model: root.rows
            objectName: "findResults"
            spacing: 0

            ScrollBar.vertical: ScrollBar {
            }
            delegate: ItemDelegate {
                id: line

                required property var modelData

                enabled: line.modelData.kind !== "page"
                objectName: line.modelData.kind === "hit" ? "findResult" : line.modelData.kind === "section" ? "findSection" : "findPage"
                width: found.width

                contentItem: RowLayout {
                    spacing: Theme.gap

                    Label {
                        color: palette.placeholderText
                        text: root.shut[line.modelData.section] === true ? "▸" : "▾"
                        visible: line.modelData.kind === "section"
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: line.modelData.kind === "hit" ? Theme.rowHeight : line.modelData.kind === "page" ? Theme.gap : 0
                        color: line.modelData.kind === "hit" ? palette.windowText : palette.placeholderText
                        elide: Text.ElideRight
                        font.bold: line.modelData.kind === "section"
                        text: line.modelData.said
                    }
                }

                onClicked: {
                    if (line.modelData.kind === "section") {
                        root.openOrShut(line.modelData.section);
                        return;
                    }
                    if (line.modelData.kind !== "hit") {
                        return;
                    }
                    root.notebook.goToFound(line.modelData.at);
                    root.close();
                }
            }
        }
    }
}
