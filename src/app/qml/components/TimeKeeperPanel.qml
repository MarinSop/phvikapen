pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property AppActions actions
    readonly property TimeKeeperViewModel keeper: root.actions.timeKeeper
    readonly property bool countingDown: root.keeper.way === TimeKeeperViewModel.Down

    background: null
    objectName: "timeKeeperPanel"
    padding: Theme.gap

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.gap

        TabBar {
            id: ways

            Layout.fillWidth: true
            currentIndex: root.countingDown ? 0 : 1

            onCurrentIndexChanged: root.keeper.way = ways.currentIndex === 0 ? TimeKeeperViewModel.Down : TimeKeeperViewModel.Up

            TabButton {
                objectName: "countdownTab"
                text: qsTr("Timer")
            }

            TabButton {
                objectName: "stopwatchTab"
                text: qsTr("Stopwatch")
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.round(112 * Theme.scale)

            Rectangle {
                anchors.fill: parent
                border.color: root.keeper.rang ? Theme.warning : Theme.line
                border.width: root.keeper.rang ? 2 : 1
                color: root.keeper.rang ? Theme.warningSoft : "transparent"
                radius: 8

                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    running: root.keeper.rang && !Theme.stillness

                    NumberAnimation {
                        duration: Theme.unhurried
                        from: 1
                        to: 0.45
                    }

                    NumberAnimation {
                        duration: Theme.unhurried
                        from: 0.45
                        to: 1
                    }
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 2

                Label {
                    Accessible.name: qsTr("Time %1").arg(root.keeper.said)
                    Layout.alignment: Qt.AlignHCenter
                    color: root.keeper.rang ? Theme.warning : palette.windowText
                    font.family: "monospace"
                    font.pixelSize: Math.round(38 * Theme.scale)
                    objectName: "timeShown"
                    text: root.keeper.said
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    color: Theme.warning
                    objectName: "timeRang"
                    text: qsTr("Time is up")
                    visible: root.keeper.rang
                }
            }

            ProgressBar {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: Theme.gap
                anchors.right: parent.right
                objectName: "timeProgress"
                value: root.keeper.howFar
                visible: root.countingDown
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap
            visible: root.countingDown && !root.keeper.started

            SpinBox {
                id: hours

                Accessible.name: qsTr("Hours")
                Layout.fillWidth: true
                objectName: "timerHours"
                to: 23
                value: root.keeper.wantedHours()

                onValueModified: root.keeper.setWantedParts(hours.value, minutes.value, seconds.value)
            }

            SpinBox {
                id: minutes

                Accessible.name: qsTr("Minutes")
                Layout.fillWidth: true
                objectName: "timerMinutes"
                to: 59
                value: root.keeper.wantedMinutes()

                onValueModified: root.keeper.setWantedParts(hours.value, minutes.value, seconds.value)
            }

            SpinBox {
                id: seconds

                Accessible.name: qsTr("Seconds")
                Layout.fillWidth: true
                objectName: "timerSeconds"
                to: 59
                value: root.keeper.wantedSeconds()

                onValueModified: root.keeper.setWantedParts(hours.value, minutes.value, seconds.value)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap

            Button {
                Layout.fillWidth: true
                highlighted: !root.keeper.running
                objectName: "timeStartButton"
                text: root.keeper.running ? qsTr("Pause") : root.keeper.started ? qsTr("Resume") : qsTr("Start")

                onClicked: root.keeper.startOrPause()
            }

            Button {
                Layout.fillWidth: true
                enabled: root.keeper.started || root.keeper.rang
                objectName: "timeResetButton"
                text: qsTr("Reset")

                onClicked: root.keeper.reset()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap
            visible: root.countingDown && !root.keeper.started

            Repeater {
                model: [5, 10, 15, 25, 45]

                Button {
                    required property int modelData

                    Layout.fillWidth: true
                    objectName: "timerPreset" + modelData
                    text: qsTr("%1m").arg(modelData)

                    onClicked: root.keeper.setWantedParts(0, modelData, 0)
                }
            }
        }

        CheckBox {
            Layout.fillWidth: true
            checked: root.keeper.sounds
            enabled: root.keeper.canSound
            objectName: "timeSoundsCheck"
            text: root.keeper.canSound ? qsTr("Make a noise when it reaches nothing") : qsTr("This machine cannot make a noise")
            visible: root.countingDown

            onToggled: root.keeper.sounds = checked
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
