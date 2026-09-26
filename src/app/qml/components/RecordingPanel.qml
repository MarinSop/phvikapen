pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property AppActions actions
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property RecordingViewModel sound: root.actions.sound
    readonly property bool ready: root.notebook !== null && root.notebook.loaded
    readonly property string shown: root.ready ? root.notebook.shownRecording : ""

    function askToDelete(recordingId, name) {
        deleteDialog.recordingId = recordingId;
        deleteDialog.itemName = name;
        deleteDialog.open();
    }

    background: null
    objectName: "recordingPanel"
    padding: Theme.gap

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.gap

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap
            visible: root.sound.trouble !== ""

            Label {
                Layout.fillWidth: true
                color: Theme.warning
                objectName: "recordingTrouble"
                text: root.sound.trouble
                wrapMode: Text.WordWrap
            }

            ShapeButton {
                icon.source: Icons.close
                label: qsTr("Dismiss")
                objectName: "dismissTroubleButton"

                onClicked: root.sound.forgetTrouble()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap

            Rectangle {
                Layout.preferredHeight: Theme.smallTap
                Layout.preferredWidth: Theme.smallTap
                color: root.sound.recording && !root.sound.paused ? Theme.warning : Theme.line
                radius: width / 2

                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    running: root.sound.recording && !root.sound.paused && !Theme.stillness

                    NumberAnimation {
                        duration: Theme.unhurried
                        from: 1
                        to: 0.3
                    }

                    NumberAnimation {
                        duration: Theme.unhurried
                        from: 0.3
                        to: 1
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                font.family: "monospace"
                font.pixelSize: Math.round(20 * Theme.scale)
                objectName: "recordingClock"
                text: root.sound.saidTime(root.sound.recordedFor)
            }

            Button {
                enabled: root.ready
                highlighted: !root.sound.recording
                objectName: "recordButton"
                text: !root.sound.recording ? qsTr("Record") : root.sound.paused ? qsTr("Resume") : qsTr("Pause")

                onClicked: root.sound.recordOrPause()
            }

            Button {
                enabled: root.sound.recording
                objectName: "stopRecordingButton"
                text: qsTr("Stop")

                onClicked: root.sound.stopRecording()
            }
        }

        ListView {
            id: list

            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, Math.round(180 * Theme.scale))
            clip: true
            model: root.ready ? root.notebook.recordings : null
            objectName: "recordingList"
            spacing: 2

            ScrollBar.vertical: ScrollBar {
            }
            delegate: ItemDelegate {
                id: line

                required property int index
                required property real length
                required property int marks
                required property string name
                required property string recordingId
                readonly property bool chosen: line.recordingId === root.shown
                readonly property bool sounding: root.sound.playingId === line.recordingId
                property bool renaming: false

                Accessible.name: line.name
                height: Math.round(46 * Theme.scale)
                objectName: "recordingLine" + line.index
                width: list.width

                background: Rectangle {
                    color: line.chosen ? Theme.accentSoft : line.hovered ? Theme.hover : "transparent"
                    radius: 6
                }
                contentItem: RowLayout {
                    spacing: Theme.gap

                    ShapeButton {
                        active: line.sounding && root.sound.playing
                        enabled: line.length > 0
                        icon.source: line.sounding && root.sound.playing ? Icons.pause : Icons.play
                        label: line.sounding && root.sound.playing ? qsTr("Pause") : qsTr("Play")
                        objectName: "playRecording" + line.index

                        onClicked: root.sound.playOrPause(line.recordingId)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Label {
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            objectName: "recordingName" + line.index
                            text: line.name
                            visible: !line.renaming
                        }

                        TextField {
                            id: rename

                            Layout.fillWidth: true
                            objectName: "recordingRename" + line.index
                            text: line.name
                            visible: line.renaming

                            Keys.onEscapePressed: line.renaming = false
                            onAccepted: {
                                root.notebook.renameRecording(line.recordingId, rename.text);
                                line.renaming = false;
                            }
                            onActiveFocusChanged: {
                                if (!rename.activeFocus) {
                                    line.renaming = false;
                                }
                            }
                            onVisibleChanged: {
                                if (rename.visible) {
                                    rename.forceActiveFocus();
                                    rename.selectAll();
                                }
                            }
                        }

                        Label {
                            color: palette.placeholderText
                            font.pixelSize: Math.round(11 * Theme.scale)
                            text: line.length > 0 ? qsTr("%1 · %n note(s)", "", line.marks).arg(root.sound.saidTime(line.length)) : qsTr("Recording…")
                            visible: !line.renaming
                        }
                    }

                    ShapeButton {
                        icon.source: Icons.close
                        label: qsTr("Delete recording")
                        objectName: "deleteRecording" + line.index
                        opacity: line.hovered || line.chosen ? 1 : 0

                        onClicked: root.askToDelete(line.recordingId, line.name)
                    }
                }

                onDoubleClicked: line.renaming = true
                onPressed: root.notebook.showRecording(line.recordingId)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap
            visible: root.sound.playingId !== ""

            Label {
                font.family: "monospace"
                font.pixelSize: Math.round(11 * Theme.scale)
                text: root.sound.saidTime(root.sound.playedTo)
            }

            Slider {
                id: along

                Accessible.name: qsTr("Position in the recording")
                Layout.fillWidth: true
                from: 0
                objectName: "playingSlider"
                to: Math.max(1, root.sound.playingLength)
                value: root.sound.playedTo

                onMoved: root.sound.goTo(along.value)
            }

            Label {
                font.family: "monospace"
                font.pixelSize: Math.round(11 * Theme.scale)
                text: root.sound.saidTime(root.sound.playingLength)
            }

            ShapeButton {
                icon.source: Icons.close
                label: qsTr("Stop playing")
                objectName: "stopPlayingButton"

                onClicked: root.sound.stopPlaying()
            }
        }

        TranscriptView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            actions: root.actions
            objectName: "transcriptView"
        }
    }

    EmptyPanelNote {
        anchors.centerIn: parent
        text: qsTr("Open a page to record on it.")
        visible: !root.ready
        width: parent.width - (Theme.gap * 4)
    }

    ConfirmDialog {
        id: deleteDialog

        property string itemName: ""
        property string recordingId: ""

        objectName: "deleteRecordingDialog"
        question: qsTr("“%1” and everything tied to it will go. This cannot be undone.").arg(deleteDialog.itemName)
        title: qsTr("Delete recording")

        onAccepted: {
            if (root.sound.playingId === deleteDialog.recordingId) {
                root.sound.stopPlaying();
            }
            root.notebook.removeRecording(deleteDialog.recordingId);
        }
    }
}
