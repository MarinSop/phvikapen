pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

Pane {
    id: root

    required property AppActions actions
    readonly property MathViewModel maths: root.actions.maths
    readonly property NotebookViewModel notebook: root.actions.notebook
    readonly property bool ready: root.notebook !== null && root.notebook.loaded

    function reasonSaid(reason, number) {
        switch (reason) {
        case MathViewModel.Gathered:
            return qsTr("Everything brought to one side");
        case MathViewModel.Divided:
            return qsTr("Divided both sides by %1").arg(number);
        case MathViewModel.Formula:
            return qsTr("Used the formula for a square");
        case MathViewModel.Reached:
            return qsTr("Answer");
        default:
            return qsTr("Answer");
        }
    }

    background: null
    objectName: "mathPanel"
    padding: Theme.gap

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.gap

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap

            TextField {
                id: asked

                Accessible.name: qsTr("Equation")
                Layout.fillWidth: true
                objectName: "mathField"
                placeholderText: qsTr("2x + 5 = 15")
                text: root.maths.said

                Keys.onReturnPressed: root.maths.ask(asked.text)
                onTextEdited: root.maths.ask(asked.text)
            }
        }

        Label {
            Layout.fillWidth: true
            color: root.maths.state === MathViewModel.Refused ? Theme.warning : palette.windowText
            font.bold: true
            font.pixelSize: Math.round(16 * Theme.scale)
            objectName: "mathAnswer"
            text: {
                switch (root.maths.state) {
                case MathViewModel.Answered:
                case MathViewModel.Solved:
                    return root.maths.answer;
                case MathViewModel.Always:
                    return qsTr("True whatever %1 stands for").arg(root.maths.letter);
                case MathViewModel.Drawn:
                    return qsTr("Two letters, so there is a curve rather than one answer.");
                case MathViewModel.Refused:
                    return root.maths.message;
                default:
                    return qsTr("Type a sum or an equation.");
                }
            }
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap
            visible: root.maths.written() !== ""

            Button {
                objectName: "mathCopyButton"
                text: qsTr("Copy")

                onClicked: root.actions.copyToClipboard(root.maths.written())
            }

            Button {
                enabled: root.ready
                objectName: "mathInsertButton"
                text: qsTr("Put on the page")

                onClicked: root.notebook.writeDown(root.maths.written(), root.actions.tools.textStyle, true)
            }

            Item {
                Layout.fillWidth: true
            }
        }

        ListView {
            id: workingList

            Accessible.name: qsTr("Working")
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, Math.round(140 * Theme.scale))
            clip: true
            model: root.maths.working
            objectName: "mathWorking"
            visible: count > 0

            ScrollBar.vertical: ScrollBar {
            }
            delegate: RowLayout {
                id: line

                required property var modelData

                spacing: Theme.gap
                width: workingList.width

                Label {
                    Layout.preferredWidth: parent.width * 0.5
                    color: palette.placeholderText
                    elide: Text.ElideRight
                    font.pixelSize: Math.round(11 * Theme.scale)
                    text: root.reasonSaid(line.modelData.reason, line.modelData.number)
                }

                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font.family: "monospace"
                    text: line.modelData.said
                }
            }
        }

        MathGraph {
            id: graph

            Layout.fillHeight: true
            Layout.fillWidth: true
            maths: root.maths
            objectName: "mathGraph"
            visible: root.maths.drawable
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap
            visible: graph.visible

            Button {
                objectName: "graphResetButton"
                text: qsTr("Fit")

                onClicked: root.maths.resetFrame()
            }

            Button {
                text: qsTr("Closer")

                onClicked: root.maths.zoomBy(0.8, 0, 0)
            }

            Button {
                text: qsTr("Further")

                onClicked: root.maths.zoomBy(1.25, 0, 0)
            }

            Item {
                Layout.fillWidth: true
            }
        }

        Item {
            Layout.fillHeight: true
            visible: !graph.visible
        }
    }
}
