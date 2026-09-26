pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

AppDialog {
    id: root

    required property AppActions actions
    readonly property NotebookViewModel notebook: root.actions.notebook
    // Empty while a link is being made; the link being changed otherwise.
    property string linkId: ""
    property bool toPage: true

    function makeOne() {
        root.linkId = "";
        root.toPage = true;
        webField.text = "";
        labelField.text = "";
        root.open();
    }

    function change(link) {
        root.linkId = link.linkId;
        root.toPage = link.toPage;
        webField.text = link.toPage ? "" : link.where;
        labelField.text = link.label;
        if (link.toPage) {
            root.chooseThePage(link.where);
        }
        root.open();
    }

    function chooseThePage(pageId) {
        for (let step = 0; step < pages.count; ++step) {
            if (pages.model[step].pageId === pageId) {
                pages.currentIndex = step;
                return;
            }
        }
    }

    objectName: "linkDialog"
    standardButtons: Dialog.Ok | Dialog.Cancel
    title: root.linkId === "" ? qsTr("Add Link") : qsTr("Change Link")

    onAccepted: {
        const where = root.toPage ? (pages.currentIndex >= 0 && pages.count > 0 ? pages.model[pages.currentIndex].pageId : "") : webField.text;
        if (root.linkId === "") {
            root.notebook.addLink(where, root.toPage, labelField.text);
        } else {
            root.notebook.changeLink(root.linkId, where, root.toPage, labelField.text);
        }
    }
    onAboutToShow: pages.model = root.notebook === null ? [] : root.notebook.pagesToLinkTo

    ColumnLayout {
        spacing: Theme.gap
        width: Math.round(360 * Theme.scale)

        TabBar {
            id: ways

            Layout.fillWidth: true
            currentIndex: root.toPage ? 0 : 1

            onCurrentIndexChanged: root.toPage = ways.currentIndex === 0

            TabButton {
                objectName: "linkToPageTab"
                text: qsTr("A page")
            }

            TabButton {
                objectName: "linkToWebTab"
                text: qsTr("Somewhere else")
            }
        }

        ComboBox {
            id: pages

            Accessible.name: qsTr("Page to go to")
            Layout.fillWidth: true
            objectName: "linkPageBox"
            textRole: "title"
            visible: root.toPage

            delegate: ItemDelegate {
                required property int index
                required property var modelData

                highlighted: pages.highlightedIndex === index
                text: modelData.section + " · " + modelData.title
                width: pages.width
            }
        }

        TextField {
            id: webField

            Accessible.name: qsTr("Where it goes")
            Layout.fillWidth: true
            objectName: "linkWhereField"
            placeholderText: qsTr("https://")
            visible: !root.toPage
        }

        Label {
            Layout.fillWidth: true
            color: palette.placeholderText
            font.pixelSize: Math.round(11 * Theme.scale)
            text: qsTr("A link goes to a web page or to an address for mail.")
            visible: !root.toPage
            wrapMode: Text.WordWrap
        }

        TextField {
            id: labelField

            Accessible.name: qsTr("What it is called")
            Layout.fillWidth: true
            objectName: "linkLabelField"
            placeholderText: qsTr("What it is called")
        }

        Label {
            Layout.fillWidth: true
            color: palette.placeholderText
            font.pixelSize: Math.round(11 * Theme.scale)
            text: qsTr("The link is put over whatever is picked up, or in the middle of the page where nothing is.")
            wrapMode: Text.WordWrap
        }
    }
}
