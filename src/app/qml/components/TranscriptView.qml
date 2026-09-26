pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Item {
    id: root

    required property AppActions actions
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property RecordingViewModel sound: root.actions.sound
    readonly property bool ready: root.notebook !== null && root.notebook.loaded
    readonly property string shown: root.ready ? root.notebook.shownRecording : ""
    readonly property int reading: root.ready ? root.notebook.shownReading : SaidState.Unasked
    readonly property string trouble: root.ready ? root.notebook.shownTrouble : ""

    function saidState() {
        switch (root.reading) {
        case SaidState.Asked:
            return qsTr("Reading what was said…");
        case SaidState.Read:
            return qsTr("Read");
        case SaidState.Failed:
            return root.trouble === "" ? qsTr("It could not be read") : root.trouble;
        default:
            return qsTr("Not read yet");
        }
    }

    implicitHeight: Math.round(160 * Theme.scale)
    visible: root.shown !== ""

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.gap

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap

            Label {
                Layout.fillWidth: true
                color: root.reading === SaidState.Failed ? Theme.warning : palette.placeholderText
                elide: Text.ElideRight
                objectName: "transcriptState"
                text: root.saidState()
            }

            BusyIndicator {
                Layout.preferredHeight: Theme.smallTap
                Layout.preferredWidth: Theme.smallTap
                objectName: "transcriptWorking"
                running: root.reading === SaidState.Asked
                visible: running
            }

            Button {
                enabled: root.shown !== "" && root.sound.readingNow === ""
                objectName: "transcribeButton"
                text: root.reading === SaidState.Failed ? qsTr("Try again") : qsTr("Read it back")

                onClicked: root.sound.readWhatWasSaid(root.shown, root.sound.languages.length > 0 ? root.sound.languages[0] : "")
            }
        }

        ListView {
            id: lines

            Accessible.name: qsTr("What was said")
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            model: root.ready ? root.notebook.sayings : null
            objectName: "transcriptList"
            spacing: 2

            ScrollBar.vertical: ScrollBar {
            }
            delegate: ItemDelegate {
                id: line

                required property real from
                required property int index
                required property string said
                required property real to
                readonly property bool beingHeard: root.sound.playingId === root.shown && root.sound.playedTo >= line.from && root.sound.playedTo < line.to

                Accessible.name: qsTr("At %1, %2").arg(root.sound.saidTime(line.from)).arg(line.said)
                height: Math.max(Theme.rowHeight, words.implicitHeight + Theme.gap)
                objectName: "saying" + line.index
                width: lines.width

                background: Rectangle {
                    border.color: line.beingHeard ? Theme.accent : "transparent"
                    border.width: 1
                    color: line.beingHeard ? Theme.accentSoft : line.hovered ? Theme.hover : "transparent"
                    radius: 6
                }
                contentItem: RowLayout {
                    spacing: Theme.gap

                    Label {
                        Layout.alignment: Qt.AlignTop
                        color: palette.placeholderText
                        font.family: "monospace"
                        font.pixelSize: Math.round(11 * Theme.scale)
                        text: root.sound.saidTime(line.from)
                    }

                    Label {
                        id: words

                        Layout.fillWidth: true
                        text: line.said
                        wrapMode: Text.WordWrap
                    }
                }

                onClicked: {
                    root.sound.goTo(line.from);
                    if (root.sound.playingId !== root.shown) {
                        root.sound.play(root.shown);
                    } else {
                        root.sound.carryOnPlaying();
                    }
                }
            }
        }
    }

    EmptyPanelNote {
        anchors.centerIn: parent
        text: root.sound.canRead ? qsTr("Nothing has been read back yet.") : qsTr("This machine cannot read a recording back as words.")
        visible: lines.count === 0
        width: parent.width - (Theme.gap * 4)
    }
}
