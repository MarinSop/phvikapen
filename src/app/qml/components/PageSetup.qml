import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

ColumnLayout {
    id: root

    property NotebookViewModel notebook: null
    readonly property list<int> backgrounds: [PageOptions.Blank, PageOptions.Lined, PageOptions.Grid, PageOptions.Dotted]
    readonly property list<string> backgroundNames: [qsTr("Blank"), qsTr("Lined"), qsTr("Grid"), qsTr("Dotted")]
    readonly property list<int> papers: [PageOptions.Infinite, PageOptions.A3, PageOptions.A4, PageOptions.A5, PageOptions.Letter, PageOptions.Legal, PageOptions.Custom]
    readonly property list<string> paperNames: [qsTr("Infinite"), qsTr("A3"), qsTr("A4"), qsTr("A5"), qsTr("Letter"), qsTr("Legal"), qsTr("As imported")]

    spacing: 4

    Label {
        text: qsTr("Page setup")
    }

    ComboBox {
        id: paperBox

        Layout.fillWidth: true
        currentIndex: root.notebook === null ? -1 : root.papers.indexOf(root.notebook.paper)
        model: root.paperNames
        objectName: "paperBox"

        onActivated: root.notebook.paper = root.papers[paperBox.currentIndex]
    }

    ComboBox {
        id: backgroundBox

        Layout.fillWidth: true
        currentIndex: root.notebook === null ? -1 : root.backgrounds.indexOf(root.notebook.background)
        model: root.backgroundNames
        objectName: "backgroundBox"

        onActivated: root.notebook.background = root.backgrounds[backgroundBox.currentIndex]
    }

    Switch {
        id: landscapeSwitch

        checked: root.notebook !== null && root.notebook.orientation === PageOptions.Landscape
        enabled: root.notebook !== null && root.notebook.paper !== PageOptions.Infinite
        objectName: "landscapeSwitch"
        text: qsTr("Landscape")

        onToggled: root.notebook.orientation = landscapeSwitch.checked ? PageOptions.Landscape : PageOptions.Portrait
    }

    RowLayout {
        Layout.fillWidth: true
        visible: root.notebook !== null && root.notebook.background !== PageOptions.Blank

        Label {
            text: qsTr("Spacing")
        }

        Slider {
            id: spacingSlider

            Layout.fillWidth: true
            from: 2
            stepSize: 0.5
            to: 30
            value: root.notebook === null ? 7 : root.notebook.lineSpacing

            onMoved: root.notebook.lineSpacing = spacingSlider.value
        }
    }
}
